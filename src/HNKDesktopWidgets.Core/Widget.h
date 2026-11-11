#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mw::rendering {
	class D2DRenderer;
	class DirectWriteRenderer;
}

namespace mw::core {

	struct WidgetMenuItem {
		int id = 0;
		std::wstring label;
		bool checked = false;
	};

	class Widget {
	public:
		static constexpr UINT AsyncUpdateMessage = WM_APP + 30;

		virtual ~Widget() = default;

		const std::wstring& Id() const noexcept { return id_; }
		virtual std::wstring_view Type() const = 0;
		virtual std::wstring_view DisplayText() const = 0;

		void SetRedrawHandler(std::function<void()> handler);
		void RequestRedraw();
		void SetTimerHandler(std::function<void()> handler);
		void RequestTimerReschedule();

		virtual void Attach(HWND window) = 0;
		virtual void Detach() = 0;
		virtual void OnTimer() = 0;
		virtual UINT NextTimerDelayMs() const;
		virtual void OnAsyncUpdate();
		virtual void MenuItems(std::vector<WidgetMenuItem>& items) const;
		virtual void OnMenuItem(int id);
		virtual void Paint(rendering::D2DRenderer& renderer, rendering::DirectWriteRenderer& text) = 0;

	protected:
		explicit Widget(std::wstring id);

	private:
		std::wstring id_;
		std::function<void()> redraw_;
		std::function<void()> timer_;
	};

}
