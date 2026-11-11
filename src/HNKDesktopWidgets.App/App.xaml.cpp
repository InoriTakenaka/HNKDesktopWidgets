#include "pch.h"

#include "App.xaml.h"
#include "MainWindow.xaml.h"

#include "BuiltInWidgets.h"

#include <exception>

namespace winrt::HNKDesktopWidgets::implementation {

	App::App() {
		InitializeComponent();

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
		UnhandledException([](winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& args) {
			if (IsDebuggerPresent()) {
				auto errorMessage = args.Message();
				__debugbreak();
			}
		});
#endif
	}

	void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
		host_ = std::make_unique<mw::core::Application>();
		mw::widgets::RegisterBuiltInWidgets(host_->Registry());
		try {
			host_->Start();
		} catch (const std::exception&) {
			// StatusText reports the startup error. The management window still opens.
		}

		auto window = winrt::make_self<MainWindow>();
		window->SetHost(host_.get());
		window_ = *window;
		window->Activate();
	}

}
