#include "framework.h"
#include <windowsx.h>
#include <wil/result.h>
#include <iostream>

const SIZE szInitial = { 700, 500 };

namespace Foundation = winrt::Windows::Foundation;
namespace Numerics = Foundation::Numerics;
namespace UI = winrt::Windows::UI;

namespace UIComposition {
	using namespace winrt::Windows::UI::Composition;
	namespace abi = ABI::Windows::UI::Composition;
}

namespace Canvas = winrt::Microsoft::Graphics::Canvas;

constexpr const wchar_t* SvgClose =
L"<svg viewBox='0 0 16 16'><path d='M2.589 2.716l.058-.069a.498.498 0 01.637-.058l.069.058L8 7.293l4.646-4.647a.5.5 0 01.707.707L8.707 8l4.647 4.646a.5.5 0 01.058.638l-.058.069a.5.5 0 01-.638.058l-.069-.058L8 8.707l-4.646 4.647a.5.5 0 01-.707-.707L7.293 8 2.646 3.354a.501.501 0 01-.057-.638l.058-.069-.058.069z'></path></svg>";

constexpr const wchar_t* SvgMaximize =
L"<svg viewBox='0 0 16 16'><path d='M4.5 3A1.5 1.5 0 003 4.5v7A1.5 1.5 0 004.5 13h7a1.5 1.5 0 001.5-1.5v-7A1.5 1.5 0 0011.5 3h-7zm0 1h7a.5.5 0 01.5.5v7a.5.5 0 01-.5.5h-7a.5.5 0 01-.5-.5v-7a.5.5 0 01.5-.5z'></path></svg>";

constexpr const wchar_t* SvgMinimize =
L"<svg viewBox='0 0 16 16'><path d='M3.5 7h9c.28 0 .5.22.5.5s-.22.5-.5.5h-9c-.28 0-.5-.22-.5-.5s.22-.5.5-.5z'></path></svg>";

constexpr const wchar_t* SvgRestore =
L"<svg viewBox='0 0 16 16'><path d='M5.084 4a1.5 1.5 0 011.415-1h3.5a3 3 0 013 3v3.5a1.5 1.5 0 01-1 1.415V6a2 2 0 00-2-2H5.084z'></path><path d='M4.5 5h5A1.5 1.5 0 0111 6.5v5A1.5 1.5 0 019.5 13h-5A1.5 1.5 0 013 11.5v-5A1.5 1.5 0 014.5 5zm0 1a.5.5 0 00-.5.5v5a.5.5 0 00.5.5h5a.5.5 0 00.5-.5v-5a.5.5 0 00-.5-.5h-5z'></path></svg>";

UIComposition::Compositor compositor{ nullptr };
UIComposition::Desktop::DesktopWindowTarget target{ nullptr };
UIComposition::ContainerVisual root{ nullptr };

UIComposition::CompositionBrush CreateBackdropBrush(UIComposition::Compositor compositor) {
	auto with_blurred_backdrop = compositor.try_as<UIComposition::abi::ICompositorWithBlurredWallpaperBackdropBrush>();
	if (with_blurred_backdrop) {
		winrt::com_ptr<UIComposition::abi::ICompositionBackdropBrush> brush;
		if (SUCCEEDED(with_blurred_backdrop->TryCreateBlurredWallpaperBackdropBrush(brush.put()))) {
			return brush.as<UIComposition::CompositionBrush>();
		}
	}
	return compositor.CreateColorBrush(UI::Colors::White());
}

void SetBackdrop(UIComposition::Desktop::DesktopWindowTarget target) {
	auto supports_backdrop = target.try_as<UIComposition::abi::ICompositionSupportsSystemBackdrop>();
	if (!supports_backdrop) {
		return;
	}

	auto brush = CreateBackdropBrush(target.Compositor());
	supports_backdrop->put_SystemBackdrop(brush.as<UIComposition::abi::ICompositionBrush>().get());
}

template<typename Callable>
UIComposition::CompositionSurfaceBrush CreateCanvasBrush(
	UIComposition::Compositor compositor,
	winrt::Windows::Foundation::Size size,
	Callable&& callable
) {
	auto canvas_device = Canvas::CanvasDevice::GetSharedDevice();
	auto composition_device = Canvas::UI::Composition::CanvasComposition::CreateCompositionGraphicsDevice(compositor, canvas_device);
	auto drawing_surface = composition_device.CreateDrawingSurface(
		size,
		winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
		winrt::Windows::Graphics::DirectX::DirectXAlphaMode::Premultiplied
	);

	{
		auto drawing_session = Canvas::UI::Composition::CanvasComposition::CreateDrawingSession(drawing_surface);
		callable(canvas_device, drawing_session);
		drawing_session.Flush();
	}

	auto surface_brush = compositor.CreateSurfaceBrush();
	surface_brush.Surface(drawing_surface);
	return surface_brush;
}

