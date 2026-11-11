#include "SystemBackdrop.h"

#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace mw::platform {
	namespace {

		using RtlGetVersionPtr = LONG(WINAPI*)(OSVERSIONINFOW*);

		bool TransparencyEnabled() {
			DWORD transparency = 1;
			DWORD size = sizeof(transparency);
			const LSTATUS status = RegGetValueW(
			    HKEY_CURRENT_USER,
			    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
			    L"EnableTransparency",
			    RRF_RT_REG_DWORD,
			    nullptr,
			    &transparency,
			    &size);
			return status != ERROR_SUCCESS || transparency != 0;
		}

	}

	bool IsDesktopAcrylicSupported() {
		const auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
		    GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
		if (rtlGetVersion == nullptr) {
			return false;
		}

		OSVERSIONINFOW version{};
		version.dwOSVersionInfoSize = sizeof(version);
		if (rtlGetVersion(&version) != 0 || version.dwBuildNumber < 22621) {
			return false;
		}
		return TransparencyEnabled();
	}

	SystemBackdropResult ApplyDesktopAcrylic(HWND window, bool enabled) {
		SystemBackdropResult result;
		result.supported = IsDesktopAcrylicSupported();

		const DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_NONE;
		DwmSetWindowAttribute(window, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
		const BOOL useHostBackdrop = enabled ? TRUE : FALSE;
		DwmSetWindowAttribute(window, DWMWA_USE_HOSTBACKDROPBRUSH, &useHostBackdrop, sizeof(useHostBackdrop));
		const MARGINS margins = enabled ? MARGINS{ -1, -1, -1, -1 } : MARGINS{ 0, 0, 0, 0 };
		DwmExtendFrameIntoClientArea(window, &margins);

		const DWM_WINDOW_CORNER_PREFERENCE corners = enabled ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
		DwmSetWindowAttribute(window, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
		result.active = false;
		return result;
	}

}
