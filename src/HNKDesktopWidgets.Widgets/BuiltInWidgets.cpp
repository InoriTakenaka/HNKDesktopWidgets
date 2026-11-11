#include "BuiltInWidgets.h"

#include "ClockWidget.h"
#include "SystemMonitorWidget.h"
#include "WeatherWidget.h"
#include "WidgetRegistry.h"

#include <memory>

namespace mw::widgets {

	void RegisterBuiltInWidgets(core::WidgetRegistry& registry) {
		registry.Register(L"clock", [](const core::WidgetConfiguration& configuration) {
			return std::make_unique<ClockWidget>(configuration);
		});
		registry.Register(L"weather", [](const core::WidgetConfiguration& configuration) {
			return std::make_unique<WeatherWidget>(configuration);
		});
		registry.Register(L"system-monitor", [](const core::WidgetConfiguration& configuration) {
			return std::make_unique<SystemMonitorWidget>(configuration);
		});
	}

}
