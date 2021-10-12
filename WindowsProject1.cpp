#include "framework.h"

#include <wil/result.h>
#include <windowsx.h>
#include <iostream>

const SIZE szInitial = {700, 500};

namespace Foundation = winrt::Windows::Foundation;
namespace Numerics = Foundation::Numerics;
namespace UI = winrt::Windows::UI;

namespace UIC {
using namespace winrt::Windows::UI::Composition;
namespace abi = ABI::Windows::UI::Composition;
}  // namespace UIC

namespace Canvas = winrt::Microsoft::Graphics::Canvas;

UIC::Compositor compositor{nullptr};
UIC::Desktop::DesktopWindowTarget target{nullptr};
UIC::ContainerVisual root{nullptr};

UIC::CompositionBrush CreateBackdropBrush(UIC::Compositor compositor) {
  auto with_blurred_backdrop =
      compositor.try_as<UIC::abi::ICompositorWithBlurredWallpaperBackdropBrush>();
  if (with_blurred_backdrop) {
    winrt::com_ptr<UIC::abi::ICompositionBackdropBrush> brush;
    if (SUCCEEDED(with_blurred_backdrop->TryCreateBlurredWallpaperBackdropBrush(brush.put()))) {
      return brush.as<UIC::CompositionBrush>();
    }
  }
  return compositor.CreateColorBrush(UI::Colors::White());
}

void SetBackdrop(UIC::Desktop::DesktopWindowTarget target) {
  auto supports_backdrop = target.try_as<UIC::abi::ICompositionSupportsSystemBackdrop>();
  if (!supports_backdrop) {
    return;
  }

  auto brush = CreateBackdropBrush(target.Compositor());
  supports_backdrop->put_SystemBackdrop(brush.as<UIC::abi::ICompositionBrush>().get());
}

template <typename Callable>
UIC::CompositionSurfaceBrush CreateCanvasBrush(UIC::Compositor compositor,
                                               winrt::Windows::Foundation::Size size,
                                               Callable&& callable) {
  auto canvas_device = Canvas::CanvasDevice::GetSharedDevice();
  auto composition_device =
      Canvas::UI::Composition::CanvasComposition::CreateCompositionGraphicsDevice(compositor,
                                                                                  canvas_device);
  auto drawing_surface = composition_device.CreateDrawingSurface(
      size,
      winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
      winrt::Windows::Graphics::DirectX::DirectXAlphaMode::Premultiplied);

  {
    auto drawing_session =
        Canvas::UI::Composition::CanvasComposition::CreateDrawingSession(drawing_surface);
    callable(canvas_device, drawing_session);
    drawing_session.Flush();
  }

  auto surface_brush = compositor.CreateSurfaceBrush();
  surface_brush.Surface(drawing_surface);
  return surface_brush;
}

