#include "WidgetManager.h"

#include <utility>

namespace mw::core {

	WidgetManager::WidgetManager(WidgetRegistry& registry)
	    : registry_(registry) {
	}

	WidgetManager::~WidgetManager() {
		Shutdown();
	}

	void WidgetManager::SetStatusHandler(std::function<void()> handler) {
		statusHandler_ = std::move(handler);
		for (const auto& window : windows_) {
			window->SetStatusHandler(statusHandler_);
		}
	}

	void WidgetManager::SetCloseHandler(std::function<void(const std::wstring& id)> handler) {
		closeHandler_ = std::move(handler);
	}

	void WidgetManager::Load(const Configuration& configuration) {
		Shutdown();
		acrylic_ = configuration.Acrylic();
		windows_.reserve(configuration.Widgets().size());
		for (const WidgetConfiguration& widgetConfiguration : configuration.Widgets()) {
			std::unique_ptr<Widget> widget = registry_.Create(widgetConfiguration);
			auto window = std::make_unique<WidgetWindow>(std::move(widget), widgetConfiguration, acrylic_);
			window->SetStatusHandler(statusHandler_);
			const std::wstring id = window->Id();
			window->SetCloseHandler([this, id] {
				if (closeHandler_) {
					closeHandler_(id);
				}
			});
			windows_.push_back(std::move(window));
		}
	}

	void WidgetManager::ShowAll() {
		for (const auto& window : windows_) {
			window->Show();
		}
	}

	void WidgetManager::Shutdown() {
		for (const auto& window : windows_) {
			window->SetStatusHandler({});
			window->SetCloseHandler({});
		}
		windows_.clear();
	}

	bool WidgetManager::HasType(std::wstring_view type) const {
		for (const auto& window : windows_) {
			if (window->Type() == type) {
				return true;
			}
		}
		return false;
	}

	bool WidgetManager::Visible(std::wstring_view type) const {
		for (const auto& window : windows_) {
			if (window->Type() == type && window->Visible()) {
				return true;
			}
		}
		return false;
	}

	void WidgetManager::SetVisible(std::wstring_view type, bool visible) {
		for (const auto& window : windows_) {
			if (window->Type() == type) {
				if (visible) {
					window->Show();
				} else {
					window->Hide();
				}
			}
		}
	}

	bool WidgetManager::ClickThrough(std::wstring_view type) const {
		for (const auto& window : windows_) {
			if (window->Type() == type) {
				return window->ClickThrough();
			}
		}
		return false;
	}

	void WidgetManager::SetClickThrough(std::wstring_view type, bool enabled) {
		for (const auto& window : windows_) {
			if (window->Type() == type) {
				window->SetClickThrough(enabled);
			}
		}
	}

	void WidgetManager::Destroy(std::wstring_view id) {
		std::erase_if(windows_, [&](const std::unique_ptr<WidgetWindow>& window) {
			return window->Id() == id;
		});
	}

	void WidgetManager::SetAcrylicEnabled(bool enabled) {
		acrylic_.enabled = enabled;
		for (const auto& window : windows_) {
			window->ApplyAcrylic(acrylic_);
		}
	}

	bool WidgetManager::AcrylicActive() const {
		bool anyRequested = false;
		for (const auto& window : windows_) {
			const WidgetHostStatus status = window->Status();
			if (!status.acrylicRequested) {
				continue;
			}
			anyRequested = true;
			if (!status.acrylicActive) {
				return false;
			}
		}
		return anyRequested;
	}

	std::vector<WidgetHostStatus> WidgetManager::Statuses() const {
		std::vector<WidgetHostStatus> statuses;
		statuses.reserve(windows_.size());
		for (const auto& window : windows_) {
			statuses.push_back(window->Status());
		}
		return statuses;
	}

}
