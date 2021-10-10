/******************************************************************************
        windows.ui.composition.Mica.h

        Copyright (c) Microsoft Corporation. All rights reserved.

******************************************************************************/
#pragma once
#include <Windows.Foundation.Numerics.h>
#include <Windows.Foundation.h>
#include <Windows.Graphics.Effects.Interop.h>
#include <Windows.Graphics.Effects.h>
#include <Windows.UI.Composition.h>
#include <Windows.UI.h>
#include <d2d1_1.h>
#include <d2d1effects_2.h>
#include <wrl.h>
#include <cstring>

#pragma push_macro("MIDL_CONST_ID")
#undef MIDL_CONST_ID
#define MIDL_CONST_ID const __declspec(selectany)

#pragma warning(push)
#pragma warning(disable : 28204)

/*
 *
 * Interface Windows.UI.Composition.ICompositionBackdropBrush
 *
 * Introduced to Windows.Foundation.UniversalApiContract in version 3.0
 *
 * Interface is a part of the implementation of type Windows.UI.Composition.CompositionBackdropBrush
 *
 */
 //#if WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0x30000
#if !defined(____x_ABI_CWindows_CUI_CComposition_CICompositionBackdropBrush_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CUI_CComposition_CICompositionBackdropBrush_INTERFACE_DEFINED__
extern const __declspec(selectany)
_Null_terminated_ WCHAR InterfaceName_Windows_UI_Composition_ICompositionBackdropBrush[] =
L"Windows.UI.Composition.ICompositionBackdropBrush";
namespace ABI {
    namespace Windows {
        namespace UI {
            namespace Composition {
                MIDL_INTERFACE("c5acae58-3898-499e-8d7f-224e91286a5d")
                    ICompositionBackdropBrush : public IInspectable{ public: };

                extern MIDL_CONST_ID IID& IID_ICompositionBackdropBrush = _uuidof(ICompositionBackdropBrush);
            }  // namespace Composition
        }  // namespace UI
    }  // namespace Windows
}  // namespace ABI

EXTERN_C const IID IID___x_ABI_CWindows_CUI_CComposition_CICompositionBackdropBrush;
#endif /* !defined(____x_ABI_CWindows_CUI_CComposition_CICompositionBackdropBrush_INTERFACE_DEFINED__) \
        */
//#endif // WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0x30000

/*
 *
 * Interface Windows.UI.Composition.ICompositorWithBlurredWallpaperBackdropBrush
 *
 * Introduced to Windows.Foundation.UniversalApiContract in version 13.0
 *
 * Type is for evaluation purposes and is subject to change or removal in future updates.
 *
 * Interface is a part of the implementation of type Windows.UI.Composition.Compositor
 *
 */
 //#if defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)
 //#if WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0xd0000
#if !defined( \
    ____x_ABI_CWindows_CUI_CComposition_CICompositorWithBlurredWallpaperBackdropBrush_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CUI_CComposition_CICompositorWithBlurredWallpaperBackdropBrush_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR
InterfaceName_Windows_UI_Composition_ICompositorWithBlurredWallpaperBackdropBrush[] =
L"Windows.UI.Composition.ICompositorWithBlurredWallpaperBackdropBrush";
namespace ABI {
    namespace Windows {
        namespace UI {
            namespace Composition {
                MIDL_INTERFACE("0d8fb190-f122-5b8d-9fdd-543b0d8eb7f3")
                    ICompositorWithBlurredWallpaperBackdropBrush : public IInspectable{
                     public:
                      virtual HRESULT STDMETHODCALLTYPE TryCreateBlurredWallpaperBackdropBrush(
                          ABI::Windows::UI::Composition::ICompositionBackdropBrush * *result) = 0;
                };

                extern MIDL_CONST_ID IID& IID_ICompositorWithBlurredWallpaperBackdropBrush =
                    _uuidof(ICompositorWithBlurredWallpaperBackdropBrush);
            }  // namespace Composition
        }  // namespace UI
    }  // namespace Windows
}  // namespace ABI

