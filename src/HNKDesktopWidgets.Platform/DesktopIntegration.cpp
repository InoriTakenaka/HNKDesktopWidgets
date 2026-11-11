#include "DesktopIntegration.h"

#include "MonitorManager.h"

namespace mw::platform {
	namespace {

		int Scale(int value, UINT dpi) {
			return MulDiv(value, static_cast<int>(dpi), 96);
		}

	}

	void PlaceOnPrimaryWorkArea(int& x, int& y, int& width, int& height, int margin) {
		const std::vector<MonitorInfo> monitors = EnumerateMonitors();
		const MonitorInfo* selected = nullptr;
		for (const MonitorInfo& monitor : monitors) {
			if (monitor.primary) {
				selected = &monitor;
				break;
			}
		}
		if (selected == nullptr && !monitors.empty()) {
			selected = &monitors.front();
		}
		if (selected == nullptr) {
			x = 100;
			y = 100;
			return;
		}

		width = Scale(width, selected->dpi);
		height = Scale(height, selected->dpi);
		const int scaledMargin = Scale(margin, selected->dpi);
		const RECT& work = selected->workArea;

		x = work.right - width - scaledMargin;
		y = work.bottom - height - scaledMargin;
		if (x < work.left) {
			x = work.left;
		}
		if (y < work.top) {
			y = work.top;
		}
	}

}