UIComposition::CompositionSurfaceBrush CreateSvgBrush(
	UIComposition::Compositor compositor,
	winrt::Windows::Foundation::Size size,
	winrt::hstring svg
) {
	return CreateCanvasBrush(compositor, size, [size, svg = std::move(svg)](
		Canvas::CanvasDevice canvas_device,
		Canvas::CanvasDrawingSession drawing_session) {
		auto svg_document = Canvas::Svg::CanvasSvgDocument::LoadFromXml(canvas_device, svg);
		drawing_session.DrawSvg(svg_document, size);
	});
}

std::wstring GetLocaleNameFromLCID(DWORD lcid) {
	std::wstring result(LOCALE_NAME_MAX_LENGTH, L'\0');
	auto length = static_cast<size_t>(
		::LCIDToLocaleName(lcid, result.data(), LOCALE_NAME_MAX_LENGTH, 0 /* dwFlags */));
	THROW_LAST_ERROR_IF(length == 0);
	result.resize(length - 1);
	return (result);
}


bool IsRTL() {
	LANGID lang_id = ::GetUserDefaultUILanguage();
	std::wstring locale_name = GetLocaleNameFromLCID(MAKELCID(lang_id, SORT_DEFAULT));

	uint32_t reading_layout;

	THROW_IF_WIN32_BOOL_FALSE(::GetLocaleInfoEx(locale_name.c_str(),
		LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER,
		reinterpret_cast<LPWSTR>(&reading_layout),
		sizeof(reading_layout) / sizeof(wchar_t)));

	// 1 => Read from right to left, as for Arabic locales.
	// https://docs.microsoft.com/en-us/windows/win32/intl/locale-ireadinglayout
	return (reading_layout == 1);
}

class SystemMenu {
public:
	explicit SystemMenu(HWND hwnd) : hwnd_{ hwnd }, hmenu_{ THROW_LAST_ERROR_IF_NULL(::GetSystemMenu(hwnd, FALSE)) } {}

	void Show(uint32_t hit_test_code, LPARAM lparam) {
		UINT flags = TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
		WI_SetFlagIf(flags, TPM_LAYOUTRTL, IsRTL());
		WI_SetFlagIf(flags, TPM_RIGHTALIGN, ::GetSystemMetrics(SM_MENUDROPALIGNMENT));

		PrepareToShow(hit_test_code);

		auto x = GET_X_LPARAM(lparam);
		auto y = GET_Y_LPARAM(lparam);
		auto command = ::TrackPopupMenu(hmenu_, flags, x, y, 0, hwnd_, nullptr);
		::SendMessageW(hwnd_, WM_SYSCOMMAND, command, lparam);
	}

private:

	void PrepareToShow(uint32_t hit_test_code) {
		uint16_t default_command = SC_CLOSE;
		if (hit_test_code == HTCAPTION) {
			default_command = ::IsZoomed(hwnd_) ? SC_RESTORE : SC_MAXIMIZE;
		}
		THROW_IF_WIN32_BOOL_FALSE(::SetMenuDefaultItem(hmenu_, default_command, FALSE));

		bool maximized = ::IsZoomed(hwnd_);
		bool minimized = ::IsIconic(hwnd_);
		bool restored = !maximized && !minimized;

		SetMenuItemEnabled(SC_RESTORE, !restored);
		SetMenuItemEnabled(SC_SIZE, restored);
		SetMenuItemEnabled(SC_MOVE, restored);
		SetMenuItemEnabled(SC_MINIMIZE, !minimized);
		SetMenuItemEnabled(SC_MAXIMIZE, !maximized);
	}

	void SetMenuItemEnabled(uint16_t command, bool enabled) {
		THROW_LAST_ERROR_IF(::EnableMenuItem(hmenu_, command, enabled ? MF_ENABLED : MF_DISABLED) == -1);
	}

private:
	HWND hwnd_;
	HMENU hmenu_;
};

enum class RendererState { Normal, MouseOver, MouseDown };