EXTERN_C const IID
IID___x_ABI_CWindows_CUI_CComposition_CICompositorWithBlurredWallpaperBackdropBrush;
#endif /* !defined(____x_ABI_CWindows_CUI_CComposition_CICompositorWithBlurredWallpaperBackdropBrush_INTERFACE_DEFINED__) \
        */
//#endif // WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0xd0000
//#endif // defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)

/*
 *
 * Interface Windows.UI.Composition.ICompositionSupportsSystemBackdropImplementation
 *
 * Introduced to Windows.Foundation.UniversalApiContract in version 13.0
 *
 */
 //#if WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0xd0000
#if !defined( \
    ____x_ABI_CWindows_CUI_CComposition_CICompositionSupportsSystemBackdrop_INTERFACE_DEFINED__)
#define ____x_ABI_CWindows_CUI_CComposition_CICompositionSupportsSystemBackdrop_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR
InterfaceName_Windows_UI_Composition_ICompositionSupportsSystemBackdrop[] =
L"Windows.UI.Composition.ICompositionSupportsSystemBackdrop";
namespace ABI {
    namespace Windows {
        namespace UI {
            namespace Composition {
                MIDL_INTERFACE("397DAFE4-B6C2-5BB9-951D-F5707DE8B7BC")
                    ICompositionSupportsSystemBackdrop : public IInspectable{
                     public:
                      virtual HRESULT STDMETHODCALLTYPE get_SystemBackdrop(
                          ABI::Windows::UI::Composition::ICompositionBrush * *value) = 0;
                      virtual HRESULT STDMETHODCALLTYPE put_SystemBackdrop(
                          ABI::Windows::UI::Composition::ICompositionBrush* value) = 0;
                };

                extern MIDL_CONST_ID IID& IID_ICompositionSupportsSystemBackdrop =
                    _uuidof(ICompositionSupportsSystemBackdrop);
            }  // namespace Composition
        }  // namespace UI
    }  // namespace Windows
}  // namespace ABI

EXTERN_C const IID IID___x_ABI_CWindows_CUI_CComposition_CICompositionSupportsSystemBackdrop;
#endif /* !defined(____x_ABI_CWindows_CUI_CComposition_CICompositionSupportsSystemBackdrop_INTERFACE_DEFINED__) \
        */
//#endif // WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0xd0000

//---------------- Effects ---

/* TODO: We are missing proper runtime class ID strings for the Effects we want to apply for Mica.
   We also need IGraphicsEffectSource. They will look something like the following. There's a
   discrepency between 'Microsof' and 'Windows' in the namespaces we need to resolve.

         #ifndef RUNTIMECLASS_Microsoft_UI_Composition_Effects_OpacityEffect_DEFINED
         #define RUNTIMECLASS_Microsoft_UI_Composition_Effects_OpacityEffect_DEFINED
         extern const __declspec(selectany) _Null_terminated_ WCHAR
   RuntimeClass_Microsoft_UI_Composition_Effects_OpacityEffect[] =
   L"Microsoft.UI.Composition.Effects.OpacityEffect"; #endif
 */

namespace ABI {
    namespace Windows {
        namespace UI {
            namespace Composition {
                namespace Effects {