UIC::CompositionSurfaceBrush CreateSvgBrush(UIC::Compositor compositor,
                                            winrt::Windows::Foundation::Size size,
                                            winrt::hstring svg) {
  return CreateCanvasBrush(
      compositor,
      size,
      [size, svg = std::move(svg)](Canvas::CanvasDevice canvas_device,
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

enum class HitTestCode : uint32_t {
  Error = static_cast<uint32_t>(HTERROR),
  Nowhere = static_cast<uint32_t>(HTNOWHERE),
  Client = HTCLIENT,
  Caption = HTCAPTION,
  SystemMenu = HTSYSMENU,
  GrowBox = HTGROWBOX,
  Menu = HTMENU,
  HorizontalScroll = HTHSCROLL,
  VerticalScroll = HTVSCROLL,
  MinimizeButton = HTMINBUTTON,
  MaximizeButton = HTMAXBUTTON,
  LeftBorder = HTLEFT,
  RightBorder = HTRIGHT,
  TopBorder = HTTOP,
  TopLeftCorner = HTTOPLEFT,
  TopRightCorner = HTTOPRIGHT,
  BottomBorder = HTBOTTOM,
  BottomLeftCorner = HTBOTTOMLEFT,
  BottomRightCorner = HTBOTTOMRIGHT,
  Border = HTBORDER,
  Object = HTOBJECT,
  CloseButton = HTCLOSE,
  HelpButton = HTHELP
};

std::ostream& operator<<(std::ostream& stream, const HitTestCode& ht) {
#define CASE_(value)       \
  case HitTestCode::value: \
    return stream << #value;

  switch (ht) {
    CASE_(Error);
    CASE_(Nowhere);
    CASE_(Client);
    CASE_(Caption);
    CASE_(SystemMenu);
    CASE_(GrowBox);
    CASE_(Menu);
    CASE_(HorizontalScroll);
    CASE_(VerticalScroll);
    CASE_(MinimizeButton);
    CASE_(MaximizeButton);
    CASE_(LeftBorder);
    CASE_(RightBorder);
    CASE_(TopBorder);
    CASE_(TopLeftCorner);
    CASE_(TopRightCorner);
    CASE_(BottomBorder);
    CASE_(BottomLeftCorner);
    CASE_(BottomRightCorner);
    CASE_(Border);
    CASE_(Object);
    CASE_(CloseButton);
    CASE_(HelpButton);
  }

  return stream << "Unknown HitTestCode " << static_cast<uint32_t>(ht);
#undef CASE_
}

bool IsSizeHitTestCode(HitTestCode code) {
  auto value = static_cast<uint32_t>(code);
  return value >= HTSIZEFIRST && value <= HTSIZELAST;
}

class SystemMenu {
 public:
  explicit SystemMenu(HWND hwnd)
      : hwnd_{hwnd}, hmenu_{THROW_LAST_ERROR_IF_NULL(::GetSystemMenu(hwnd, FALSE))} {}

  void Show(HitTestCode hit_test_code, bool right_button, const POINT& client_point) {
    UINT flags = TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
    WI_SetFlagIf(flags, TPM_LAYOUTRTL, IsRTL());
    WI_SetFlagIf(flags, TPM_RIGHTBUTTON, right_button);
    WI_SetFlagIf(flags, TPM_RIGHTALIGN, ::GetSystemMetrics(SM_MENUDROPALIGNMENT));

    PrepareToShow(hit_test_code);

    POINT screen_point = client_point;
    THROW_IF_WIN32_BOOL_FALSE(::ClientToScreen(hwnd_, &screen_point));

    auto command =
        ::TrackPopupMenu(hmenu_, flags, screen_point.x, screen_point.y, 0, hwnd_, nullptr);
    ::SendMessageW(hwnd_, WM_SYSCOMMAND, command, MAKELPARAM(screen_point.x, screen_point.y));
  }

 private:
  void PrepareToShow(HitTestCode hit_test_code) {
    uint16_t default_command = SC_CLOSE;
    if (hit_test_code == HitTestCode::Caption) {
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
    THROW_LAST_ERROR_IF(::EnableMenuItem(hmenu_, command, enabled ? MF_ENABLED : MF_DISABLED) ==
                        -1);
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

  virtual UIC::Visual Visual() = 0;
};

template <typename RendererT>
class DelegatingRenderer : public Renderer {
 protected:
  template <typename... Args>
  DelegatingRenderer(Args&&... args) : renderer_{std::forward<Args>(args)...} {}

  RendererT& Delegate() { return renderer_; }

 public:
  void SetRasterizationScale(float scale) { renderer_.SetRasterizationScale(scale); }
  void SetState(RendererState state) { renderer_.SetState(state); }
  UIC::Visual Visual() { return renderer_.Visual(); }

 private:
  RendererT renderer_;
};

struct RendererColors {
  RendererColors(UI::Color color) : normal{color}, hover{color}, active{color} {}
  RendererColors(UI::Color normal, UI::Color hover, UI::Color active)
      : normal{normal}, hover{hover}, active{active} {}
  UI::Color normal;
  UI::Color hover;
  UI::Color active;
};

class ContainerRenderer final : public Renderer {
 public:
  explicit ContainerRenderer(UIC::Compositor compositor)
      : visual_{compositor.CreateContainerVisual()} {}

  void InsertAtTop(std::unique_ptr<Renderer> renderer) {
    visual_.Children().InsertAtTop(renderer->Visual());
    renderers_.emplace_back(std::move(renderer));
  }

  void SetState(RendererState state) final { ForEach(&Renderer::SetState, state); }

  void SetRasterizationScale(float dpi) final { ForEach(&Renderer::SetRasterizationScale, dpi); }

  UIC::Visual Visual() final { return visual_; }

 private:
  template <typename Callable, typename... Args>
  void ForEach(Callable&& callable, Args&&... args) {
    for (auto& renderer : renderers_) {
      std::invoke(callable, renderer, args...);
    }
  }

 private:
  std::vector<std::unique_ptr<Renderer>> renderers_;
  UIC::ContainerVisual visual_;
};

class BackgroundRenderer final : public Renderer {
 public:
  BackgroundRenderer(UIC::Compositor compositor, RendererColors background_colors)
      : visual_{compositor.CreateSpriteVisual()},
        colors_{background_colors},
        brush_{compositor.CreateColorBrush()} {
    visual_.Brush(brush_);
    visual_.RelativeSizeAdjustment({1, 1});
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

  UIC::Visual Visual() final { return visual_; }

 private:
  UIC::CompositionColorBrush brush_;
  UIC::SpriteVisual visual_;
  RendererColors colors_;
};

class SpriteRenderer final : public Renderer {
 public:
  using BrushFactory = std::function<UIC::CompositionBrush(UIC::Compositor, float)>;

  SpriteRenderer(UIC::Compositor compositor, BrushFactory brush_factory = nullptr)
      : compositor_{std::move(compositor)},
        brush_factory_{std::move(brush_factory)},
        visual_{compositor_.CreateSpriteVisual()} {
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

  UIC::Visual Visual() final { return visual_; }
  UIC::SpriteVisual SpriteVisual() { return visual_; }

 private:
  void Update() {
    brush_ = brush_factory_ ? brush_factory_(compositor_, scale_) : nullptr;
    visual_.Brush(brush_);
  }

  BrushFactory brush_factory_;
  UIC::Compositor compositor_;
  UIC::SpriteVisual visual_;

  UIC::CompositionBrush brush_ = nullptr;
  float scale_ = 1.0f;
};

std::unique_ptr<SpriteRenderer> MakeButtonGlyphRenderer(UIC::Compositor compositor,
                                                        winrt::hstring glyph) {
  return std::make_unique<SpriteRenderer>(
      compositor,
      [glyph = std::move(glyph)](UIC::Compositor compositor, float rasterization_scale) {
        auto size = 16.0f * rasterization_scale;
        auto brush = CreateSvgBrush(compositor, {size, size}, glyph);
        brush.Stretch(UIC::CompositionStretch::None);
        brush.SnapToPixels(true);
        return brush;
      });
}

class ButtonRenderer final : public DelegatingRenderer<ContainerRenderer> {
 public:
  ButtonRenderer(UIC::Compositor compositor, RendererColors background_colors, winrt::hstring glyph)
      : ButtonRenderer{compositor, background_colors, std::vector{glyph}} {}

  ButtonRenderer(UIC::Compositor compositor,
                 RendererColors background_colors,
                 const std::vector<winrt::hstring>& glyphs)
      : DelegatingRenderer{compositor} {
    Delegate().InsertAtTop(std::make_unique<BackgroundRenderer>(compositor, background_colors));
    glyph_visuals_.reserve(glyphs.size());

    auto first = true;
    for (const auto& glyph : glyphs) {
      auto renderer = MakeButtonGlyphRenderer(compositor, glyph);
      auto visual = renderer->Visual();
      glyph_visuals_.push_back(visual);
      Delegate().InsertAtTop(std::move(renderer));

      visual.RelativeSizeAdjustment({1, 1});
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
  std::vector<UIC::Visual> glyph_visuals_;
};

enum class MouseButton { Left, Right };

class Element {
 public:
 public:
  virtual ~Element() = default;
  virtual void MouseClick(MouseButton, const POINT&) {}
  virtual void MouseDoubleClick(MouseButton, const POINT&) {}
  virtual void MouseDown(MouseButton, const POINT&) {}
  virtual void MouseEnter() {}
  virtual void MouseLeave() {}
  virtual void MouseUp(MouseButton, const POINT&) {}
  virtual void WindowMaximized(bool) {}

 public:
  const RECT& Bounds() const { return bounds_; }
  void Bounds(RECT bounds) {
    bounds_ = std::move(bounds);
    if (renderer_) {
      auto left = static_cast<float>(bounds.left);
      auto top = static_cast<float>(bounds.top);
      Visual().Offset({left, top, 0.0f});

      auto width = static_cast<float>(bounds.right - bounds.left);
      auto height = static_cast<float>(bounds.bottom - bounds.top);
      Visual().Size({width, height});
    }
  }

  bool CallDefWindowProc() const { return call_dwp_; }
  void CallDefWindowProc(bool value) { call_dwp_ = value; }

  bool Contains(const POINT& pt) const { return ::PtInRect(&bounds_, pt); }

  HitTestCode HitTest() const { return hit_test_result_; }
  void HitTest(HitTestCode value) { hit_test_result_ = value; }

  void MouseState(RendererState state) {
    if (renderer_) {
      renderer_->SetState(state);
    }
  }

  void SetDpi(uint32_t dpi) {
    if (renderer_) {
      renderer_->SetRasterizationScale(dpi / 96.0f);
    }
  }

  UIC::Visual Visual() const { return renderer_ ? renderer_->Visual() : nullptr; }

 protected:
  template <typename T, typename... Args>
  T& CreateRenderer(Args&&... args) {
    auto renderer = std::make_unique<T>(std::forward<Args>(args)...);
    T& ref = *renderer;
    renderer_ = std::move(renderer);
    return ref;
  }

 private:
  RECT bounds_{};
  bool call_dwp_ = false;
  HitTestCode hit_test_result_ = HitTestCode::Client;
  std::unique_ptr<Renderer> renderer_ = nullptr;
};

static constexpr const wchar_t* MaximizeGlyph =
    L"<svg viewBox='0 0 16 16'><path d='M4.5 3A1.5 1.5 0 003 4.5v7A1.5 1.5 0 004.5 13h7a1.5 1.5 0 "
    L"001.5-1.5v-7A1.5 1.5 0 0011.5 3h-7zm0 1h7a.5.5 0 01.5.5v7a.5.5 0 01-.5.5h-7a.5.5 0 "
    L"01-.5-.5v-7a.5.5 0 01.5-.5z'></path></svg>";

static constexpr const wchar_t* RestoreGlyph =
    L"<svg viewBox='0 0 16 16'><path d='M5.084 4a1.5 1.5 0 011.415-1h3.5a3 3 0 013 3v3.5a1.5 1.5 0 "
    L"01-1 1.415V6a2 2 0 00-2-2H5.084z'></path><path d='M4.5 5h5A1.5 1.5 0 0111 6.5v5A1.5 1.5 0 "
    L"019.5 13h-5A1.5 1.5 0 013 11.5v-5A1.5 1.5 0 014.5 5zm0 1a.5.5 0 00-.5.5v5a.5.5 0 "
    L"00.5.5h5a.5.5 0 00.5-.5v-5a.5.5 0 00-.5-.5h-5z'></path></svg>";

class MaximizeElement : public Element {
 public:
  explicit MaximizeElement(HWND hwnd, UIC::Compositor compositor, RendererColors background)
      : hwnd_{hwnd},
        renderer_{CreateRenderer<ButtonRenderer>(
            compositor,
            background,
            std::vector<winrt::hstring>{RestoreGlyph, MaximizeGlyph})} {
    HitTest(HitTestCode::MaximizeButton);
  }

  void MouseClick(MouseButton button, const POINT&) final {
    if (button == MouseButton::Left) {
      ::ShowWindow(hwnd_, ::IsZoomed(hwnd_) ? SW_RESTORE : SW_MAXIMIZE);
    }
  }

  void Maximized(bool value) { renderer_.SetActiveGlyph(value ? 0 : 1); }

 private:
  ButtonRenderer& renderer_;
  HWND hwnd_;
};

constexpr const wchar_t* MinimizeGlyph =
    L"<svg viewBox='0 0 16 16'><path d='M3.5 7h9c.28 0 .5.22.5.5s-.22.5-.5.5h-9c-.28 "
    L"0-.5-.22-.5-.5s.22-.5.5-.5z'></path></svg>";

class MinimizeElement : public Element {
 public:
  explicit MinimizeElement(HWND hwnd, UIC::Compositor compositor, RendererColors background)
      : hwnd_{hwnd} {
    HitTest(HitTestCode::MinimizeButton);
    CreateRenderer<ButtonRenderer>(compositor, background, MinimizeGlyph);
  }

  void MouseClick(MouseButton button, const POINT&) final {
    if (button == MouseButton::Left) {
      ::CloseWindow(hwnd_);
    }
  }

 private:
  HWND hwnd_;
};

constexpr const wchar_t* CloseGlyph =
    L"<svg viewBox='0 0 16 16'><path d='M2.589 2.716l.058-.069a.498.498 0 01.637-.058l.069.058L8 "
    L"7.293l4.646-4.647a.5.5 0 01.707.707L8.707 8l4.647 4.646a.5.5 0 01.058.638l-.058.069a.5.5 0 "
    L"01-.638.058l-.069-.058L8 8.707l-4.646 4.647a.5.5 0 01-.707-.707L7.293 8 2.646 3.354a.501.501 "
    L"0 01-.057-.638l.058-.069-.058.069z'></path></svg>";

class CloseElement : public Element {
 public:
  explicit CloseElement(HWND hwnd, UIC::Compositor compositor, RendererColors background)
      : hwnd_{hwnd} {
    HitTest(HitTestCode::CloseButton);
    CreateRenderer<ButtonRenderer>(compositor, background, CloseGlyph);
  }

  void MouseClick(MouseButton button, const POINT&) final {
    if (button == MouseButton::Left) {
      THROW_IF_WIN32_BOOL_FALSE(::DestroyWindow(hwnd_));
    }
  }

 private:
  HWND hwnd_;
};

class CaptionElement : public Element {
 public:
  explicit CaptionElement(HWND hwnd, UIC::Compositor compositor, UI::Color background)
      : hwnd_{hwnd} {
    CallDefWindowProc(true);
    CreateRenderer<BackgroundRenderer>(compositor, RendererColors{background});
    HitTest(HitTestCode::Caption);
  }

  void MouseClick(MouseButton button, const POINT& pt) final {
    if (button == MouseButton::Right) {
      SystemMenu{hwnd_}.Show(HitTest(), true, pt);
    }
  }

 private:
  HWND hwnd_;
};

class SystemMenuElement : public Element {
 public:
  explicit SystemMenuElement(HWND hwnd, UIC::Compositor compositor, UI::Color background)
      : hwnd_{hwnd} {
    CreateRenderer<BackgroundRenderer>(compositor, RendererColors{background});
    HitTest(HitTestCode::SystemMenu);
  }

  void MouseDown(MouseButton button, const POINT&) final {
    if (button == MouseButton::Left) {
      ShowMenu(button);
    }
  }

  void MouseClick(MouseButton button, const POINT& point) final {
    if (button == MouseButton::Right) {
      ShowMenu(button, point);
    }
  }

  void MouseDoubleClick(MouseButton button, const POINT&) final {
    if (button == MouseButton::Left) {
      THROW_IF_WIN32_BOOL_FALSE(::DestroyWindow(hwnd_));
    }
  }

 private:
  void ShowMenu(MouseButton button, std::optional<POINT> point = std::nullopt) const {
    SystemMenu{hwnd_}.Show(HitTest(),
                           button == MouseButton::Right,
                           point.value_or(POINT{Bounds().left, Bounds().bottom}));
  }

 private:
  HWND hwnd_;
};

std::ostream& operator<<(std::ostream& stream, const RECT& rect) {
  auto width = (rect.right - rect.left);
  auto height = (rect.bottom - rect.top);
  return stream << rect.left << "," << rect.top << " " << width << "x" << height;
}

std::ostream& operator<<(std::ostream& stream, const Element& el) {
  return stream << el.HitTest() << " (" << el.Bounds() << ')';
}

std::ostream& operator<<(std::ostream& stream, const Element* el) {
  if (el) {
    return stream << *el;
  } else {
    return stream << "(none)";
  }
}

UIC::CompositionColorBrush mouse_brush = nullptr;
UIC::CompositionColorBrush mouse_down_brush = nullptr;
UIC::SpriteVisual mouse_visual = nullptr;

template <typename It>
class Range {
 public:
  Range(It begin, It end) : begin_{begin}, end_{end} {}

  It begin() const { return begin_; }
  It end() const { return end_; }

 private:
  It begin_;
  It end_;
};

class ElementSet {
 private:
  using ElementRef = std::reference_wrapper<Element>;
  using ElementRefVector = std::vector<ElementRef>;

 public:
  ElementSet() {}
  ElementSet(std::initializer_list<ElementRef> elements) : elements_{elements} {}

  auto BottomUp() const { return Range{elements_.begin(), elements_.end()}; }

  auto TopDown() const { return Range{elements_.rbegin(), elements_.rend()}; }

  Element* FindAtClientPointTopDown(const POINT& pt) const {
    for (Element& el : TopDown()) {
      if (el.Contains(pt)) {
        return &el;
      }
    }
    return nullptr;
  }

 private:
  ElementRefVector elements_;
};

class MouseStateMachine {
 public:
  explicit MouseStateMachine(ElementSet& elements) : elements_{elements} {}

  void MouseDown(Element* element, MouseButton button, const POINT& point) {
    mouse_down_element_ = element;
    mouse_down_button_ = button;
    for (Element& el : elements_.TopDown()) {
      el.MouseState(&el == element ? RendererState::MouseDown : RendererState::Normal);
    };
    if (element) {
      element->MouseDown(button, point);
    }
  }

  void MouseDoubleClick(Element* element, MouseButton button, const POINT& point) {
    MouseMove(element);
    if (element) {
      element->MouseDoubleClick(button, point);
    }
  }

  void MouseUp(Element* element, MouseButton button, const POINT& point) {
    auto same_as_mouse_down = mouse_down_element_ == element && mouse_down_button_ == button;
    mouse_down_element_ = nullptr;
    mouse_down_button_ = std::nullopt;
    MouseMove(element);

    if (element) {
      element->MouseUp(button, point);
      if (same_as_mouse_down) {
        element->MouseClick(button, point);
      }
    }
  }

  void MouseMove(Element* element) {
    if (element) {
      MouseOver(*element);
    } else {
      MouseLeave();
    }
  }

  void MouseLeave() {
    if (mouse_over_element_) {
      mouse_over_element_->MouseLeave();
      mouse_over_element_ = nullptr;
    }
    for (Element& el : elements_.TopDown()) {
      el.MouseState(RendererState::Normal);
    }
  }

 private:
  void MouseOver(Element& element) {
    if (mouse_over_element_ != &element) {
      if (mouse_over_element_) {
        mouse_over_element_->MouseLeave();
      }
      mouse_over_element_ = &element;
    }

    for (Element& el : elements_.TopDown()) {
      if (&el == &element) {
        el.MouseState(&el == mouse_down_element_ ? RendererState::MouseDown
                                                 : RendererState::MouseOver);
      } else {
        el.MouseState(RendererState::Normal);
      }
    };
  }

  const ElementSet& elements_;

  Element* mouse_down_element_ = nullptr;
  Element* mouse_over_element_ = nullptr;
  std::optional<MouseButton> mouse_down_button_;
};

ElementSet elements;
std::unique_ptr<CaptionElement> caption_el;
std::unique_ptr<SystemMenuElement> system_menu_el;
std::unique_ptr<MinimizeElement> minimize_el;
std::unique_ptr<MaximizeElement> maximize_el;
std::unique_ptr<CloseElement> close_el;

MouseStateMachine mouse_state_machine{elements};

void TrackMouseLeave(HWND hwnd, bool non_client) {
  TRACKMOUSEEVENT tme = {sizeof(tme)};
  tme.hwndTrack = hwnd;
  tme.dwFlags = TME_LEAVE;

  //
  // Problem: Because this mouse move is 'simulated' from WM_NCMOUSEMOVE calling TrackMouseEvent
  //          without the TME_NONCLIENT will generate the WM_MOUSELEAVE immediately.
  //
  //          Our window never uses HTCLIENT, so all WM_MOUSE messages are for the max button
  //          (which requires TME_NONCLIENT). If we ever used HTCLIENT in NCHITTEST this call
  //          to TrackMouseEvent would need to know if the original HT code was HTCLIENT and
  //          not use TME_NONCLIENT.
  //
  WI_SetFlagIf(tme.dwFlags, TME_NONCLIENT, non_client);

  // Call TrackMouseEvent to ensure we see a WM_MOUSELEAVE.
  THROW_IF_WIN32_BOOL_FALSE(::TrackMouseEvent(&tme));
}

void MouseLeave() {
  mouse_state_machine.MouseLeave();
  mouse_visual.Brush(nullptr);
}

void MouseMove(Element* element, const POINT& pt) {
  mouse_state_machine.MouseMove(element);
  mouse_visual.Brush(mouse_brush);
  mouse_visual.Offset({static_cast<float>(pt.x), static_cast<float>(pt.y), 0.0f});
}

void MouseDown(Element* element, MouseButton button, const POINT& pt) {
  mouse_state_machine.MouseDown(element, button, pt);
  mouse_visual.Brush(mouse_down_brush);
}

void MouseUp(Element* element, MouseButton button, const POINT& pt) {
  mouse_state_machine.MouseUp(element, button, pt);
  mouse_visual.Brush(mouse_brush);
}

void MouseDoubleClick(Element* element, MouseButton button, const POINT& pt) {
  mouse_state_machine.MouseDoubleClick(element, button, pt);
}

void LayoutElements(const RECT& rcClient, uint32_t dpi) {
  int kButtonHeight = MulDiv(47, dpi, 96);
  int kButtonWidth = MulDiv(44, dpi, 96);

  RECT rcTop = rcClient;
  rcTop.bottom = kButtonHeight;
  caption_el->Bounds(rcTop);

  RECT rcIcon = rcTop;
  rcIcon.right = rcTop.left + kButtonWidth;
  system_menu_el->Bounds(rcIcon);

  RECT rcClose = rcTop;
  rcClose.left = rcTop.right - kButtonWidth;
  close_el->Bounds(rcClose);

  RECT rcMaxButton = rcTop;
  rcMaxButton.left = rcClose.left - kButtonWidth;
  rcMaxButton.right = rcClose.left;
  maximize_el->Bounds(rcMaxButton);

  RECT rcMinButton = rcTop;
  rcMinButton.left = rcMaxButton.left - kButtonWidth;
  rcMinButton.right = rcMaxButton.left;
  minimize_el->Bounds(rcMinButton);

  for (Element& element : elements.BottomUp()) {
    element.SetDpi(dpi);
  }
}

UIC::SpriteVisual CreateMouseVisual() {
  auto ellipse = compositor.CreateEllipseGeometry();
  ellipse.Radius({8, 8});
  ellipse.Center({8, 8});

  auto clip = compositor.CreateGeometricClip(ellipse);
  auto sprite = compositor.CreateSpriteVisual();
  sprite.AnchorPoint({0.5, 0.5});
  sprite.Brush(mouse_brush);
  sprite.Size({16, 16});
  sprite.Clip(clip);
  return sprite;
}

void CreateRenderers(UIC::ContainerVisual container, HWND hwnd) {
  caption_el = std::make_unique<CaptionElement>(hwnd, container.Compositor(), UI::Colors::Aqua());

  system_menu_el =
      std::make_unique<SystemMenuElement>(hwnd, container.Compositor(), UI::Colors::BlueViolet());

  minimize_el = std::make_unique<MinimizeElement>(
      hwnd,
      container.Compositor(),
      RendererColors{
          UI::Colors::Transparent(), UI::Colors::NavajoWhite(), UI::Colors::LightGray()});

  maximize_el = std::make_unique<MaximizeElement>(
      hwnd,
      container.Compositor(),
      RendererColors{
          UI::Colors::Transparent(), UI::Colors::NavajoWhite(), UI::Colors::LightGray()});

  close_el = std::make_unique<CloseElement>(
      hwnd,
      container.Compositor(),
      RendererColors{UI::Colors::Transparent(), UI::Colors::Red(), UI::Colors::DarkRed()});

  elements = ElementSet{*caption_el, *system_menu_el, *minimize_el, *maximize_el, *close_el};

  for (Element& element : elements.BottomUp()) {
    if (element.Visual()) {
      container.Children().InsertAtTop(element.Visual());
    }
  }

  mouse_brush = compositor.CreateColorBrush(UI::Colors::DarkRed());
  mouse_down_brush = compositor.CreateColorBrush(UI::Colors::Yellow());
  mouse_visual = CreateMouseVisual();
  container.Children().InsertAtTop(mouse_visual);
}

void LogMessage(bool nc, const char* message, Element* element) {
  std::cout << (nc ? "NC " : "   ") << message << " element=" << element << '\n';
}

bool IsClientMouseMessage(uint32_t message) {
  return message >= WM_MOUSEFIRST && message <= WM_MOUSELAST;
}

bool IsNonClientMouseMessage(uint32_t message) {
  return message >= WM_NCMOUSEMOVE && message <= WM_NCXBUTTONDBLCLK;
}

void ConvertToClientMouseMessage(HWND hwnd, uint32_t message, POINT point) {
  LPARAM lparam = MAKELPARAM(point.x, point.y);

  WPARAM wparam = 0;
  WI_SetFlagIf(wparam, MK_CONTROL, ::GetKeyState(VK_CONTROL) != 0);
  WI_SetFlagIf(wparam, MK_LBUTTON, ::GetKeyState(VK_LBUTTON) != 0);
  WI_SetFlagIf(wparam, MK_MBUTTON, ::GetKeyState(VK_MBUTTON) != 0);
  WI_SetFlagIf(wparam, MK_RBUTTON, (::GetKeyState(VK_RBUTTON) | ::GetKeyState(VK_SHIFT)) != 0);
  WI_SetFlagIf(wparam, MK_XBUTTON1, ::GetKeyState(VK_XBUTTON1) != 0);
  WI_SetFlagIf(wparam, MK_XBUTTON2, ::GetKeyState(VK_XBUTTON2) != 0);

  constexpr int nc_to_client = (WM_NCMOUSEMOVE - WM_MOUSEMOVE);
  ::SendMessageW(hwnd, message - nc_to_client, wparam, lparam);
}

LRESULT HandleMouseMessage(HWND hwnd, uint32_t message, WPARAM wparam, LPARAM lparam) {
  POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
  bool is_nc_message = IsNonClientMouseMessage(message);
  HitTestCode hit_test_code = HitTestCode::Client;

  if (is_nc_message) {
    THROW_IF_WIN32_BOOL_FALSE(::ScreenToClient(hwnd, &point));
    hit_test_code = static_cast<HitTestCode>(wparam);
  }

  Element* element = elements.FindAtClientPointTopDown(point);
  switch (message) {
    case WM_NCMOUSEMOVE:
    case WM_MOUSEMOVE:
      MouseMove(element, point);
      TrackMouseLeave(hwnd, is_nc_message);
      break;

    case WM_NCMOUSELEAVE:
    case WM_MOUSELEAVE:
      MouseLeave();
      break;

    case WM_NCLBUTTONDOWN:
    case WM_LBUTTONDOWN:
      MouseDown(element, MouseButton::Left, point);
      break;

    case WM_NCLBUTTONUP:
    case WM_LBUTTONUP:
      MouseUp(element, MouseButton::Left, point);
      break;

    case WM_NCLBUTTONDBLCLK:
    case WM_LBUTTONDBLCLK:
      MouseDoubleClick(element, MouseButton::Left, point);
      break;

    case WM_NCRBUTTONDOWN:
    case WM_RBUTTONDOWN:
      MouseDown(element, MouseButton::Right, point);
      break;

    case WM_NCRBUTTONUP:
    case WM_RBUTTONUP:
      MouseUp(element, MouseButton::Right, point);
      break;

    case WM_NCRBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
      MouseDoubleClick(element, MouseButton::Right, point);
      break;
  }

  if (IsSizeHitTestCode(hit_test_code) || !element || element->CallDefWindowProc()) {
    // Problem: Handle these messages (do NOT call DefWindowProc).
    //          Default handling for NC messages with the caption button HT codes (like MAXBUTTON)
    //          have various side effects we do not want (because we're handling the input over
    //          these areas).
    //
    // Notably WM_NCLBUTTONDOWN with HTMAXBUTTON will show an old bitmap of the
    // 'depressed' maximize button in the default location. See
    // xxxDCETrackCaptionButton/ xxxTrackCaptionButton.
    //
    // Note: We MUST pass HTCAPTION and resize borders to DefWindowProc (or else we'd
    // break moving and resizing).
    return ::DefWindowProcW(hwnd, message, wparam, lparam);
  } else {
    return 0;
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
#define LOG_MESSAGE(message)                                                          \
  case (message):                                                                     \
    printf(#message " wParam=%08x lParam=%08x\n", (uint32_t)wParam, (int32_t)lParam); \
    break;

  switch (msg) {
    LOG_MESSAGE(WM_SYSCOMMAND);
    LOG_MESSAGE(WM_CONTEXTMENU);
    LOG_MESSAGE(WM_ENTERMENULOOP);
    // LOG_MESSAGE(WM_MOUSEMOVE);
    LOG_MESSAGE(WM_MOUSELEAVE);
    LOG_MESSAGE(WM_LBUTTONDOWN);
    LOG_MESSAGE(WM_LBUTTONUP);
    LOG_MESSAGE(WM_LBUTTONDBLCLK);
    LOG_MESSAGE(WM_RBUTTONDOWN);
    LOG_MESSAGE(WM_RBUTTONUP);
    LOG_MESSAGE(WM_RBUTTONDBLCLK);
    LOG_MESSAGE(WM_NCMOUSEMOVE);
    LOG_MESSAGE(WM_NCMOUSELEAVE);
    LOG_MESSAGE(WM_NCLBUTTONDOWN);
    LOG_MESSAGE(WM_NCLBUTTONUP);
    LOG_MESSAGE(WM_NCLBUTTONDBLCLK);
    LOG_MESSAGE(WM_NCRBUTTONDOWN);
    LOG_MESSAGE(WM_NCRBUTTONUP);
    LOG_MESSAGE(WM_NCRBUTTONDBLCLK);
  }
#undef LOG_MESSAGE

  switch (msg) {
    case WM_SIZE: {
      RECT rcClient;
      ::GetClientRect(hwnd, &rcClient);
      const UINT dpi = ::GetDpiForWindow(hwnd);
      LayoutElements(rcClient, dpi);
      maximize_el->Maximized(wParam == SIZE_MAXIMIZED);
      break;
    }

    case WM_NCHITTEST: {
      // Get the default HT code and only handle if HTCLIENT.
      UINT ht = (UINT)::DefWindowProcW(hwnd, msg, wParam, lParam);

      if (ht >= HTSIZEFIRST && ht <= HTSIZELAST) {
        // This allows the left/right/bottom resize HT codes to flow through.
        return ht;
      }

      POINT ptClient = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      ::ScreenToClient(hwnd, &ptClient);

      // Top resize
      if (ptClient.y < MulDiv(8, GetDpiForWindow(hwnd), 96)) {
        return HTTOP;
      }

      auto element = elements.FindAtClientPointTopDown(ptClient);
      if (element) {
        std::cout << "WM_NCHITTEST element=" << element << " result=" << element->HitTest() << '\n';
        return static_cast<uint32_t>(element->HitTest());
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
      //          caption bar (while still marking the window with all the styles/
      //          WS_OVERLAPPEDWINDOW).
      //
      //          Note: By not touching the left/right/bottom we retain our resize borders.
      //

    case WM_NCCALCSIZE: {
      RECT* prc = (PRECT)lParam;
      auto top = prc->top;
      auto lRet = ::DefWindowProcW(hwnd, msg, wParam, lParam);
      prc->top = top - 1;
      return lRet;
    }

    // On WM_ACTIVATE set isActive and repaint.
    case WM_ACTIVATE:
      // isActive = (wParam != WA_INACTIVE);
      break;

    case WM_CREATE: {
      using namespace winrt::Windows::System;

      DispatcherQueueOptions options{sizeof(DispatcherQueueOptions)};
      options.apartmentType = DQTAT_COM_ASTA;
      options.threadType = DQTYPE_THREAD_CURRENT;

      DispatcherQueueController controller{nullptr};
      winrt::check_hresult(::CreateDispatcherQueueController(
          options,
          reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(
              winrt::put_abi(controller))));

      compositor = UIC::Compositor();
      auto interop = compositor.as<UIC::abi::Desktop::ICompositorDesktopInterop>();
      winrt::check_hresult(interop->CreateDesktopWindowTarget(
          hwnd,
          FALSE,
          reinterpret_cast<UIC::abi::Desktop::IDesktopWindowTarget**>(winrt::put_abi(target))));

      root = compositor.CreateContainerVisual();
      target.Root(root);
      SetBackdrop(target);
      CreateRenderers(root, hwnd);

      THROW_IF_FAILED(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
      THROW_IF_FAILED(::CreateCoreWebView2Environment(
          Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
              [hwnd](HRESULT hr, ICoreWebView2Environment* env) {
                THROW_IF_FAILED(hr);
                RETURN_IF_FAILED(env->CreateCoreWebView2Controller(
                    hwnd,
                    Microsoft::WRL::Callback<
                        ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT hr, ICoreWebView2Controller* controller) {
                          THROW_IF_FAILED(hr);
                          RECT rect{50, 50, 250, 250};
                          THROW_IF_FAILED(controller->put_Bounds(rect));
                          controller->put_IsVisible(TRUE);
                          controller->AddRef();
                          return S_OK;
                        })
                        .Get()));
                return S_OK;
              })
              .Get()));

      UINT dpi = ::GetDpiForWindow(hwnd);
      SIZE sz = {::MulDiv(szInitial.cx, dpi, 96), ::MulDiv(szInitial.cy, dpi, 96)};
      ::SetWindowPos(hwnd, nullptr, 150, 300, sz.cx, sz.cy, SWP_SHOWWINDOW);
      break;
    }

    // Destroy the window on escape key
    case WM_CHAR:
      if (wParam == VK_ESCAPE) {
        ::DestroyWindow(hwnd);
      }
      break;

    case WM_DPICHANGED: {
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

  if (IsClientMouseMessage(msg) || IsNonClientMouseMessage(msg)) {
    return HandleMouseMessage(hwnd, msg, wParam, lParam);
  }

  return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void InitWindow(HINSTANCE hInst) {
  PCWSTR wndClassName = L"WndClass";
  PCWSTR windowTitle = L"BigMaxButton";

  WNDCLASSEX wc = {0};
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

int __stdcall wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ wchar_t*, _In_ int) {
  ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  InitWindow(hInstance);

  MSG msg;
  while (::GetMessageW(&msg, nullptr, 0, 0)) {
    ::TranslateMessage(&msg);
    ::DispatchMessageW(&msg);
  }

  return 0;
}

int wmain(int, wchar_t*[]) {
  wchar_t str[] = L"";
  return wWinMain(::GetModuleHandleW(nullptr), nullptr, str, SW_NORMAL);
}
