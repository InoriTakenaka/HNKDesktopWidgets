#pragma once

#include <optional>
#include <string>
#include <string_view>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

namespace mw::widgets {

	struct WeatherReading {
		bool ok = false;
		bool sample = false;
		std::wstring location;
		std::wstring condition;
		std::wstring detail;
		int weatherCode = -1;
		double temperatureC = 0;
		double feelsLikeC = 0;
		int humidityPercent = 0;
		double windKmh = 0;
		bool hasFeelsLike = false;
		bool hasHumidity = false;
		bool hasWind = false;
	};

	std::wstring WeatherConditionName(int weatherCode);
	WeatherReading MakeSampleWeather(std::wstring location);
	bool ParseOpenMeteoLocation(std::string_view json, double& latitude, double& longitude, std::wstring& name);
	WeatherReading ParseOpenMeteoForecast(std::string_view json, std::wstring location);

	// session is an open WinHTTP session. The caller owns its lifetime.
	WeatherReading FetchOpenMeteo(HINTERNET session, std::wstring location, bool hasCoordinates, double latitude, double longitude);
	WeatherReading FetchOpenMeteo(std::wstring location, bool hasCoordinates = false, double latitude = 0, double longitude = 0);

}
