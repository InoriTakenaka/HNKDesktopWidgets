#pragma once

#include "MainWindow.g.h"
#include "MainWindow.xaml.g.h"

namespace mw::core {
	class Application;
}

namespace winrt::HNKDesktopWidgets::implementation {

	struct MainWindow : MainWindowT<MainWindow> {
		MainWindow();

		void SetHost(mw::core::Application* host);
		void OnToggleClock(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
		void OnToggleWeather(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
		void OnToggleClockClickThrough(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
		void OnToggleWeatherClickThrough(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
		void OnToggleAcrylic(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
		void OnOpenSettings(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
		void OnOpenEditor(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

	private:
		void RefreshStatus();

		mw::core::Application* host_ = nullptr;
		Microsoft::UI::Xaml::DispatcherTimer timer_{ nullptr };
	};

}

namespace winrt::HNKDesktopWidgets::factory_implementation {

	struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
	};

}