                    typedef enum class BlendEffectMode {
                        BlendEffectMode_Multiply = 0,
                        BlendEffectMode_Screen = 1,
                        BlendEffectMode_Darken = 2,
                        BlendEffectMode_Lighten = 3,
                        BlendEffectMode_Dissolve = 4,
                        BlendEffectMode_ColorBurn = 5,
                        BlendEffectMode_LinearBurn = 6,
                        BlendEffectMode_DarkerColor = 7,
                        BlendEffectMode_LighterColor = 8,
                        BlendEffectMode_ColorDodge = 9,
                        BlendEffectMode_LinearDodge = 10,
                        BlendEffectMode_Overlay = 11,
                        BlendEffectMode_SoftLight = 12,
                        BlendEffectMode_HardLight = 13,
                        BlendEffectMode_VividLight = 14,
                        BlendEffectMode_LinearLight = 15,
                        BlendEffectMode_PinLight = 16,
                        BlendEffectMode_HardMix = 17,
                        BlendEffectMode_Difference = 18,
                        BlendEffectMode_Exclusion = 19,
                        BlendEffectMode_Hue = 20,
                        BlendEffectMode_Saturation = 21,
                        BlendEffectMode_Color = 22,
                        BlendEffectMode_Luminosity = 23,
                        BlendEffectMode_Subtract = 24,
                        BlendEffectMode_Division = 25
                    } BlendEffectMode;

                    MIDL_INTERFACE("5673248E-7266-5E49-B2AB-2589D5D875C3")
                        IBlendEffect : IInspectable{
                          virtual HRESULT STDMETHODCALLTYPE get_Mode(BlendEffectMode * value) = 0;
                          virtual HRESULT STDMETHODCALLTYPE put_Mode(BlendEffectMode value) = 0;
                          virtual HRESULT STDMETHODCALLTYPE get_Background(
                              ::ABI::Windows::Graphics::Effects::IGraphicsEffectSource** source) = 0;
                          virtual HRESULT STDMETHODCALLTYPE put_Background(
                              ::ABI::Windows::Graphics::Effects::IGraphicsEffectSource* source) = 0;
                          virtual HRESULT STDMETHODCALLTYPE get_Foreground(
                              ::ABI::Windows::Graphics::Effects::IGraphicsEffectSource** source) = 0;
                          virtual HRESULT STDMETHODCALLTYPE put_Foreground(
                              ::ABI::Windows::Graphics::Effects::IGraphicsEffectSource* source) = 0;
                    };

                    MIDL_INTERFACE("25F942C7-7FEE-518A-BA7B-22A0060AF7F6")
                        IColorSourceEffect : IInspectable{
                          virtual HRESULT STDMETHODCALLTYPE get_Color(::ABI::Windows::UI::Color * value) = 0;
                          virtual HRESULT STDMETHODCALLTYPE put_Color(::ABI::Windows::UI::Color value) = 0;
                    };

                    // MIDL_INTERFACE("94B6AD75-C540-51B8-A9D1-544174ADC68D")  // GUID used in Edge
                    MIDL_INTERFACE("C20AE33A-1844-4650-811C-63CA823B86B6")  // GUID in Windows depot
                        IOpacityEffect : IInspectable{
                          virtual HRESULT STDMETHODCALLTYPE get_Opacity(float* value) = 0;
                          virtual HRESULT STDMETHODCALLTYPE put_Opacity(float value) = 0;
                          virtual HRESULT STDMETHODCALLTYPE get_Source(
                              ::ABI::Windows::Graphics::Effects::IGraphicsEffectSource** source) = 0;
                          virtual HRESULT STDMETHODCALLTYPE put_Source(
                              ::ABI::Windows::Graphics::Effects::IGraphicsEffectSource* source) = 0;
                    };

