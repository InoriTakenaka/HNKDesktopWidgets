#include "pch.h"

#include "SettingsPage.xaml.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

namespace winrt::HNKDesktopWidgets::implementation {

	SettingsPage::SettingsPage() {
		InitializeComponent();
		BodyText().Text(
		    L"Deployment is unpackaged. WindowsPackageType is None, so the process is a normal Win32 executable and has no MSIX identity.\n\n"
		    L"The Windows App SDK 2.6.5 dependency is framework-dependent. A machine that runs this application needs the Windows App SDK 2.6 runtime. This development machine already has runtime 2.6.5.\n\n"
		    L"The project builds for x64 with MSVC toolset v145 and the Windows SDK 10.0.26100.0.\n\n"
		    L"WinUI draws this management window only. The clock and weather widgets each use their own native HWND, the shared acrylic backdrop, Direct2D, and DirectWrite.");
	}

}
