#include "BuiltInWidgets.h"
#include "ClockWidget.h"
#include "Configuration.h"
#include "SystemBackdrop.h"
#include "SystemMonitorWidget.h"
#include "WeatherData.h"
#include "WeatherWidget.h"
#include "WidgetRegistry.h"
#include "WidgetWindow.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <string_view>

namespace {

	int g_failures = 0;

	void Expect(bool condition, const char* description) {
		if (!condition) {
			std::fprintf(stderr, "Failed: %s\n", description);
			++g_failures;
		}
	}

	std::string Utf8(std::wstring_view text) {
		if (text.empty()) {
			return {};
		}
		const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		if (size <= 0) {
			return {};
		}
		std::string utf8(static_cast<size_t>(size), '\0');
		WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), size, nullptr, nullptr);
		return utf8;
	}

	void Pump(unsigned milliseconds) {
		const DWORD start = GetTickCount();
		for (;;) {
			MSG message{};
			while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE) {
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
			if (GetTickCount() - start >= milliseconds) {
				break;
			}
			Sleep(15);
		}
	}

	LPARAM CenterPoint(HWND window) {
		RECT rect{};
		GetWindowRect(window, &rect);
		const int x = (rect.left + rect.right) / 2;
		const int y = (rect.top + rect.bottom) / 2;
		return MAKELPARAM(static_cast<WORD>(static_cast<SHORT>(x)), static_cast<WORD>(static_cast<SHORT>(y)));
	}

	void CheckWindows() {
		mw::core::WidgetRegistry registry;
		mw::widgets::RegisterBuiltInWidgets(registry);

		mw::core::WidgetConfiguration clockConfig;
		clockConfig.id = L"clock-test";
		clockConfig.type = L"clock";
		clockConfig.x = 80;
		clockConfig.y = 80;
		clockConfig.width = 320;
		clockConfig.height = 188;
		clockConfig.showSeconds = true;
		clockConfig.hour24 = true;

		mw::core::WidgetConfiguration weatherConfig;
		weatherConfig.id = L"weather-test";
		weatherConfig.type = L"weather";
		weatherConfig.x = 460;
		weatherConfig.y = 80;
		weatherConfig.width = 340;
		weatherConfig.height = 236;
		weatherConfig.sampleWeather = true;
		weatherConfig.location = L"Sample City";

		mw::core::AcrylicSettings acrylic;
		auto clock = std::make_unique<mw::core::WidgetWindow>(registry.Create(clockConfig), clockConfig, acrylic);
		auto weather = std::make_unique<mw::core::WidgetWindow>(registry.Create(weatherConfig), weatherConfig, acrylic);
		const HWND clockHwnd = clock->Handle();
		const HWND weatherHwnd = weather->Handle();
		Expect(clockHwnd != nullptr && weatherHwnd != nullptr && clockHwnd != weatherHwnd, "each widget has its own HWND");
		clock->Show();
		weather->Show();

		const DWORD started = GetTickCount();
		for (;;) {
			Pump(150);
			const bool clockReady = clock->Status().acrylicActive || !mw::platform::IsDesktopAcrylicSupported();
			const bool weatherReady = weather->Status().acrylicActive || !mw::platform::IsDesktopAcrylicSupported();
			if ((clockReady && weatherReady && clock->Status().paints > 0 && weather->Status().paints > 0) || GetTickCount() - started >= 3000) {
				break;
			}
		}

		const mw::core::WidgetHostStatus clockStatus = clock->Status();
		const mw::core::WidgetHostStatus weatherStatus = weather->Status();
		std::printf(
		    "clock hwnd=%p ex=0x%lx style=0x%lx parent=%p acrylic=%d paints=%d text=%s error=%s\n",
		    clockStatus.window,
		    clockStatus.exStyle,
		    clockStatus.style,
		    clockStatus.parent,
		    clockStatus.acrylicActive ? 1 : 0,
		    clockStatus.paints,
		    Utf8(clockStatus.text).c_str(),
		    clockStatus.error.c_str());
		std::printf(
		    "weather hwnd=%p ex=0x%lx style=0x%lx parent=%p acrylic=%d paints=%d text=%s error=%s\n",
		    weatherStatus.window,
		    weatherStatus.exStyle,
		    weatherStatus.style,
		    weatherStatus.parent,
		    weatherStatus.acrylicActive ? 1 : 0,
		    weatherStatus.paints,
		    Utf8(weatherStatus.text).c_str(),
		    weatherStatus.error.c_str());

		Expect(clockStatus.hasDevice && weatherStatus.hasDevice, "both widgets create a Direct3D device");
		Expect(clockStatus.hasSwapChain && weatherStatus.hasSwapChain, "both widgets create a DXGI swap chain");
		Expect(clockStatus.hasDirect2D && weatherStatus.hasDirect2D, "both widgets create Direct2D");
		Expect(clockStatus.hasDirectWrite && weatherStatus.hasDirectWrite, "both widgets create DirectWrite");
		Expect(clockStatus.hasComposition && weatherStatus.hasComposition, "both widgets create a composition target");
		Expect(clockStatus.paints > 0 && weatherStatus.paints > 0, "both widgets paint");
		Expect(clockStatus.parent == nullptr && weatherStatus.parent == nullptr, "widgets are top-level windows");
		Expect(!clockStatus.topmost && !weatherStatus.topmost, "widgets are not topmost");
		Expect((clockStatus.exStyle & WS_EX_TOOLWINDOW) != 0 && (weatherStatus.exStyle & WS_EX_TOOLWINDOW) != 0, "widgets are tool windows");
		Expect((clockStatus.style & WS_CAPTION) == 0 && (weatherStatus.style & WS_CAPTION) == 0, "widgets have no title bar");
		Expect((clockStatus.style & WS_POPUP) != 0 && (weatherStatus.style & WS_POPUP) != 0, "widgets are borderless popups");
		Expect(clockStatus.text.find(L':') != std::wstring::npos, "the clock window shows a time");
		Expect(weatherStatus.text.find(L"Sample data") != std::wstring::npos, "the weather window identifies sample data");

		if (mw::platform::IsDesktopAcrylicSupported()) {
			Expect(clockStatus.acrylicActive && weatherStatus.acrylicActive, "acrylic is active on both widgets");
		} else {
			Expect(!clockStatus.acrylicActive && !weatherStatus.acrylicActive, "acrylic uses the solid fallback when it is unavailable");
		}

		const int clockPaints = clock->Status().paints;
		Pump(400);
		const int clockDelta = clock->Status().paints - clockPaints;
		Expect(clockDelta >= 0 && clockDelta < 4, "widgets do not continuously redraw");

		RECT weatherBefore{};
		RECT clockBefore{};
		GetWindowRect(weatherHwnd, &weatherBefore);
		GetWindowRect(clockHwnd, &clockBefore);
		SetWindowPos(clockHwnd, nullptr, clockBefore.left + 48, clockBefore.top + 36, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		RECT weatherAfterMove{};
		RECT clockAfterMove{};
		GetWindowRect(weatherHwnd, &weatherAfterMove);
		GetWindowRect(clockHwnd, &clockAfterMove);
		Expect(weatherBefore.left == weatherAfterMove.left && weatherBefore.top == weatherAfterMove.top && weatherBefore.right == weatherAfterMove.right && weatherBefore.bottom == weatherAfterMove.bottom, "moving the clock does not move the weather widget");
		Expect(clockAfterMove.left == clockBefore.left + 48 && clockAfterMove.top == clockBefore.top + 36, "the clock window moved on its own");

		const int paintsBeforeResize = clock->Status().paints;
		SetWindowPos(
		    clockHwnd,
		    nullptr,
		    0,
		    0,
		    (clockAfterMove.right - clockAfterMove.left) + 40,
		    (clockAfterMove.bottom - clockAfterMove.top) + 24,
		    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		Pump(200);
		RECT weatherAfterResize{};
		GetWindowRect(weatherHwnd, &weatherAfterResize);
		Expect(weatherAfterResize.left == weatherAfterMove.left && weatherAfterResize.right == weatherAfterMove.right && weatherAfterResize.top == weatherAfterMove.top && weatherAfterResize.bottom == weatherAfterMove.bottom, "resizing the clock does not resize the weather widget");
		Expect(clock->Status().paints > paintsBeforeResize, "resizing invalidates the resized widget");

		clock->Hide();
		Expect(IsWindowVisible(clockHwnd) == FALSE, "the clock can be hidden");
		Expect(IsWindowVisible(weatherHwnd) != FALSE, "hiding the clock leaves the weather widget visible");
		clock->Show();

		Expect(SendMessageW(clockHwnd, WM_NCHITTEST, 0, CenterPoint(clockHwnd)) == HTCAPTION, "normal hit testing accepts the clock");
		clock->SetClickThrough(true);
		Expect(SendMessageW(clockHwnd, WM_NCHITTEST, 0, CenterPoint(clockHwnd)) == HTTRANSPARENT, "click-through passes the clock hit test through");
		Expect(SendMessageW(weatherHwnd, WM_NCHITTEST, 0, CenterPoint(weatherHwnd)) == HTCAPTION, "clock click-through does not change the weather widget");
		clock->SetClickThrough(false);
		Expect(SendMessageW(clockHwnd, WM_NCHITTEST, 0, CenterPoint(clockHwnd)) == HTCAPTION, "click-through can be turned off");

		const HWND survivingWeather = weatherHwnd;
		clock.reset();
		Expect(IsWindow(clockHwnd) == FALSE, "destroying the clock destroys only its HWND");
		Expect(IsWindow(survivingWeather) != FALSE && IsWindowVisible(survivingWeather) != FALSE, "destroying the clock leaves the weather widget open");
	}

}

int main() {
	const mw::core::Configuration configuration = mw::core::Configuration::LoadDefault();
	Expect(configuration.Widgets().size() == 2, "default configuration contains the clock and weather widgets");
	Expect(configuration.Widgets().front().type == L"clock", "the first default widget is a clock");
	Expect(configuration.Widgets().front().samplingIntervalMs == 1000, "clock interval is one second");
	Expect(configuration.Widgets().front().hour24, "the default clock uses 24-hour time");
	Expect(configuration.Widgets().front().showSeconds, "the default clock shows seconds");
	Expect(configuration.Widgets()[1].type == L"weather", "the second default widget is weather");
	Expect(!configuration.Widgets()[1].sampleWeather, "weather uses the live source by default");
	Expect(configuration.Widgets()[1].location == L"Hong Kong", "the default weather location is configured");
	Expect(configuration.Widgets()[1].refreshIntervalMs >= 60000, "weather does not poll rapidly");
	Expect(configuration.Acrylic().enabled, "acrylic is enabled by default");
	Expect(!configuration.Widgets().front().acrylic.has_value(), "the default clock inherits the global acrylic setting");

	mw::core::AcrylicSettings global = configuration.Acrylic();
	global.tintOpacity = 0.4f;
	const mw::core::AcrylicSettings inherited = mw::core::ResolveAcrylic(global, std::nullopt);
	Expect(inherited.enabled && inherited.tintOpacity == 0.4f, "a widget without an override uses the global acrylic setting");
	mw::core::AcrylicSettings widgetAcrylic = global;
	widgetAcrylic.enabled = false;
	widgetAcrylic.tintOpacity = 0.2f;
	const mw::core::AcrylicSettings overridden = mw::core::ResolveAcrylic(global, widgetAcrylic);
	Expect(!overridden.enabled && overridden.tintOpacity == 0.2f, "a widget override replaces the global acrylic setting");

	std::tm local{};
	local.tm_year = 126;
	local.tm_mon = 10;
	local.tm_mday = 11;
	local.tm_wday = 3;
	local.tm_hour = 15;
	local.tm_min = 4;
	local.tm_sec = 5;
	mw::widgets::ClockPresentation presentation;
	const mw::widgets::ClockText full = mw::widgets::FormatClock(local, presentation);
	wchar_t expectedTime[128]{};
	wchar_t expectedWeekday[128]{};
	wchar_t expectedDate[128]{};
	wcsftime(expectedTime, 128, L"%H:%M:%S", &local);
	wcsftime(expectedWeekday, 128, L"%A", &local);
	wcsftime(expectedDate, 128, L"%B %#d, %Y", &local);
	Expect(full.time == expectedTime, "24-hour time matches the locale pattern");
	Expect(full.weekday == expectedWeekday, "weekday matches the locale pattern");
	Expect(full.date == expectedDate, "date matches the locale pattern");
	Expect(mw::widgets::ClockShowsSeconds(presentation), "the default clock pattern includes seconds");

	presentation.hour24 = false;
	presentation.showSeconds = false;
	const mw::widgets::ClockText twelve = mw::widgets::FormatClock(local, presentation);
	wchar_t expectedTwelve[128]{};
	wcsftime(expectedTwelve, 128, L"%#I:%M %p", &local);
	Expect(twelve.time == expectedTwelve, "12-hour time without seconds matches the locale pattern");
	Expect(twelve.time != full.time, "12-hour time differs from 24-hour time");
	std::tm sameMinute = local;
	sameMinute.tm_sec = 40;
	Expect(mw::widgets::FormatClock(sameMinute, presentation).time == twelve.time, "hiding seconds ignores a change within the minute");
	presentation.timeFormat = L"%H:%M";
	Expect(!mw::widgets::ClockShowsSeconds(presentation), "a custom format without seconds hides seconds");
	presentation.timeFormat = L"%H:%M:%S";
	Expect(mw::widgets::ClockShowsSeconds(presentation), "a custom format with seconds shows seconds");
	presentation.timeFormat = L"%%S %H:%M";
	Expect(!mw::widgets::ClockShowsSeconds(presentation), "an escaped percent is not a seconds conversion");

	mw::core::WidgetConfiguration formattedClock;
	formattedClock.id = L"clock-format";
	formattedClock.type = L"clock";
	formattedClock.showSeconds = false;
	formattedClock.hour24 = true;
	auto formatWidget = std::make_unique<mw::widgets::ClockWidget>(formattedClock);
	int formatRedraws = 0;
	formatWidget->SetRedrawHandler([&formatRedraws] {
		++formatRedraws;
	});
	Expect(formatWidget->SetDisplayedTime(local), "the first displayed time requests a redraw");
	const int afterMinute = formatRedraws;
	sameMinute.tm_sec = 10;
	Expect(!formatWidget->SetDisplayedTime(sameMinute), "the same displayed minute does not redraw");
	Expect(formatRedraws == afterMinute, "a seconds-only change does not request a redraw");
	sameMinute.tm_min = 5;
	Expect(formatWidget->SetDisplayedTime(sameMinute), "the next minute requests one redraw");
	Expect(formatRedraws == afterMinute + 1, "the minute change requests exactly one redraw");

	mw::core::WidgetRegistry registry;
	mw::widgets::RegisterBuiltInWidgets(registry);
	Expect(registry.Contains(L"clock"), "clock type is registered");
	Expect(registry.Contains(L"weather"), "weather type is registered");
	Expect(registry.Contains(L"system-monitor"), "system monitor type is registered");
	Expect(registry.Types().size() == 3, "three built-in types are registered");

	const auto clock = registry.Create(configuration.Widgets().front());
	int redraws = 0;
	clock->SetRedrawHandler([&redraws] {
		++redraws;
	});
	clock->RequestRedraw();
	Expect(redraws == 1, "explicit redraw request notifies the host once");

	clock->OnTimer();
	const int afterFirstTick = redraws;
	Expect(afterFirstTick == 2, "first timer event publishes the initial time");
	clock->OnTimer();
	Expect(redraws == afterFirstTick || redraws == afterFirstTick + 1, "a second tick does not enter a render loop");

	const char* geocode = R"({"results":[{"name":"Hong Kong","latitude":22.3193,"longitude":114.1694}]})";
	double latitude = 0;
	double longitude = 0;
	std::wstring geocodedName;
	Expect(mw::widgets::ParseOpenMeteoLocation(geocode, latitude, longitude, geocodedName), "geocoding json parses");
	Expect(geocodedName == L"Hong Kong", "geocoding name is read");
	Expect(latitude > 22.0 && latitude < 23.0 && longitude > 114.0 && longitude < 115.0, "geocoding coordinates are read");

	const char* forecast = R"({"current_units":{"temperature_2m":"C","weather_code":"wmo code"},"current":{"temperature_2m":26.4,"relative_humidity_2m":70,"apparent_temperature":29.1,"weather_code":2,"wind_speed_10m":12.6}})";
	const mw::widgets::WeatherReading parsed = mw::widgets::ParseOpenMeteoForecast(forecast, L"Hong Kong");
	Expect(parsed.ok && !parsed.sample, "forecast json parses as live data");
	Expect(parsed.weatherCode == 2 && parsed.condition == L"Partly cloudy", "weather code maps to a condition");
	Expect(std::lround(parsed.temperatureC) == 26, "temperature is read from the numeric field");
	Expect(parsed.hasHumidity && parsed.humidityPercent == 70, "humidity is read");
	Expect(parsed.hasFeelsLike && std::lround(parsed.feelsLikeC) == 29, "feels-like temperature is read");
	Expect(parsed.hasWind && std::lround(parsed.windKmh) == 13, "wind speed is read");

	mw::core::WidgetConfiguration weatherConfiguration;
	weatherConfiguration.id = L"weather-1";
	weatherConfiguration.type = L"weather";
	weatherConfiguration.sampleWeather = true;
	weatherConfiguration.location = L"Sample City";
	auto weatherBase = registry.Create(weatherConfiguration);
	auto* weather = dynamic_cast<mw::widgets::WeatherWidget*>(weatherBase.get());
	Expect(weather != nullptr, "registry creates a weather widget");
	if (weather != nullptr) {
		weather->SetRedrawHandler([&redraws] {
			++redraws;
		});
		const mw::widgets::WeatherReading sample = mw::widgets::MakeSampleWeather(L"Sample City");
		const int before = redraws;
		weather->ApplyReading(sample);
		Expect(std::wstring(weather->DisplayText()).find(L"Sample data") != std::wstring::npos, "sample weather is labeled as sample data");
		Expect(redraws == before + 1, "new weather data requests one redraw");
		weather->ApplyReading(sample);
		Expect(redraws == before + 1, "unchanged weather data does not request a redraw");
		weather->ApplyReading(parsed);
		Expect(std::wstring(weather->DisplayText()).find(L"Sample data") == std::wstring::npos, "live weather is not labeled as sample data");
		Expect(std::wstring(weather->DisplayText()).find(L"26") != std::wstring::npos, "the weather widget shows the parsed temperature");
	}

	mw::core::WidgetConfiguration monitorConfiguration;
	monitorConfiguration.id = L"system-1";
	monitorConfiguration.type = L"system-monitor";
	monitorConfiguration.samplingIntervalMs = 2500;
	auto monitorBase = registry.Create(monitorConfiguration);
	auto* monitor = dynamic_cast<mw::widgets::SystemMonitorWidget*>(monitorBase.get());
	Expect(monitor != nullptr && monitor->SamplingIntervalMs() == 2500, "system monitor keeps its sampling interval");
	Expect(monitor != nullptr && monitor->NextTimerDelayMs() == 2500, "system monitor timer follows its sampling interval");
	if (monitor != nullptr) {
		monitor->OnTimer();
	}

	const mw::widgets::WeatherReading live = mw::widgets::FetchOpenMeteo(L"Hong Kong");
	if (!live.ok) {
		std::fprintf(stderr, "Live weather fetch failed: %s\n", Utf8(live.detail).c_str());
		++g_failures;
	} else {
		Expect(!live.sample, "live weather is not sample data");
		Expect(!live.location.empty() && !live.condition.empty(), "live weather has a location and condition");
		Expect(live.hasHumidity && live.hasWind && live.hasFeelsLike, "live weather includes humidity, wind, and feels-like");
		std::printf(
		    "Live weather: %s temp=%.1f feels=%.1f humidity=%d wind=%.1f code=%d condition=%s\n",
		    Utf8(live.location).c_str(),
		    live.temperatureC,
		    live.feelsLikeC,
		    live.humidityPercent,
		    live.windKmh,
		    live.weatherCode,
		    Utf8(live.condition).c_str());
	}

	try {
		CheckWindows();
	} catch (const std::exception& exception) {
		std::fprintf(stderr, "Window check failed: %s\n", exception.what());
		++g_failures;
	}

	if (g_failures == 0) {
		std::puts("HNKDesktopWidgets.Tests passed.");
		return 0;
	}

	std::fprintf(stderr, "%d check(s) failed.\n", g_failures);
	return 1;
}