                    // Base class for Win2D-like effect descriptions
                    template <typename TEffectInterface>
                    class EffectBase abstract
                        : public Microsoft::WRL::RuntimeClass<
                        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
                        Windows::Graphics::Effects::IGraphicsEffect,
                        Windows::Graphics::Effects::IGraphicsEffectSource,
                        Windows::Graphics::Effects::IGraphicsEffectD2D1Interop,
                        TEffectInterface> {
                    protected:
                        // This is a header file so we can't use "using namespace", but we can do this:
                        typedef Windows::UI::Color UIColor;  // Renamed because we use "Color" as a field name
                        typedef Windows::Foundation::IPropertyValue IPropertyValue;
                        typedef Windows::Foundation::IPropertyValueStatics IPropertyValueStatics;
                        typedef Windows::Foundation::Numerics::Vector2 Vector2;
                        typedef Windows::Foundation::Numerics::Vector3 Vector3;
                        typedef Windows::Foundation::Numerics::Matrix3x2 Matrix3x2;
                        typedef Windows::Graphics::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING
                            GRAPHICS_EFFECT_PROPERTY_MAPPING;
                        typedef Windows::Graphics::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING PropertyMapping;
                        typedef Windows::Graphics::Effects::IGraphicsEffectSource IGraphicsEffectSource;

                    public:
                        // IGraphicsEffect
                        IFACEMETHODIMP get_Name(_Out_ HSTRING* name) override { return Name.CopyTo(name); }
                        IFACEMETHODIMP put_Name(_In_ HSTRING name) override { return Name.Set(name); }

                        // IGraphicsEffectD2D1Interop
                        IFACEMETHODIMP GetSourceCount(_Out_ UINT* count) override {
                            *count = 0;
                            return S_OK;
                        }
                        IFACEMETHODIMP GetPropertyCount(_Out_ UINT* count) override {
                            *count = 0;
                            return S_OK;
                        }

                        IFACEMETHODIMP GetSource(UINT, _Outptr_result_maybenull_ IGraphicsEffectSource**) override {
                            return E_INVALIDARG;
                        }

                        IFACEMETHODIMP GetProperty(UINT, _Outptr_ IPropertyValue**) override { return E_INVALIDARG; }

                        IFACEMETHODIMP GetNamedPropertyMapping(_In_z_ LPCWSTR,
                            _Out_ UINT*,
                            _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING*) override {
                            return E_INVALIDARG;
                        }

                    protected:
                        // Invokes a functor with the pointer to the property factory
                        template <typename TFunc>
                        static HRESULT UsePropertyFactory(const TFunc& func) {
                            Microsoft::WRL::ComPtr<IPropertyValueStatics> propertyValueFactory;
                            Microsoft::WRL::Wrappers::HStringReference activatableClassId{
                                RuntimeClass_Windows_Foundation_PropertyValue };
                            HRESULT hr = GetActivationFactory(activatableClassId.Get(), &propertyValueFactory);
                            return FAILED(hr) ? hr : func(propertyValueFactory.Get());
                        }

                        template <UINT32 ComponentCount>
                        static HRESULT CreateColor(_In_ IPropertyValueStatics* statics,
                            UIColor color,
                            _Outptr_ IPropertyValue** value) {
                            static_assert(ComponentCount == 3 || ComponentCount == 4, "Unexpected color component count.");
                            float values[] = { color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f };
                            Microsoft::WRL::ComPtr<IInspectable> valueInspectable;
                            return statics->CreateSingleArray(ComponentCount, values, (IInspectable**)value);
                        }

                        // Make a bool not a compile-time constant to avoid compiler/OACR warnings
                        static bool Passthrough(bool value) { return value; }

                        // Helpers to implement GetNamedPropertyMapping more succintly
                        struct NamedProperty {
                            const wchar_t* Name;  // Compile-time constant
                            UINT Index;           // Property index
                            GRAPHICS_EFFECT_PROPERTY_MAPPING Mapping;
                        };

                        HRESULT GetNamedPropertyMappingImpl(_In_count_(namedPropertyCount)
                            const NamedProperty* namedProperties,
                            UINT namedPropertyCount,
                            _In_z_ LPCWSTR name,
                            _Out_ UINT* index,
                            _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) {
                            for (UINT i = 0; i < namedPropertyCount; ++i) {
                                const auto& prop = namedProperties[i];
                                if (_wcsicmp(name, prop.Name) == 0) {
                                    *index = prop.Index;
                                    *mapping = prop.Mapping;
                                    return S_OK;
                                }
                            }
                            return E_INVALIDARG;
                        }

                        // M_PI requires us to be the first to include math.h, not worth it
                        static constexpr float k_PI = 3.14159265358979f;
                        static constexpr float k_DegreesPerRadian = 180.0f / k_PI;

                    public:
                        Microsoft::WRL::Wrappers::HString Name;
                    };

