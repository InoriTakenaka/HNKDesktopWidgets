#pragma once

#include "Configuration.h"
#include "WidgetRegistry.h"
#include "WidgetWindow.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mw::core {

	class WidgetManager {
	public:
		explicit WidgetManager(WidgetRegistry& registry);
		~WidgetManager();

		WidgetManager(const WidgetManager&) = delete;
		WidgetManager& operator=(const WidgetManager&) = delete;

		void SetStatusHandler(std::function<void()> handler);
		void SetCloseHandler(std::function<void(const std::wstring& id)> handler);
		void Load(const Configuration& configuration);
		void ShowAll();
		void Shutdown();

		bool HasType(std::wstring_view type) const;
		bool Visible(std::wstring_view type) const;
		void SetVisible(std::wstring_view type, bool visible);
		bool ClickThrough(std::wstring_view type) const;
		void SetClickThrough(std::wstring_view type, bool enabled);
		void Destroy(std::wstring_view id);

		void SetAcrylicEnabled(bool enabled);
		const AcrylicSettings& Acrylic() const noexcept { return acrylic_; }
		bool AcrylicActive() const;
		std::vector<WidgetHostStatus> Statuses() const;

	private:
		WidgetRegistry& registry_;
		AcrylicSettings acrylic_{};
		std::vector<std::unique_ptr<WidgetWindow>> windows_;
		std::function<void()> statusHandler_;
		std::function<void(const std::wstring&)> closeHandler_;
	};

}
