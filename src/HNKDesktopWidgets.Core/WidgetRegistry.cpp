#include "WidgetRegistry.h"

#include <stdexcept>
#include <utility>

namespace mw::core {
	namespace {

		std::string Narrow(std::wstring_view text) {
			std::string narrow;
			narrow.reserve(text.size());
			for (const wchar_t character : text) {
				narrow.push_back(character < 128 ? static_cast<char>(character) : '?');
			}
			return narrow;
		}

	}

	void WidgetRegistry::Register(std::wstring type, Factory factory) {
		const auto inserted = factories_.emplace(std::move(type), std::move(factory));
		if (!inserted.second) {
			throw std::invalid_argument("Widget type is already registered: " + Narrow(inserted.first->first));
		}
	}

	bool WidgetRegistry::Contains(std::wstring_view type) const {
		return factories_.find(std::wstring(type)) != factories_.end();
	}

	std::unique_ptr<Widget> WidgetRegistry::Create(const WidgetConfiguration& configuration) const {
		const auto found = factories_.find(configuration.type);
		if (found == factories_.end()) {
			throw std::invalid_argument("No widget is registered for type: " + Narrow(configuration.type));
		}
		return found->second(configuration);
	}

	std::vector<std::wstring> WidgetRegistry::Types() const {
		std::vector<std::wstring> types;
		types.reserve(factories_.size());
		for (const auto& entry : factories_) {
			types.push_back(entry.first);
		}
		return types;
	}

}
