#include "SystemMonitorWidget.h"

#include "D2DRenderer.h"
#include "DirectWriteRenderer.h"

#include <algorithm>
#include <chrono>
#include <string>

namespace mw::widgets {

	SystemMonitorWidget::SystemMonitorWidget(core::WidgetConfiguration configuration)
	    : Widget(configuration.id), configuration_(std::move(configuration)) {
	}

	std::wstring_view SystemMonitorWidget::Type() const {
		return L"system-monitor";
	}

	std::wstring_view SystemMonitorWidget::DisplayText() const {
		return L"System monitor";
	}

	void SystemMonitorWidget::Attach(HWND window) {
		window_ = window;
		last_ = std::chrono::steady_clock::now();
	}

	void SystemMonitorWidget::Detach() {
		window_ = nullptr;
	}

	UINT SystemMonitorWidget::NextTimerDelayMs() const {
		return static_cast<UINT>(std::max(1, configuration_.samplingIntervalMs));
	}

	void SystemMonitorWidget::OnTimer() {
		const auto now = std::chrono::steady_clock::now();
		if (now - last_ < std::chrono::milliseconds(configuration_.samplingIntervalMs)) {
			return;
		}
		last_ = now;
		// No CPU, memory, or GPU source is connected in this foundation.
	}

	void SystemMonitorWidget::Paint(rendering::D2DRenderer& renderer, rendering::DirectWriteRenderer& text) {
		const float width = renderer.Width() > 40.0f ? renderer.Width() - 40.0f : renderer.Width();
		const std::wstring detail = L"Sampling interval " + std::to_wstring(configuration_.samplingIntervalMs) + L" ms";
		renderer.DrawString(L"System monitor", 20.0f, 24.0f, width, 48.0f, text.TimeFormat());
		renderer.DrawString(detail, 20.0f, 96.0f, width, 28.0f, text.CaptionFormat());
	}

}
