#pragma once

#include "App.xaml.g.h"

#include "Application.h"

#include <memory>

namespace winrt::HNKDesktopWidgets::implementation {

	struct App : AppT<App> {
		App();
		void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const& args);

	private:
		std::unique_ptr<mw::core::Application> host_;
		Microsoft::UI::Xaml::Window window_{ nullptr };
	};

}
