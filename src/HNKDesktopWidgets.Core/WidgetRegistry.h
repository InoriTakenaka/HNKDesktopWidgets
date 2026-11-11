#pragma once

#include "Configuration.h"
#include "Widget.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mw::core {

	class WidgetRegistry {
	public:
		using Factory = std::function<std::unique_ptr<Widget>(const WidgetConfiguration&)>;

		void Register(std::wstring type, Factory factory);
		bool Contains(std::wstring_view type) const;
		std::unique_ptr<Widget> Create(const WidgetConfiguration& configuration) const;
		std::vector<std::wstring> Types() const;

	private:
		std::unordered_map<std::wstring, Factory> factories_;
	};

}
