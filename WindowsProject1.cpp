#include "framework.h"
#include <windowsx.h>
#include <stdio.h>

const SIZE szInitial = { 700, 500 };

namespace UI = winrt::Windows::UI;

namespace UIComposition {
	using namespace winrt::Windows::UI::Composition;
	namespace abi = ABI::Windows::UI::Composition;
}

namespace Canvas = winrt::Microsoft::Graphics::Canvas;

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

UIComposition::CompositionSurfaceBrush CreateSvgBrush(UIComposition::Compositor compositor, const winrt::hstring& svg) {
	auto size = winrt::Windows::Foundation::Size(100, 100);
	auto canvas_device = Canvas::CanvasDevice::GetSharedDevice();
	auto composition_device = Canvas::UI::Composition::CanvasComposition::CreateCompositionGraphicsDevice(compositor, canvas_device);
	auto drawing_surface = composition_device.CreateDrawingSurface(
		size,
		winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
		winrt::Windows::Graphics::DirectX::DirectXAlphaMode::Premultiplied
	);

	{
		auto svg_document = Canvas::Svg::CanvasSvgDocument::LoadFromXml(canvas_device, svg);
		auto drawing_session = Canvas::UI::Composition::CanvasComposition::CreateDrawingSession(drawing_surface);
		drawing_session.DrawSvg(svg_document, size);
	}

	auto surface_brush = compositor.CreateSurfaceBrush();
	surface_brush.Surface(drawing_surface);
	return surface_brush;
}

class Renderer {
public:
	virtual ~Renderer() = default;
	virtual void MouseEnter() = 0;
	virtual void MouseLeave() = 0;
	virtual void MouseDown() = 0;
	virtual void MouseUp() = 0;
	virtual UIComposition::Visual Visual() = 0;
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

	void MouseEnter() final {
		ForEach(&Renderer::MouseEnter);
	}

	void MouseLeave() final {
		ForEach(&Renderer::MouseLeave);
	}

	void MouseDown() final {
		ForEach(&Renderer::MouseDown);
	}

	void MouseUp() final {
		ForEach(&Renderer::MouseUp);
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

	void MouseEnter() final {
		brush_.Color(colors_.hover);
	}

	void MouseLeave() final {
		mouse_down = false;
		brush_.Color(colors_.normal);
	}

	void MouseDown() final {
		mouse_down = true;
		brush_.Color(colors_.active);
	}

	void MouseUp() final {
		mouse_down = false;
		brush_.Color(colors_.hover);
	}

	UIComposition::Visual Visual() final { return visual_; }

private:
	UIComposition::CompositionColorBrush brush_;
	UIComposition::SpriteVisual visual_;
	RendererColors colors_;
	bool mouse_down = false;
};

class GlyphRenderer final : public Renderer {

public:
	GlyphRenderer(UIComposition::Compositor compositor, RendererColors colors)
		: visual_{ compositor.CreateSpriteVisual() },
		brush_{ CreateSvgBrush(compositor, L"<svg><circle fill=\"#660000\" r=\"10\"/></svg>") }
	{
		visual_.RelativeSizeAdjustment({ 1.0f, 1.0f });
	}

	void MouseEnter() final {
	}

	void MouseLeave() final {
	}

	void MouseDown() final {
	}

	void MouseUp() final {
	}

	UIComposition::Visual Visual() final { return visual_; }

private:
	UIComposition::CompositionSurfaceBrush brush_;
	UIComposition::SpriteVisual visual_;
};

std::unique_ptr<Renderer> CreateButtonRenderer(UIComposition::Compositor compositor, RendererColors background_colors) {
	auto container = std::make_unique<ContainerRenderer>(compositor);
	container->InsertAtTop(std::make_unique<BackgroundRenderer>(compositor, background_colors));
	container->InsertAtTop(std::make_unique<GlyphRenderer>(compositor, background_colors));
	return container;
}

struct Element {

	uint32_t hit_test_code;
	std::unique_ptr<Renderer> renderer;

	explicit Element(uint32_t hit_test_code = HTCLIENT) : hit_test_code{ hit_test_code } {
	}

	bool Contains(const POINT& pt)const {
		return PtInRect(&rect_, pt);
	}

	void Bounds(const RECT& rect) {
		rect_ = rect;
		if (renderer) {
			renderer->Visual().Offset({ (float)rect.left, (float)rect.top, 0.0f });
			renderer->Visual().Size({ (float)(rect.right - rect.left), (float)(rect.bottom - rect.top) });
		}
	}

