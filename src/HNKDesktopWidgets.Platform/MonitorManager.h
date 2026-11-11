#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <vector>

namespace mw::platform {

	struct MonitorInfo {
		RECT bounds{};
		RECT workArea{};
		UINT dpi = 96;
		bool primary = false;
	};

	std::vector<MonitorInfo> EnumerateMonitors();

}