class Renderer {
public:
	virtual ~Renderer() = default;
	virtual void SetRasterizationScale(float) {}
	virtual void SetState(RendererState) {}

	virtual UIComposition::Visual Visual() = 0;
};

template <typename RendererT>
class DelegatingRenderer : public Renderer {
protected:
	template<typename... Args>
	DelegatingRenderer(Args&&... args) : renderer_{ std::forward<Args>(args)... } {}

	RendererT& Delegate() { return renderer_; }

public:
	void SetRasterizationScale(float scale) { renderer_.SetRasterizationScale(scale); }
	void SetState(RendererState state) { renderer_.SetState(state); }
	UIComposition::Visual Visual() { return renderer_.Visual(); }

private:
	RendererT renderer_;
};

struct RendererColors {
	UI::Color normal;
	UI::Color hover;
	UI::Color active;
};


class ContainerRenderer final : public Renderer {
public:
	explicit ContainerRenderer(UIComposition::Compositor compositor)
		: visual_{ compositor.CreateContainerVisual() }
	{
	}

	void InsertAtTop(std::unique_ptr<Renderer> renderer) {
		visual_.Children().InsertAtTop(renderer->Visual());
		renderers_.emplace_back(std::move(renderer));
	}

	void SetState(RendererState state) final {
		ForEach(&Renderer::SetState, state);
	}

	void SetRasterizationScale(float dpi) final {
		ForEach(&Renderer::SetRasterizationScale, dpi);
	}

	UIComposition::Visual Visual() final { return visual_; }

private:
	template <typename Callable, typename... Args>
	void ForEach(Callable&& callable, Args&&... args) {
		for (auto& renderer : renderers_) {
			std::invoke(callable, renderer, args...);
		}
	}

private:
	std::vector<std::unique_ptr<Renderer>> renderers_;
	UIComposition::ContainerVisual visual_;
};

class BackgroundRenderer final : public Renderer {
public:
	BackgroundRenderer(UIComposition::Compositor compositor, RendererColors background_colors)
		: visual_{ compositor.CreateSpriteVisual() },
		colors_{ background_colors },
		brush_{ compositor.CreateColorBrush() } {
		visual_.Brush(brush_);
		visual_.RelativeSizeAdjustment({ 1, 1 });
		brush_.Color(background_colors.normal);
	}

	void SetState(RendererState state) final {
		switch (state) {
		case RendererState::MouseDown:
			brush_.Color(colors_.active);
			break;
		case RendererState::MouseOver:
			brush_.Color(colors_.hover);
			break;
		default:
			brush_.Color(colors_.normal);
			break;
		}
	}

	UIComposition::Visual Visual() final { return visual_; }

private:
	UIComposition::CompositionColorBrush brush_;
	UIComposition::SpriteVisual visual_;
	RendererColors colors_;
};

class SpriteRenderer final : public Renderer {
public:
	using BrushFactory = std::function<UIComposition::CompositionBrush(UIComposition::Compositor, float)>;

	SpriteRenderer(UIComposition::Compositor compositor, BrushFactory brush_factory = nullptr)
		: compositor_{ std::move(compositor) },
		brush_factory_{ std::move(brush_factory) },
		visual_{ compositor_.CreateSpriteVisual() } {
		Update();
	}

	void SetBrushFactory(BrushFactory brush_factory) {
		brush_factory_ = std::move(brush_factory);
		Update();
	}

	void SetRasterizationScale(float scale) final {
		if (scale_ != scale) {
			scale_ = scale;
			Update();
		}
	}

	UIComposition::Visual Visual() final { return visual_; }
	UIComposition::SpriteVisual SpriteVisual() { return visual_; }

private:

	void Update() {
		brush_ = brush_factory_ ? brush_factory_(compositor_, scale_) : nullptr;
		visual_.Brush(brush_);
	}

	BrushFactory brush_factory_;
	UIComposition::Compositor compositor_;
	UIComposition::SpriteVisual visual_;

	UIComposition::CompositionBrush brush_ = nullptr;
	float scale_ = 1.0f;
};

std::unique_ptr<SpriteRenderer> MakeButtonGlyphRenderer(UIComposition::Compositor compositor, winrt::hstring glyph) {
	return std::make_unique<SpriteRenderer>(
		compositor,
		[glyph = std::move(glyph)](UIComposition::Compositor compositor, float rasterization_scale)
	{
		auto size = 16.0f * rasterization_scale;
		auto brush = CreateSvgBrush(compositor, { size,size }, glyph);
		brush.Stretch(UIComposition::CompositionStretch::None);
		brush.SnapToPixels(true);
		return brush;
	});
}