	void MouseEnter() {
		if (renderer) renderer->MouseEnter();
	}

	void MouseLeave() {
		if (renderer) renderer->MouseLeave();
	}

	void MouseDown() {
		if (renderer) renderer->MouseDown();
	}

	void MouseUp() {
		if (renderer) renderer->MouseUp();
	}

private:
	RECT rect_{ 0,0,0,0 };

};

Element max{ HTMAXBUTTON };
Element close{ HTCLOSE };
Element top{ HTCAPTION };
//Element buttons{ HTCLIENT };

std::vector<std::reference_wrapper<Element>> elements{ max, close, top };
Element* mouse_down_element = nullptr;
Element* mouse_over_element = nullptr;

void MouseOver(Element* element) {
	if (mouse_over_element == element) {
		return;
	}
	if (mouse_over_element) {
		mouse_over_element->MouseLeave();
	}
	mouse_over_element = element;
	if (mouse_over_element) {
		element->MouseEnter();
	}
}

Element* MouseDown(Element* element) {
	MouseOver(element);
	if (element) {
		element->MouseDown();
	}
	std::swap(mouse_down_element, element);
	return element;
}

void MouseUp(Element* element) {
	MouseOver(element);
	if (mouse_down_element) {
		element->MouseUp();
	}
}


Element* FindElementAt(POINT pt) {
	for (Element& element : elements) {
		if (element.Contains(pt)) {
			return &element;
		}
	}
	return nullptr;
}

Element* FindElementAtClientLParam(LPARAM lParam) {
	POINT ptClient = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	return FindElementAt(ptClient);
}

Element* FindElementWithHT(WPARAM ht) {
	auto it = std::find_if(elements.cbegin(), elements.cend(), [ht](const Element& e) {return  e.hit_test_code == ht; });
	return it == elements.cend() ? nullptr : &(it->get());
}

void ConvertToClientMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
	UINT clientMsg = msg + (WM_MOUSEMOVE - WM_NCMOUSEMOVE);
	printf("Simulating mouse message 0x%X\n", clientMsg);

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

	SendMessage(hwnd, clientMsg, 0, lpClient);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
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
		MouseOver(element);
		//if (wParam == HTCAPTION) {
		//	SetElementUnderMouse(&top);
		//}
		//else if (wParam == HTCLOSE) {
		//	SetElementUnderMouse(&close);
		//}
		//else {
		ConvertToClientMessage(hwnd, msg, wParam, lParam);
		//}

		return 0;

