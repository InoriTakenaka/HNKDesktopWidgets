#pragma once

#include "WidgetEditor.g.h"
#include "WidgetEditor.xaml.g.h"

namespace winrt::HNKDesktopWidgets::implementation {

	struct WidgetEditor : WidgetEditorT<WidgetEditor> {
		WidgetEditor();
	};

}

namespace winrt::HNKDesktopWidgets::factory_implementation {

	struct WidgetEditor : WidgetEditorT<WidgetEditor, implementation::WidgetEditor> {
	};

}
