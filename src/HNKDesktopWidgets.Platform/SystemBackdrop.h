#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace mw::platform {

	struct SystemBackdropResult {
		bool supported = false;
		bool active = false;
	};

	// Windows 11 22H2 (build 22621) or later, with transparency enabled.
	bool IsDesktopAcrylicSupported();

	// Reports whether this OS allows a translucent backdrop, and clears DWM's own
	// system backdrop so the composition blur behind the swap chain stays visible.
	SystemBackdropResult ApplyDesktopAcrylic(HWND window, bool enabled);

}