class ButtonRenderer final : public DelegatingRenderer<ContainerRenderer> {
public:

	ButtonRenderer(UIComposition::Compositor compositor, RendererColors background_colors, const std::vector<winrt::hstring>& glyphs)
		: DelegatingRenderer{ compositor } {

		Delegate().InsertAtTop(std::make_unique<BackgroundRenderer>(compositor, background_colors));
		glyph_visuals_.reserve(glyphs.size());

		auto first = true;
		for (const auto& glyph : glyphs) {
			auto renderer = MakeButtonGlyphRenderer(compositor, glyph);
			auto visual = renderer->Visual();
			glyph_visuals_.push_back(visual);
			Delegate().InsertAtTop(std::move(renderer));

			visual.RelativeSizeAdjustment({ 1,1 });
			visual.IsVisible(first);
			first = false;
		}
	}

	void SetActiveGlyph(size_t index) {
		for (size_t i = 0; i < glyph_visuals_.size(); ++i) {
			glyph_visuals_[i].IsVisible(i == index);
		}
	}


private:
	std::vector<UIComposition::Visual> glyph_visuals_;
};

struct Element {

	const char* comment = "";
	uint32_t hit_test_code = HTCLIENT;
	std::unique_ptr<Renderer> renderer;

	explicit Element(
		const char* comment,
		uint32_t hit_test_code
	) : comment{ comment },
		hit_test_code{ hit_test_code }
	{
	}

	bool Contains(const POINT& pt)const {
		return PtInRect(&rect_, pt);
	}

	void SetDpi(uint32_t dpi) {
		if (renderer) {
			renderer->SetRasterizationScale(dpi / 96.0f);
		}
	}

	void Bounds(const RECT& rect) {
		rect_ = rect;
		if (renderer) {
			renderer->Visual().Offset({ (float)rect.left, (float)rect.top, 0.0f });
			renderer->Visual().Size({ (float)(rect.right - rect.left), (float)(rect.bottom - rect.top) });
		}
	}

	const RECT& Bounds() const { return rect_; }

	void SetState(RendererState state) {
		if (renderer) renderer->SetState(state);
	}

private:
	RECT rect_{};
};

std::ostream& operator<<(std::ostream& stream, const RECT& rect) {
	auto width = (rect.right - rect.left);
	auto height = (rect.bottom - rect.top);
	return stream << rect.left << "," << rect.top << " " << width << "x" << height;
}

std::ostream& operator<<(std::ostream& stream, const Element& el) {
	return stream << el.comment;
}

std::ostream& operator<<(std::ostream& stream, const Element* el) {
	if (el) {
		return stream << *el;
	}
	else {
		return stream << "(none)";
	}
}

enum class MouseButton { Left, Right, Other };

struct MouseDownState {
	std::optional<MouseButton> button;
	Element* element = nullptr;
};

Element max{ "max", HTMAXBUTTON };
Element close{ "close", HTCLOSE };
Element title{ "title", HTCAPTION };
Element min{ "min", HTMINBUTTON };
Element icon{ "icon", HTSYSMENU };
Element canvas{ "canvas", HTCLIENT };

ButtonRenderer* max_renderer;
UIComposition::CompositionColorBrush mouse_brush = nullptr;
UIComposition::CompositionColorBrush mouse_down_brush = nullptr;
UIComposition::SpriteVisual mouse_visual = nullptr;

std::vector<std::reference_wrapper<Element>> elements{ canvas, title, icon, min, max, close };
MouseDownState mouse_state;

template <typename Callable, typename...Args>
void ForEachElement(Callable&& callable, Args&&... args) {
	for (Element& element : elements) {
		std::invoke(callable, element, args...);
	}
}

template <typename Callable, typename...Args>
Element* FindElement(Callable&& callable, Args&&... args) {
	for (auto it = elements.crbegin(); it != elements.crend(); ++it) {
		if (std::invoke(callable, *it, args...)) {
			return &it->get();
		}
	}
	return nullptr;
}

Element* FindElementAt(POINT pt) {
	return FindElement(&Element::Contains, pt);
}

Element* FindElementAtClientLParam(LPARAM lParam) {
	POINT ptClient = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	return FindElementAt(ptClient);
}

Element* FindElementWithHT(WPARAM ht) {
	return FindElement([ht](const Element& element) {return element.hit_test_code == ht; });
}

