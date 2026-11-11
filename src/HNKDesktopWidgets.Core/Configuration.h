#pragma once

#include <optional>
#include <string>
#include <vector>

namespace mw::core {

	struct AcrylicSettings {
		bool enabled = true;
		float tintRed = 1.0f;
		float tintGreen = 1.0f;
		float tintBlue = 1.0f;
		float tintOpacity = 0.08f;
		float fallbackRed = 0.05f;
		float fallbackGreen = 0.08f;
		float fallbackBlue = 0.12f;
	};

	inline AcrylicSettings ResolveAcrylic(const AcrylicSettings& global, const std::optional<AcrylicSettings>& widgetOverride) {
		if (widgetOverride.has_value()) {
			return *widgetOverride;
		}
		return global;
	}

	struct WidgetConfiguration {
		std::wstring id;
		std::wstring type;
		int x = 100;
		int y = 100;
		int width = 360;
		int height = 160;
		int samplingIntervalMs = 1000;
		std::optional<AcrylicSettings> acrylic;

		// Clock. Empty format strings use the defaults selected by hour24 and showSeconds.
		bool hour24 = true;
		bool showSeconds = true;
		std::wstring timeFormat;
		std::wstring weekdayFormat;
		std::wstring dateFormat;

		// Weather. Open-Meteo is used unless sampleWeather is set.
		std::wstring location;
		bool hasCoordinates = false;
		double latitude = 0;
		double longitude = 0;
		int refreshIntervalMs = 600000;
		bool sampleWeather = false;
	};

	class Configuration {
	public:
		explicit Configuration(std::vector<WidgetConfiguration> widgets, AcrylicSettings acrylic = {});

		static Configuration LoadDefault();

		const std::vector<WidgetConfiguration>& Widgets() const noexcept { return widgets_; }
		const AcrylicSettings& Acrylic() const noexcept { return acrylic_; }

	private:
		std::vector<WidgetConfiguration> widgets_;
		AcrylicSettings acrylic_;
	};

}
