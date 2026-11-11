#include "Win32Window.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace mw::platform {
	namespace {

		void ThrowIfFailed(bool succeeded, const char* message) {
			if (!succeeded) {
				throw std::runtime_error(std::string(message) + " (" + std::to_string(GetLastError()) + ")");
			}
		}

	}

	void Win32Window::RegisterClassOnce() {
		static bool registered = false;
		if (registered) {
			return;
		}

		WNDCLASSEXW windowClass{};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = &Win32Window::WndProc;
		windowClass.hInstance = GetModuleHandleW(nullptr);
		windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		windowClass.lpszClassName = L"HNKDesktopWidgets.WidgetHost";
		ThrowIfFailed(RegisterClassExW(&windowClass) != 0, "RegisterClassExW");
		registered = true;
	}

	Win32Window::Win32Window(int x, int y, int width, int height, std::wstring title, int minWidth, int minHeight)
	    : minWidth_(minWidth), minHeight_(minHeight) {
		RegisterClassOnce();
		if (title.empty()) {
			title = L"HNK Desktop Widget";
		}
		hwnd_ = CreateWindowExW(
		    WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW,
		    L"HNKDesktopWidgets.WidgetHost",
		    title.c_str(),
		    WS_POPUP | WS_THICKFRAME,
		    x,
		    y,
		    width,
		    height,
		    nullptr,
		    nullptr,
		    GetModuleHandleW(nullptr),
		    this);
		ThrowIfFailed(hwnd_ != nullptr, "CreateWindowExW");
	}

	Win32Window::~Win32Window() {
		if (hwnd_ != nullptr) {
			SetMessageHandler({});
			DestroyWindow(hwnd_);
			hwnd_ = nullptr;
		}
	}

	void Win32Window::Show() {
		ShowWindow(hwnd_, SW_SHOWNA);
	}

	void Win32Window::Hide() {
		ShowWindow(hwnd_, SW_HIDE);
	}

	bool Win32Window::Visible() const noexcept {
		return hwnd_ != nullptr && IsWindowVisible(hwnd_) != FALSE;
	}

	void Win32Window::SetClickThrough(bool enabled) {
		clickThrough_ = enabled;
	}

	void Win32Window::SetMessageHandler(MessageHandler handler) {
		handler_ = std::move(handler);
	}

	LRESULT Win32Window::HitTest(int x, int y) const {
		RECT rect{};
		GetWindowRect(hwnd_, &rect);
		const UINT dpi = GetDpiForWindow(hwnd_);
		const int border = std::max(6, MulDiv(8, dpi == 0 ? 96 : static_cast<int>(dpi), 96));
		const bool left = x < rect.left + border;
		const bool right = x >= rect.right - border;
		const bool top = y < rect.top + border;
		const bool bottom = y >= rect.bottom - border;
		if (top && left) {
			return HTTOPLEFT;
		}
		if (top && right) {
			return HTTOPRIGHT;
		}
		if (bottom && left) {
			return HTBOTTOMLEFT;
		}
		if (bottom && right) {
			return HTBOTTOMRIGHT;
		}
		if (left) {
			return HTLEFT;
		}
		if (right) {
			return HTRIGHT;
		}
		if (top) {
			return HTTOP;
		}
		if (bottom) {
			return HTBOTTOM;
		}
		return HTCAPTION;
	}

	LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
		if (message == WM_NCCREATE) {
			const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
			auto* window = static_cast<Win32Window*>(create->lpCreateParams);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
			window->hwnd_ = hwnd;
		}

		auto* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (window != nullptr) {
			if (message == WM_NCCALCSIZE && wparam != 0) {
				return 0;
			}
			if (message == WM_NCHITTEST) {
				if (window->clickThrough_) {
					return HTTRANSPARENT;
				}
				const int x = static_cast<int>(static_cast<short>(LOWORD(lparam)));
				const int y = static_cast<int>(static_cast<short>(HIWORD(lparam)));
				return window->HitTest(x, y);
			}
			if (message == WM_NCLBUTTONDBLCLK && wparam == HTCAPTION) {
				return 0;
			}
			if (message == WM_GETMINMAXINFO) {
				auto* info = reinterpret_cast<MINMAXINFO*>(lparam);
				const UINT dpi = GetDpiForWindow(hwnd);
				const int scale = dpi == 0 ? 96 : static_cast<int>(dpi);
				info->ptMinTrackSize.x = MulDiv(window->minWidth_, scale, 96);
				info->ptMinTrackSize.y = MulDiv(window->minHeight_, scale, 96);
				return 0;
			}
			if (message == WM_MOUSEACTIVATE) {
				return MA_NOACTIVATE;
			}
			if (window->handler_) {
				if (const std::optional<LRESULT> handled = window->handler_(hwnd, message, wparam, lparam)) {
					return *handled;
				}
			}
		}
		return DefWindowProcW(hwnd, message, wparam, lparam);
	}

}