void MouseOver(Element& element, HWND hwnd) {
	ForEachElement([&element](Element& el) {
		if (&el == &element) {
			el.SetState(&el == mouse_state.element ? RendererState::MouseDown : RendererState::MouseOver);

		}
		else {
			el.SetState(RendererState::Normal);
		}
		});

	TRACKMOUSEEVENT tme = { sizeof(tme) };
	tme.hwndTrack = hwnd;
	tme.dwFlags = TME_HOVER | TME_LEAVE;

	if (element.hit_test_code != HTCLIENT)
	{
		//
		// Problem: Because this mouse move is 'simulated' from WM_NCMOUSEMOVE calling TrackMouseEvent
		//          without the TME_NONCLIENT will generate the WM_MOUSELEAVE immediately.
		//
		//          Our window never uses HTCLIENT, so all WM_MOUSE messages are for the max button
		//          (which requires TME_NONCLIENT). If we ever used HTCLIENT in NCHITTEST this call
		//          to TrackMouseEvent would need to know if the original HT code was HTCLIENT and
		//          not use TME_NONCLIENT.
		//

		tme.dwFlags |= TME_NONCLIENT;
	}

	// Call TrackMouseEvent to ensure we see a WM_MOUSELEAVE.
	::TrackMouseEvent(&tme);
}

void MouseLeave() {
	ForEachElement([](Element& el) {
		el.SetState(RendererState::Normal);
		});
	mouse_visual.Brush(nullptr);
}

void MouseMove(Element* element, HWND hwnd) {
	if (element) {
		MouseOver(*element, hwnd);
	}
	else {
		MouseLeave();
	}
	mouse_visual.Brush(mouse_brush);
}

void MouseDown(Element* element, MouseButton button) {
	ForEachElement([element](Element& el) {
		el.SetState(&el == element ? RendererState::MouseDown : RendererState::Normal);
		});
	mouse_state.element = element;
	mouse_state.button = button;
}

void MouseUp(Element* element, HWND hwnd) {
	MouseMove(element, hwnd);
	mouse_state.element = nullptr;
	mouse_state.button = std::nullopt;
}

void LayoutElements(const RECT& rcClient, uint32_t dpi) {

	int kButtonHeight = MulDiv(47, dpi, 96);
	int kButtonWidth = MulDiv(44, dpi, 96);

	RECT rcTop = rcClient;
	rcTop.bottom = kButtonHeight;
	title.Bounds(rcTop);

	RECT rcIcon = rcTop;
	rcIcon.right = rcTop.left + kButtonWidth;
	icon.Bounds(rcIcon);

	RECT rcClose = rcTop;
	rcClose.left = rcTop.right - kButtonWidth;
	close.Bounds(rcClose);

	RECT rcMaxButton = rcTop;
	rcMaxButton.left = rcClose.left - kButtonWidth;
	rcMaxButton.right = rcClose.left;
	max.Bounds(rcMaxButton);

	RECT rcMinButton = rcTop;
	rcMinButton.left = rcMaxButton.left - kButtonWidth;
	rcMinButton.right = rcMaxButton.left;
	min.Bounds(rcMinButton);

	RECT rcCanvas = rcClient;
	rcCanvas.top = rcTop.bottom;
	canvas.Bounds(rcCanvas);

	ForEachElement(&Element::SetDpi, dpi);
}

UIComposition::SpriteVisual CreateMouseVisual() {
	auto ellipse = compositor.CreateEllipseGeometry();
	ellipse.Radius({ 8, 8 });
	ellipse.Center({ 8, 8 });

	auto clip = compositor.CreateGeometricClip(ellipse);
	auto sprite = compositor.CreateSpriteVisual();
	sprite.AnchorPoint({ 0.5, 0.5 });
	sprite.Brush(mouse_brush);
	sprite.Size({ 16, 16 });
	sprite.Clip(clip);
	return sprite;
}