                    //-----------------------------------------------------------------------------------------------------------------
                    // Helper macros to make implementation more succint

#pragma push_macro("DECLARE_D2D_GUID")
#undef DECLARE_D2D_GUID
#define DECLARE_D2D_GUID(Guid)                          \
  IFACEMETHODIMP GetEffectId(_Out_ GUID* id) override { \
    *id = Guid;                                         \
    return S_OK;                                        \
  }

#pragma push_macro("DECLARE_POD_PROPERTY")
#undef DECLARE_POD_PROPERTY
#define DECLARE_POD_PROPERTY(Name, Type, InitialValue, Condition) \
 private:                                                         \
  Type Name = InitialValue;                                       \
                                                                  \
 public:                                                          \
  IFACEMETHODIMP get_##Name(_Out_ Type* value) override {         \
    *value = Name;                                                \
    return S_OK;                                                  \
  }                                                               \
  IFACEMETHODIMP put_##Name(Type value) override {                \
    if (!Passthrough(Condition)) {                                \
      return E_INVALIDARG;                                        \
    }                                                             \
    Name = value;                                                 \
    return S_OK;                                                  \
  }

#pragma push_macro("DECLARE_SOURCE")
#undef DECLARE_SOURCE
#define DECLARE_SOURCE(Name)                                                                    \
  Microsoft::WRL::ComPtr<IGraphicsEffectSource> Name;                                           \
  IFACEMETHODIMP get_##Name(_Outptr_result_maybenull_ IGraphicsEffectSource** value) override { \
    return Name.CopyTo(value);                                                                  \
  }                                                                                             \
  IFACEMETHODIMP put_##Name(_In_ IGraphicsEffectSource* value) override {                       \
    Name = value;                                                                               \
    return S_OK;                                                                                \
  }

#pragma push_macro("DECLARE_SINGLE_SOURCE")
#undef DECLARE_SINGLE_SOURCE
#define DECLARE_SINGLE_SOURCE(Name)                                                              \
  DECLARE_SOURCE(Name)                                                                           \
  IFACEMETHODIMP GetSourceCount(_Out_ UINT* count) override {                                    \
    *count = 1;                                                                                  \
    return S_OK;                                                                                 \
  }                                                                                              \
  IFACEMETHODIMP GetSource(UINT index, _Outptr_result_maybenull_ IGraphicsEffectSource** source) \
      override {                                                                                 \
    return index == 0 ? Name.CopyTo(source) : E_INVALIDARG;                                      \
  }

#pragma push_macro("DECLARE_DUAL_SOURCES")
#undef DECLARE_DUAL_SOURCES
#define DECLARE_DUAL_SOURCES(Name1, Name2)                                                       \
  DECLARE_SOURCE(Name1)                                                                          \
  DECLARE_SOURCE(Name2)                                                                          \
  IFACEMETHODIMP GetSourceCount(_Out_ UINT* count) override {                                    \
    *count = 2;                                                                                  \
    return S_OK;                                                                                 \
  }                                                                                              \
  IFACEMETHODIMP GetSource(UINT index, _Outptr_result_maybenull_ IGraphicsEffectSource** source) \
      override {                                                                                 \
    return index == 0 ? Name1.CopyTo(source) : index == 1 ? Name2.CopyTo(source) : E_INVALIDARG; \
  }

#pragma push_macro("DECLARE_NAMED_PROPERTY_MAPPING")
#undef DECLARE_NAMED_PROPERTY_MAPPING
#define DECLARE_NAMED_PROPERTY_MAPPING(...)                                                    \
  IFACEMETHODIMP GetNamedPropertyMapping(                                                      \
      _In_z_ LPCWSTR name, _Out_ UINT* index, _Out_ GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) \
      override {                                                                               \
    static const NamedProperty s_Properties[] = {__VA_ARGS__};                                 \
    return GetNamedPropertyMappingImpl(                                                        \
        s_Properties, _countof(s_Properties), name, index, mapping);                           \
  }
//----------------------------------------------------

