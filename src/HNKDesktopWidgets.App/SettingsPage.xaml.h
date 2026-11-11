#pragma once

#include "SettingsPage.g.h"
#include "SettingsPage.xaml.g.h"

namespace winrt::HNKDesktopWidgets::implementation {

	struct SettingsPage : SettingsPageT<SettingsPage> {
		SettingsPage();
	};

}

namespace winrt::HNKDesktopWidgets::factory_implementation {

	struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {
	};

}
