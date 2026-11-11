#pragma once

#include "Configuration.h"
#include "Widget.h"

#include <chrono>

namespace mw::widgets {

	class SystemMonitorWidget final : public core::Widget {
	public:
		explicit SystemMonitorWidget(core::WidgetConfiguration configuration);

		std::wstring_view Type() const override;
		std::wstring_view DisplayText() const override;
		int SamplingIntervalMs() const noexcept { return configuration_.samplingIntervalMs; }
		void Attach(HWND window) override;
		void Detach() override;
		void OnTimer() override;
		UINT NextTimerDelayMs() const override;
		void Paint(rendering::D2DRenderer& renderer, rendering::DirectWriteRenderer& text) override;

	private:
		core::WidgetConfiguration configuration_;
		HWND window_ = nullptr;
		std::chrono::steady_clock::time_point last_{};
	};

}
