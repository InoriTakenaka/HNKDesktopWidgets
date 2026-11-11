#include "Configuration.h"

#include "DesktopIntegration.h"
#include "MonitorManager.h"

#include <algorithm>
#include <utility>

namespace mw::core {

	Configuration::Configuration(std::vector<WidgetConfiguration> widgets, AcrylicSettings acrylic)
	    : widgets_(std::move(widgets)), acrylic_(acrylic) {
	}

	Configuration Configuration::LoadDefault() {
		WidgetConfiguration clock;
		clock.id = L"clock-1";
		clock.type = L"clock";
		clock.width = 320;
		clock.height = 188;
		clock.samplingIntervalMs = 1000;
		clock.hour24 = true;
		clock.showSeconds = true;
		platform::PlaceOnPrimaryWorkArea(clock.x, clock.y, clock.width, clock.height, 24);

		WidgetConfiguration weather;
		weather.id = L"weather-1";
		weather.type = L"weather";
		weather.width = 340;
		weather.height = 236;
		weather.samplingIntervalMs = 600000;
		weather.refreshIntervalMs = 600000;
		weather.location = L"Hong Kong";
		weather.sampleWeather = false;
		int placedX = 0;
		int placedY = 0;
		platform::PlaceOnPrimaryWorkArea(placedX, placedY, weather.width, weather.height, 24);
		const int gap = std::max(16, clock.width / 16);
		weather.x = clock.x - gap - weather.width;
		weather.y = clock.y + clock.height - weather.height;

		RECT work{};
		bool foundWorkArea = false;
		for (const platform::MonitorInfo& monitor : platform::EnumerateMonitors()) {
			if (monitor.primary) {
				work = monitor.workArea;
				foundWorkArea = true;
				break;
			}
		}
		if (foundWorkArea) {
			if (weather.x < work.left) {
				weather.x = clock.x;
				weather.y = clock.y - gap - weather.height;
			}
			if (weather.y < work.top) {
				weather.y = work.top + gap;
			}
			if (weather.x < work.left) {
				weather.x = work.left + gap;
			}
		}

		return Configuration({ std::move(clock), std::move(weather) });
	}

}
