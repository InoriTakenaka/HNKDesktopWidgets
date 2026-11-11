#include "pch.h"

#include "WidgetEditor.xaml.h"
#if __has_include("WidgetEditor.g.cpp")
#include "WidgetEditor.g.cpp"
#endif

#include "Configuration.h"

#include <string>

namespace winrt::HNKDesktopWidgets::implementation {

	WidgetEditor::WidgetEditor() {
		InitializeComponent();

		const mw::core::Configuration configuration = mw::core::Configuration::LoadDefault();
		std::wstring text = L"Configured widgets:\n";
		text += configuration.Acrylic().enabled ? L"Global acrylic: enabled\n" : L"Global acrylic: disabled\n";
		for (const mw::core::WidgetConfiguration& widget : configuration.Widgets()) {
			text += L"- " + widget.id + L"  type=" + widget.type + L"  bounds=(" + std::to_wstring(widget.x) + L"," + std::to_wstring(widget.y) + L"," + std::to_wstring(widget.width) + L"," + std::to_wstring(widget.height) + L")" + L"  interval=" + std::to_wstring(widget.samplingIntervalMs) + L" ms  acrylic=";
			text += widget.acrylic.has_value() ? L"override" : L"inherit";
			if (widget.type == L"clock") {
				text += widget.hour24 ? L"  time=24-hour" : L"  time=12-hour";
				text += widget.showSeconds ? L"  seconds=shown" : L"  seconds=hidden";
				if (!widget.timeFormat.empty()) {
					text += L"  timeFormat=" + widget.timeFormat;
				}
				if (!widget.dateFormat.empty()) {
					text += L"  dateFormat=" + widget.dateFormat;
				}
			}
			if (widget.type == L"weather") {
				text += L"  location=" + widget.location;
				text += widget.sampleWeather ? L"  data=sample" : L"  data=open-meteo";
				text += L"  refresh=" + std::to_wstring(widget.refreshIntervalMs) + L" ms";
			}
			text += L"\n";
		}
		text += L"\nThe editor does not modify widgets yet. Each configured widget opens in its own window. The clock redraws when the displayed time changes, and weather redraws when a new reading arrives.";
		BodyText().Text(text);
	}

}
