#include "MonitorManager.h"

#include <shellscalingapi.h>

#pragma comment(lib, "shcore.lib")

namespace mw::platform {
	namespace {

		BOOL CALLBACK MonitorEnumProc(HMONITOR monitor, HDC, LPRECT, LPARAM parameter) {
			auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(parameter);

			MONITORINFO info{};
			info.cbSize = sizeof(info);
			if (GetMonitorInfoW(monitor, &info) == FALSE) {
				return TRUE;
			}

			UINT dpiX = 96;
			UINT dpiY = 96;
			if (FAILED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
				dpiX = 96;
			}

			MonitorInfo item;
			item.bounds = info.rcMonitor;
			item.workArea = info.rcWork;
			item.dpi = dpiX;
			item.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
			monitors->push_back(item);
			return TRUE;
		}

	}

	std::vector<MonitorInfo> EnumerateMonitors() {
		std::vector<MonitorInfo> monitors;
		EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
		return monitors;
	}

}