void CreateRenderers() {

	title.renderer = std::make_unique<BackgroundRenderer>(
		compositor,
		RendererColors{ UI::Colors::Aqua(), UI::Colors::Aqua(),  UI::Colors::Aqua() }
	);
	root.Children().InsertAtTop(title.renderer->Visual());

	icon.renderer = std::make_unique<BackgroundRenderer>(
		compositor,
		RendererColors{ UI::Colors::BlueViolet(), UI::Colors::BlueViolet(),  UI::Colors::BlueViolet() }
	);
	root.Children().InsertAtTop(icon.renderer->Visual());

	min.renderer = std::make_unique<ButtonRenderer>(
		compositor,
		RendererColors{ UI::Colors::Transparent(), UI::Colors::NavajoWhite(), UI::Colors::LightGray() },
		std::vector<winrt::hstring>{ SvgMinimize }
	);
	root.Children().InsertAtTop(min.renderer->Visual());

	auto max_ptr = std::make_unique<ButtonRenderer>(
		compositor,
		RendererColors{ UI::Colors::Transparent(), UI::Colors::NavajoWhite(), UI::Colors::LightGray() },
		std::vector<winrt::hstring>{ SvgRestore, SvgMaximize }
	);
	root.Children().InsertAtTop(max_ptr->Visual());
	max_renderer = max_ptr.get();
	max.renderer = std::move(max_ptr);

	close.renderer = std::make_unique<ButtonRenderer>(
		compositor,
		RendererColors{ UI::Colors::Transparent(), UI::Colors::Red(), UI::Colors::DarkRed() },
		std::vector<winrt::hstring>{SvgClose}
	);
	root.Children().InsertAtTop(close.renderer->Visual());

	mouse_brush = compositor.CreateColorBrush(UI::Colors::DarkRed());
	mouse_down_brush = compositor.CreateColorBrush(UI::Colors::Yellow());
	mouse_visual = CreateMouseVisual();
	root.Children().InsertAtTop(mouse_visual);

}

void ConvertToClientMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
	UINT clientMsg = msg + (WM_MOUSEMOVE - WM_NCMOUSEMOVE);
	//printf("Simulating mouse message 0x%X\n", clientMsg);

	// NC message use screen coordinates, but non-NC message use client.
	// Convert the LParam from screen to client.
	POINT ptScreen = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	POINT ptClient = ptScreen;
	ScreenToClient(hwnd, &ptClient);
	LPARAM lpClient = MAKELPARAM(ptClient.x, ptClient.y);

	//
	// Problem: By default, MAXBUTTON would NOT generate WM_MOUSE* messages (it is non-client area).
	//
	//          Send the non-NC equivalent messages to ourselves manually.
	//

	::SendMessageW(hwnd, clientMsg, 0, lpClient);
}

void LogMessage(bool nc, const char* message, Element* element) {
	std::cout << (nc ? "NC " : "   ") << message << " element=" << element << '\n';
}

