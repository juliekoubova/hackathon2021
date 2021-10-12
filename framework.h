// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
// Windows Header Files
#include <DispatcherQueue.h>
#include <WebView2.h>
#include <unknwn.h>
#include <wil/cppwinrt.h>
#include <wil/result.h>
#include <windows.h>
#include <windows.ui.composition.interop.h>
#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/base.h>
#include <wrl.h>
#include "Windows.UI.Composition.Mica.h"
#include "winrt/Microsoft.Graphics.Canvas.Svg.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.Composition.h"
// C RunTime Header Files
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <tchar.h>
#include <functional>
#include <sstream>
