#pragma once

#include "Configuration.h"
#include "WeatherData.h"
#include "Widget.h"

#include <memory>

namespace mw::widgets {

	class WeatherWidget final : public core::Widget {
	public:
		explicit WeatherWidget(core::WidgetConfiguration configuration);
		~WeatherWidget() override;

		WeatherWidget(const WeatherWidget&) = delete;
		WeatherWidget& operator=(const WeatherWidget&) = delete;

		std::wstring_view Type() const override;
		std::wstring_view DisplayText() const override;
		void ApplyReading(WeatherReading reading);
		void Attach(HWND window) override;
		void Detach() override;
		void OnTimer() override;
		UINT NextTimerDelayMs() const override;
		void OnAsyncUpdate() override;
		void MenuItems(std::vector<core::WidgetMenuItem>& items) const override;
		void OnMenuItem(int id) override;
		void Paint(rendering::D2DRenderer& renderer, rendering::DirectWriteRenderer& text) override;

	private:
		struct FetchState;

		void StartFetch();
		void CancelFetch();

		core::WidgetConfiguration configuration_;
		WeatherReading reading_;
		std::shared_ptr<FetchState> fetch_;
		mutable std::wstring displayText_;
	};

}