void HandleMouseUp(Element* element, HWND hwnd, LPARAM lparam, MouseButton button) {
	auto same_as_mouse_down = mouse_state.element == element;
	if (element && same_as_mouse_down) {
		std::cout << "  mouse up element same as mouse down element\n";
		if (button == MouseButton::Right) {
			switch (element->hit_test_code) {
			case HTCAPTION:
			case HTSYSMENU:
				SystemMenu menu{ hwnd };
				menu.Show(element->hit_test_code, lparam);
				break;
			}
		}
		else if (button == MouseButton::Left) {
			switch (element->hit_test_code) {
			case HTCLOSE:
				::DestroyWindow(hwnd);
				break;

			case HTSYSMENU:
				SystemMenu{ hwnd }.Show(HTSYSMENU, lparam);
				break;

			case HTMAXBUTTON:
				::ShowWindow(hwnd, ::IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
				break;

			case HTMINBUTTON:
				::CloseWindow(hwnd);
				break;
			}
		}
	}
	MouseUp(element, hwnd);
}

#define LOG_MESSAGE(message) case (message): printf(#message " wParam=%08x lParam=%08x\n", (uint32_t)wParam, (uint32_t)lParam); break;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		LOG_MESSAGE(WM_SYSCOMMAND);
		LOG_MESSAGE(WM_CONTEXTMENU);
		LOG_MESSAGE(WM_ENTERMENULOOP);

	case WM_NCMOUSEMOVE:
	{
		//
		// Problem: Because TrackMouseEvent called with TME_NONCLIENT we get a WM_NCMOUSELEAVE (not
		//          WM_MOUSELEAVE). Likewise if cursor moves from HTMAXBUTTON to HTCAPTION this is
		//          treated as 'still over the NC area' (though we consider this a leave).
		//
		//          In either case send ourselves a WM_MOUSELEAVE if we believe the cursor is still
		//          hovering over the max button.
		//


		auto element = FindElementWithHT(wParam);
		MouseMove(element, hwnd);

		return 0;

		//
		// GOAL: WndProc handles WM_MOUSEMOVE/ TrackMouseEvent/ WM_MOUSELEAVE
		//       to implement a 'fake' maximize button over the main area of the window.
		//       Must show the flyout when you hover and get enter/ leave input messages (for hover state).
		//
	}
	case WM_MOUSEMOVE:
	{
		auto element = FindElementAtClientLParam(lParam);
		MouseMove(element, hwnd);

		mouse_visual.Offset({ (float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), 0.0f });

		break;
	}


	case WM_NCMOUSELEAVE:
		MouseLeave();
		break;

	case WM_MOUSELEAVE:
		MouseLeave();
		break;

	case WM_LBUTTONDOWN:
	{
		auto element = FindElementAtClientLParam(lParam);
		MouseDown(element, MouseButton::Left);
		mouse_visual.Brush(mouse_down_brush);
		break;
	}

	case WM_NCLBUTTONDOWN:
	{
		auto element = FindElementWithHT(wParam);
		MouseDown(element, MouseButton::Left);

		if (element) {
			wParam = element->hit_test_code;
		}

		switch (wParam) {
		case HTMAXBUTTON:
		case HTMINBUTTON:
		case HTCLOSE:
		case HTSYSMENU:
			//
			// Problem: Handle these messages (do NOT call DefWindowProc).
			//          Default handling for NC messages with the caption button HT codes (like MAXBUTTON)
			//          have various side effects we do not want (because we're handling the input over these areas).
			//
			//          Notably WM_NCLBUTTONDOWN with HTMAXBUTTON will show an old bitmap of the 'depressed'
			//          maximize button in the default location.
			//          See xxxDCETrackCaptionButton/ xxxTrackCaptionButton.
			//
			//          Note: We MUST pass the HTCAPTION to DefWindowProc (or else we'd break dragging).
			//
			return 0;
		}
	}
	break;

	case WM_NCRBUTTONDOWN:
	{
		auto element = FindElementWithHT(wParam);
		MouseDown(element, MouseButton::Right);
		break;
	}

	case WM_RBUTTONDOWN:
	{
		auto element = FindElementAtClientLParam(lParam);
		MouseDown(element, MouseButton::Right);
		break;
	}

	case WM_LBUTTONUP:
	{
		auto element = FindElementAtClientLParam(lParam);
		HandleMouseUp(element, hwnd, lParam, MouseButton::Left);
		mouse_visual.Brush(mouse_brush);
		break;
	}

	case WM_NCLBUTTONUP:
	{
		auto element = FindElementWithHT(wParam);
		HandleMouseUp(element, hwnd, lParam, MouseButton::Left);
		break;
	}

	case WM_RBUTTONUP:
	{
		auto element = FindElementAtClientLParam(lParam);
		HandleMouseUp(element, hwnd, lParam, MouseButton::Right);
		break;
	}

	case WM_NCRBUTTONUP:
	{
		auto element = FindElementWithHT(wParam);
		HandleMouseUp(element, hwnd, lParam, MouseButton::Right);
		return 0; // we're handling input
	}

	//Handle WM_NC* messages. Do NOT send them to DefWindowProc
	case WM_NCLBUTTONDBLCLK:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCMBUTTONDBLCLK:
		switch (wParam) {
		case HTMAXBUTTON:
		case HTMINBUTTON:
			return 0;
		}
		break;

		//
		// RESTRICTION: NCHITTEST over this area must return HTMAXBUTTON (or else no flyout).
		//
		//              Using WM_WINDOWPOSCHANGED, create a top area that is draggable and
		//              'close button' in top-right).
		//
		//              Using WM_NCHITTEST, if default is HTCLIENT, split into HTTOP, HTCLOSE,
		//              HTCAPTION, and for the rest use HTMAXBUTTON.
		//
	case WM_SIZE:
	{
		RECT rcClient;
		::GetClientRect(hwnd, &rcClient);
		const UINT dpi = ::GetDpiForWindow(hwnd);
		LayoutElements(rcClient, dpi);
		max_renderer->SetActiveGlyph(wParam == SIZE_MAXIMIZED ? 0 : 1);
		break;
	}

	case WM_NCHITTEST:
	{
		// Get the default HT code and only handle if HTCLIENT.
		// This allows the left/right/bottom resize HT codes to flow through.
		UINT ht = (UINT)::DefWindowProcW(hwnd, msg, wParam, lParam);

		if (ht != HTCLIENT)
		{
			return ht;
		}

		POINT ptClient = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		::ScreenToClient(hwnd, &ptClient);

		// Top resize
		if (ptClient.y < MulDiv(8, GetDpiForWindow(hwnd), 96))
		{
			return HTTOP;
		}

		auto element = FindElementAt(ptClient);
		if (element) {
			std::cout << "WM_NCHITTEST element=" << element << " result=" << element->hit_test_code << '\n';
			return element->hit_test_code;
		}

		// HTCLIENT for the rest
		return HTCLIENT;
	}


	//
	// Problem: If window has a regular max button, shell will find that
	//          cursor isn't over that max button and not show the flyout...
	//          pcshell/twinui/SnapComponent/lib/SnapFlyoutHost.cpp
	//          SnapFlyoutHost::InternalShow
	//          SnapLayoutHelpers::TryGetMaximizeButtonRectFromDwm
	//
	//          You can ask for no non-client area (using window styles, like WS_POPUP)
	//          but not having the styles for NC parts will hit heuristics where certain
	//          ppl ignore the window which will block us from getting the flyout.
	//          For example, ntuser (HitTestPartUpdate) avoids windows without WS_MAXIMIZEBOX.
	//
	//          Use WM_NCCALCSIZE to set rcClient.top = rcWindow.top, which hides the default
	//          caption bar (while still marking the window with all the styles/ WS_OVERLAPPEDWINDOW).
	//
	//          Note: By not touching the left/right/bottom we retain our resize borders.
	//

	case WM_NCCALCSIZE:
	{
		RECT* prc = (PRECT)lParam;
		auto top = prc->top;
		auto lRet = ::DefWindowProcW(hwnd, msg, wParam, lParam);
		prc->top = top;
		return lRet;
	}


	// On WM_ACTIVATE set isActive and repaint.
	case WM_ACTIVATE:
		//isActive = (wParam != WA_INACTIVE);
		break;

	case WM_CREATE:
	{
		using namespace winrt::Windows::System;

		DispatcherQueueOptions options{ sizeof(DispatcherQueueOptions) };
		options.apartmentType = DQTAT_COM_ASTA;
		options.threadType = DQTYPE_THREAD_CURRENT;

		DispatcherQueueController controller{ nullptr };
		winrt::check_hresult(::CreateDispatcherQueueController(options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**> (winrt::put_abi(controller))));

		compositor = UIComposition::Compositor();
		auto interop = compositor.as<UIComposition::abi::Desktop::ICompositorDesktopInterop>();
		winrt::check_hresult(interop->CreateDesktopWindowTarget(hwnd, FALSE, reinterpret_cast<UIComposition::abi::Desktop::IDesktopWindowTarget**>(winrt::put_abi(target))));

		root = compositor.CreateContainerVisual();
		target.Root(root);
		SetBackdrop(target);
		CreateRenderers();

		UINT dpi = ::GetDpiForWindow(hwnd);
		SIZE sz = { ::MulDiv(szInitial.cx, dpi, 96), ::MulDiv(szInitial.cy, dpi, 96) };
		::SetWindowPos(hwnd, nullptr, 150, 300, sz.cx, sz.cy, SWP_SHOWWINDOW);
		break;
	}

	// Destroy the window on escape key
	case WM_CHAR:
		if (wParam == VK_ESCAPE)
		{
			::DestroyWindow(hwnd);
		}
		break;

	case WM_DPICHANGED:
	{
		RECT* prc = (RECT*)lParam;

		::SetWindowPos(hwnd,
			nullptr,
			prc->left,
			prc->top,
			prc->right - prc->left,
			prc->bottom - prc->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		break;
	}

	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	}

	return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void InitWindow(HINSTANCE hInst)
{
	PCWSTR wndClassName = L"WndClass";
	PCWSTR windowTitle = L"BigMaxButton";

	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = wndClassName;

	THROW_LAST_ERROR_IF(::RegisterClassExW(&wc) == INVALID_ATOM);

	THROW_LAST_ERROR_IF_NULL(::CreateWindowExW(0,
		wndClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		nullptr,
		nullptr,
		hInst,
		nullptr));

}

int __stdcall wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ wchar_t*, _In_ int)
{
	::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	InitWindow(hInstance);

	MSG msg;
	while (::GetMessageW(&msg, nullptr, 0, 0))
	{
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}

	return 0;
}

int wmain(int, wchar_t* []) {
	wchar_t str[] = L"";
	return wWinMain(::GetModuleHandleW(nullptr), nullptr, str, SW_NORMAL);
}