                    class BlendEffect WrlFinal : public EffectBase<IBlendEffect> {
                        InspectableClass(L"RuntimeClass_Microsoft_UI_Composition_Effects_BlendEffect", BaseTrust);

                    public:
                        DECLARE_D2D_GUID(CLSID_D2D1Blend);
                        DECLARE_DUAL_SOURCES(Background, Foreground);
                        DECLARE_POD_PROPERTY(Mode, BlendEffectMode, BlendEffectMode::BlendEffectMode_Multiply, true);
                        DECLARE_NAMED_PROPERTY_MAPPING({ L"Mode",
                                                        D2D1_BLEND_PROP_MODE,
                                                        PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

                    public:
                        IFACEMETHODIMP GetPropertyCount(_Out_ UINT* count) override {
                            *count = 1;
                            return S_OK;
                        }

                        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ IPropertyValue** value) override {
                            return UsePropertyFactory([=](IPropertyValueStatics* statics) {
                                switch (index) {
                                case D2D1_BLEND_PROP_MODE:
                                    return statics->CreateUInt32(static_cast<UINT32>(Mode), (IInspectable**)value);
                                default:
                                    return E_INVALIDARG;
                                }
                                });
                        }
                    };

                    class ColorSourceEffect WrlFinal : public EffectBase<IColorSourceEffect> {
                        InspectableClass(L"RuntimeClass_Microsoft_UI_Composition_Effects_ColorSourceEffect", BaseTrust);

                    public:
                        DECLARE_D2D_GUID(CLSID_D2D1Flood);
                        DECLARE_POD_PROPERTY(Color, UIColor, (UIColor{ 255, 0, 0, 0 }), true);
                        DECLARE_NAMED_PROPERTY_MAPPING(
      { L"Color",
       D2D1_FLOOD_PROP_COLOR,
       PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR4 });

                    public:
                        IFACEMETHODIMP GetPropertyCount(_Out_ UINT* count) override {
                            *count = 1;
                            return S_OK;
                        }

                        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ IPropertyValue** value) override {
                            return UsePropertyFactory([=](IPropertyValueStatics* statics) {
                                switch (index) {
                                case D2D1_FLOOD_PROP_COLOR:
                                    return CreateColor<4>(statics, Color, value);
                                default:
                                    return E_INVALIDARG;
                                }
                                });
                        }
                    };

                    class OpacityEffect WrlFinal : public EffectBase<IOpacityEffect> {
                        InspectableClass(L"RuntimeClass_Microsoft_UI_Composition_Effects_OpacityEffect", BaseTrust);

                    public:
                        DECLARE_D2D_GUID(CLSID_D2D1Opacity);
                        DECLARE_SINGLE_SOURCE(Source);
                        DECLARE_POD_PROPERTY(Opacity, float, 1.0f, value >= 0.0f && value <= 1.0f);
                        DECLARE_NAMED_PROPERTY_MAPPING({ L"Opacity",
                                                        D2D1_OPACITY_PROP_OPACITY,
                                                        PropertyMapping::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT });

                    public:
                        IFACEMETHODIMP GetPropertyCount(_Out_ UINT* count) override {
                            *count = 1;
                            return S_OK;
                        }

                        IFACEMETHODIMP GetProperty(UINT index, _Outptr_ IPropertyValue** value) override {
                            return UsePropertyFactory([=](IPropertyValueStatics* statics) {
                                switch (index) {
                                case D2D1_OPACITY_PROP_OPACITY:
                                    return statics->CreateSingle(Opacity, (IInspectable**)value);
                                default:
                                    return E_INVALIDARG;
                                }
                                });
                        }
                    };

                }  // namespace Effects
            }  // namespace Composition
        }  // namespace UI
    }  // namespace Windows
}  // namespace ABI

#pragma warning(pop)
