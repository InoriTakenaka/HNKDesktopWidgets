#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <functional>
#include <optional>
#include <string>

namespace mw::platform {

	class Win32Window {
	public:
		using MessageHandler = std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)>;

		Win32Window(int x, int y, int width, int height, std::wstring title, int minWidth = 220, int minHeight = 150);
		~Win32Window();

		Win32Window(const Win32Window&) = delete;
		Win32Window& operator=(const Win32Window&) = delete;

		HWND Handle() const noexcept { return hwnd_; }
		void Show();
		void Hide();
		bool Visible() const noexcept;
		bool ClickThrough() const noexcept { return clickThrough_; }
		void SetClickThrough(bool enabled);
		void SetMessageHandler(MessageHandler handler);

	private:
		static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		static void RegisterClassOnce();
		LRESULT HitTest(int x, int y) const;

		HWND hwnd_ = nullptr;
		bool clickThrough_ = false;
		int minWidth_ = 220;
		int minHeight_ = 150;
		MessageHandler handler_;
	};

}