		//
		// GOAL: WndProc handles WM_MOUSEMOVE/ TrackMouseEvent/ WM_MOUSELEAVE
		//       to implement a 'fake' maximize button over the main area of the window.
		//       Must show the flyout when you hover and get enter/ leave input messages (for hover state).
		//
	}
	case WM_MOUSEMOVE:
	{
		TRACKMOUSEEVENT tme = { sizeof(tme) };
		tme.hwndTrack = hwnd;
		tme.dwFlags = TME_LEAVE;

		auto element = FindElementAtClientLParam(lParam);
		MouseOver(element);

		if (element && element->hit_test_code != HTCLIENT)
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
		break;
	}


	case WM_NCMOUSELEAVE:
	case WM_MOUSELEAVE:
		MouseOver(nullptr);
		break;

	case WM_LBUTTONDOWN:
	{
		printf("WM_LBUTTONDOWN\n");
		auto element = FindElementAtClientLParam(lParam);
		MouseOver(element);
		if (mouse_over_element) {
			mouse_over_element->MouseDown();
		}
		break;
	}

	case WM_NCLBUTTONDOWN:
	{
		printf("WM_NCLBUTTONDOWN\n");
		auto element = FindElementWithHT(wParam);
		MouseDown(element);

		if (wParam == HTMAXBUTTON) {
			return 0; // do not let DefWindowProc see this
		}
	}
	break;

	case WM_LBUTTONUP:
	{
		auto element = FindElementAtClientLParam(lParam);
		MouseUp(element);
		break;
	}

	case WM_NCLBUTTONUP:
	{
		printf("WM_NCLBUTTONUP\n");

		auto element = FindElementWithHT(wParam);
		auto same_as_mouse_down = mouse_down_element == element;

		if (same_as_mouse_down) {
			MouseUp(mouse_down_element);
			if (wParam == HTCLOSE)
			{
				DestroyWindow(hwnd);
				break;
			}
			if (wParam == HTMAXBUTTON) {
				ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
			}
		}
	}
	break;
	//
	// Handle WM_NC* messages. Do NOT send them to DefWindowProc
	//

	case WM_NCLBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCMBUTTONDBLCLK:
	{
		printf("NC message 0x%X\n", msg);

		// If button down on close area, destroy the window.

		if (wParam == HTMAXBUTTON)
		{
			ConvertToClientMessage(hwnd, msg, wParam, lParam);

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
		break;
	}


	//
	// RESTRICTION: NCHITTEST over this area must return HTMAXBUTTON (or else no flyout).
	//
	//              Using WM_WINDOWPOSCHANGED, create a top area that is draggable and
	//              'close button' in top-right).
	//
	//              Using WM_NCHITTEST, if default is HTCLIENT, split into HTTOP, HTCLOSE,
	//              HTCAPTION, and for the rest use HTMAXBUTTON.
	//

	case WM_WINDOWPOSCHANGED:
	{
		RECT rcClient;
		::GetClientRect(hwnd, &rcClient);
		const UINT dpi = ::GetDpiForWindow(hwnd);

		// Top area is entire top 30 pixels
		RECT rcTop = rcClient;
		rcTop.bottom = MulDiv(30, dpi, 96);
		top.Bounds(rcTop);

		// Close area is right 30 pixels of top area
		RECT rcClose = rcTop;
		rcClose.left = rcClose.right - MulDiv(30, dpi, 96);
		close.Bounds(rcClose);

		// Bottom area is bottom 60 pixels of client area
		RECT rcButtons = rcClient;
		rcButtons.top = rcButtons.bottom - MulDiv(60, dpi, 96);

		// Max button is the remaining space
		RECT rcMaxButton = rcClient;
		rcMaxButton.top = rcTop.bottom;
		rcMaxButton.bottom = rcButtons.top;
		max.Bounds(rcMaxButton);

		break;
	}

	case WM_NCHITTEST:
	{
		// Get the default HT code and only handle if HTCLIENT.
		// This allows the left/right/bottom resize HT codes to flow through.
		UINT ht = (UINT)DefWindowProc(hwnd, msg, wParam, lParam);

		if (ht != HTCLIENT)
		{
			return ht;
		}

		POINT ptClient = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(hwnd, &ptClient);

		// Top resize
		if (ptClient.y < MulDiv(8, GetDpiForWindow(hwnd), 96))
		{
			return HTTOP;
		}

		auto element = FindElementAt(ptClient);
		if (element) {
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
		auto lRet = DefWindowProc(hwnd, msg, wParam, lParam);
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


		max.renderer = CreateButtonRenderer(compositor, { UI::Colors::MediumSeaGreen(), UI::Colors::SeaGreen(), UI::Colors::DarkSeaGreen() });
		top.renderer = CreateButtonRenderer(compositor, { UI::Colors::Aqua(), UI::Colors::Aqua(),  UI::Colors::Aqua() });
		close.renderer = CreateButtonRenderer(compositor, { UI::Colors::Transparent(), UI::Colors::Red(), UI::Colors::DarkRed() });

		root.Children().InsertAtTop(max.renderer->Visual());
		root.Children().InsertAtTop(top.renderer->Visual());
		root.Children().InsertAtTop(close.renderer->Visual());

		UINT dpi = GetDpiForWindow(hwnd);
		SIZE sz = { MulDiv(szInitial.cx, dpi, 96), MulDiv(szInitial.cy, dpi, 96) };
		SetWindowPos(hwnd, nullptr, 150, 300, sz.cx, sz.cy, SWP_SHOWWINDOW);
		break;
	}

	// Destroy the window on escape key
	case WM_CHAR:
		if (wParam == VK_ESCAPE)
		{
			DestroyWindow(hwnd);
		}
		break;

	case WM_DPICHANGED:
	{
		RECT* prc = (RECT*)lParam;

		SetWindowPos(hwnd,
			nullptr,
			prc->left,
			prc->top,
			prc->right - prc->left,
			prc->bottom - prc->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		break;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool InitWindow(HINSTANCE hInst)
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

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"RegisterClassEx Failed!", L"ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	if (!CreateWindowEx(0,
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
		nullptr))
	{
		MessageBox(NULL, L"CreateWindowEx Failed!", L"ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	return true;
}

int __stdcall wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ wchar_t*, _In_ int)
{
	if (!InitWindow(hInstance))
	{
		return 1;
	}

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
