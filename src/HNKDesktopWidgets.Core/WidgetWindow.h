#pragma once

#include "Configuration.h"
#include "SystemBackdrop.h"
#include "Widget.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace mw::platform {
	class Win32Window;
	class InputManager;
}

namespace mw::rendering {
	class CompositionManager;
	class D2DRenderer;
	class DirectWriteRenderer;
}

namespace mw::core {

	struct WidgetHostStatus {
		std::wstring id;
		std::wstring type;
		HWND window = nullptr;
		HWND parent = nullptr;
		bool visible = false;
		bool clickThrough = false;
		bool topmost = false;
		bool acrylicRequested = false;
		bool acrylicSupported = false;
		bool acrylicActive = false;
		unsigned long style = 0;
		unsigned long exStyle = 0;
		int redrawRequests = 0;
		int paints = 0;
		unsigned int timerDelayMs = 0;
		std::wstring text;
		std::string error;
		bool hasDevice = false;
		bool hasSwapChain = false;
		bool hasDirect2D = false;
		bool hasDirectWrite = false;
		bool hasComposition = false;
		bool hasBlur = false;
	};

	class WidgetWindow {
	public:
		WidgetWindow(std::unique_ptr<Widget> widget, const WidgetConfiguration& configuration, const AcrylicSettings& globalAcrylic);
		~WidgetWindow();

		WidgetWindow(const WidgetWindow&) = delete;
		WidgetWindow& operator=(const WidgetWindow&) = delete;

		const std::wstring& Id() const;
		std::wstring_view Type() const;
		HWND Handle() const noexcept;
		bool Visible() const;
		void Show();
		void Hide();
		bool ClickThrough() const;
		void SetClickThrough(bool enabled);
		void ApplyAcrylic(const AcrylicSettings& globalAcrylic);
		void SetStatusHandler(std::function<void()> handler);
		void SetCloseHandler(std::function<void()> handler);
		WidgetHostStatus Status() const;

	private:
		void Shutdown();
		void ScheduleTimer();
		void RefreshBackdrop();
		void OnRedrawRequested();
		void PaintFrame();
		void ShowMenu(HWND window, LPARAM position);
		void Publish();
		std::optional<LRESULT> HandleMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

		std::unique_ptr<Widget> widget_;
		std::optional<AcrylicSettings> override_;
		AcrylicSettings resolved_{};
		platform::SystemBackdropResult backdrop_{};
		std::function<void()> statusHandler_;
		std::function<void()> closeHandler_;
		int redrawRequests_ = 0;
		int paints_ = 0;
		std::string lastError_;
		bool shutdown_ = false;
		std::unique_ptr<platform::Win32Window> window_;
		std::unique_ptr<platform::InputManager> input_;
		std::unique_ptr<rendering::CompositionManager> composition_;
		std::unique_ptr<rendering::D2DRenderer> renderer_;
		std::unique_ptr<rendering::DirectWriteRenderer> text_;
	};

}
