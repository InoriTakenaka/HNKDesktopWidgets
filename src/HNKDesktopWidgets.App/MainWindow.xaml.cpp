#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "Application.h"
#include "SettingsPage.xaml.h"
#include "WidgetEditor.xaml.h"

#include <chrono>

namespace winrt::HNKDesktopWidgets::implementation {

	MainWindow::MainWindow() {
		InitializeComponent();
		Title(L"NYX Desktop Widgets");
		AppWindow().Resize({ 1080, 760 });

		Closed([this](auto&&, auto&&) {
			if (timer_ != nullptr) {
				timer_.Stop();
			}
			if (host_ != nullptr) {
				host_->Stop();
				host_ = nullptr;
			}
		});
	}

	void MainWindow::SetHost(mw::core::Application* host) {
		host_ = host;
		RefreshStatus();

		timer_ = Microsoft::UI::Xaml::DispatcherTimer();
		timer_.Interval(std::chrono::milliseconds(500));
		timer_.Tick([this](auto&&, auto&&) {
			RefreshStatus();
		});
		timer_.Start();
	}

	void MainWindow::RefreshStatus() {
		if (host_ == nullptr) {
			StatusText().Text(L"Widget host is not running.");
			return;
		}
		StatusText().Text(host_->StatusText());
		const bool running = host_->Started();
		const bool hasClock = running && host_->HasWidget(L"clock");
		const bool hasWeather = running && host_->HasWidget(L"weather");
		ToggleButton().IsEnabled(hasClock);
		WeatherButton().IsEnabled(hasWeather);
		ClockClickThroughButton().IsEnabled(hasClock);
		WeatherClickThroughButton().IsEnabled(hasWeather);
		AcrylicButton().IsEnabled(running);
		if (hasClock) {
			const bool visible = host_->WidgetVisible(L"clock");
			ToggleButton().Content(winrt::box_value(visible ? L"Hide clock" : L"Show clock"));
			const bool clickThrough = host_->WidgetClickThrough(L"clock");
			ClockClickThroughButton().Content(winrt::box_value(clickThrough ? L"Disable clock click-through" : L"Enable clock click-through"));
		}
		if (hasWeather) {
			const bool visible = host_->WidgetVisible(L"weather");
			WeatherButton().Content(winrt::box_value(visible ? L"Hide weather" : L"Show weather"));
			const bool clickThrough = host_->WidgetClickThrough(L"weather");
			WeatherClickThroughButton().Content(winrt::box_value(clickThrough ? L"Disable weather click-through" : L"Enable weather click-through"));
		}
		if (running) {
			AcrylicButton().Content(winrt::box_value(host_->AcrylicEnabled() ? L"Use solid background" : L"Use acrylic background"));
		}
	}

	void MainWindow::OnToggleClock(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
		if (host_ == nullptr || !host_->Started() || !host_->HasWidget(L"clock")) {
			return;
		}

		host_->SetWidgetVisible(L"clock", !host_->WidgetVisible(L"clock"));
		RefreshStatus();
	}

	void MainWindow::OnToggleWeather(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
		if (host_ == nullptr || !host_->Started() || !host_->HasWidget(L"weather")) {
			return;
		}

		host_->SetWidgetVisible(L"weather", !host_->WidgetVisible(L"weather"));
		RefreshStatus();
	}

	void MainWindow::OnToggleClockClickThrough(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
		if (host_ == nullptr || !host_->Started() || !host_->HasWidget(L"clock")) {
			return;
		}

		host_->SetWidgetClickThrough(L"clock", !host_->WidgetClickThrough(L"clock"));
		RefreshStatus();
	}

	void MainWindow::OnToggleWeatherClickThrough(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
		if (host_ == nullptr || !host_->Started() || !host_->HasWidget(L"weather")) {
			return;
		}

		host_->SetWidgetClickThrough(L"weather", !host_->WidgetClickThrough(L"weather"));
		RefreshStatus();
	}

	void MainWindow::OnToggleAcrylic(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
		if (host_ == nullptr || !host_->Started()) {
			return;
		}

		const bool enable = !host_->AcrylicEnabled();
		host_->SetAcrylicEnabled(enable);
		AcrylicButton().Content(winrt::box_value(enable ? L"Use solid background" : L"Use acrylic background"));
		RefreshStatus();
	}

	void MainWindow::OnOpenSettings(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
		ContentFrame().Navigate(winrt::xaml_typename<winrt::HNKDesktopWidgets::SettingsPage>());
	}

	void MainWindow::OnOpenEditor(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&) {
		ContentFrame().Navigate(winrt::xaml_typename<winrt::HNKDesktopWidgets::WidgetEditor>());
	}

}
