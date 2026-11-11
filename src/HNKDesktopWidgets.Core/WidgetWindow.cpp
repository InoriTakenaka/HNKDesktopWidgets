#include "WidgetWindow.h"

#include "CompositionManager.h"
#include "D2DRenderer.h"
#include "DirectWriteRenderer.h"
#include "InputManager.h"
#include "Win32Window.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace mw::core {
	namespace {

		constexpr UINT_PTR kTimerId = 1;
		constexpr int kClickThroughCommand = 1001;
		constexpr int kHideCommand = 1002;
		constexpr int kCloseCommand = 1003;

		rendering::DrawColor Rgb(float red, float green, float blue) {
			return rendering::DrawColor{ red, green, blue };
		}

	}

	WidgetWindow::WidgetWindow(std::unique_ptr<Widget> widget, const WidgetConfiguration& configuration, const AcrylicSettings& globalAcrylic)
	    : widget_(std::move(widget)), override_(configuration.acrylic), resolved_(ResolveAcrylic(globalAcrylic, override_)) {
		if (widget_ == nullptr) {
			throw std::invalid_argument("A widget window requires a widget.");
		}
		try {
			window_ = std::make_unique<platform::Win32Window>(
			    configuration.x,
			    configuration.y,
			    configuration.width,
			    configuration.height,
			    std::wstring(widget_->Type()),
			    240,
			    160);
			window_->SetMessageHandler([this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
				return HandleMessage(hwnd, message, wparam, lparam);
			});
			input_ = std::make_unique<platform::InputManager>();
			composition_ = std::make_unique<rendering::CompositionManager>();
			composition_->Initialize(window_->Handle());
			renderer_ = std::make_unique<rendering::D2DRenderer>(composition_->DeviceContext());
			text_ = std::make_unique<rendering::DirectWriteRenderer>();
			text_->Initialize();
			widget_->SetRedrawHandler([this] {
				OnRedrawRequested();
			});
			widget_->SetTimerHandler([this] {
				ScheduleTimer();
			});
			RefreshBackdrop();
			widget_->Attach(window_->Handle());
			PostMessageW(window_->Handle(), WM_APP + 21, 0, 0);
			ScheduleTimer();
		} catch (...) {
			Shutdown();
			throw;
		}
	}

	WidgetWindow::~WidgetWindow() {
		Shutdown();
	}

	const std::wstring& WidgetWindow::Id() const {
		return widget_->Id();
	}

	std::wstring_view WidgetWindow::Type() const {
		return widget_->Type();
	}

	HWND WidgetWindow::Handle() const noexcept {
		return window_ == nullptr ? nullptr : window_->Handle();
	}

	bool WidgetWindow::Visible() const {
		return window_ != nullptr && window_->Visible();
	}

	void WidgetWindow::Show() {
		if (window_ == nullptr) {
			return;
		}
		window_->Show();
		InvalidateRect(window_->Handle(), nullptr, FALSE);
		Publish();
	}

	void WidgetWindow::Hide() {
		if (window_ == nullptr) {
			return;
		}
		window_->Hide();
		Publish();
	}

	bool WidgetWindow::ClickThrough() const {
		return window_ != nullptr && window_->ClickThrough();
	}

	void WidgetWindow::SetClickThrough(bool enabled) {
		if (window_ == nullptr) {
			return;
		}
		window_->SetClickThrough(enabled);
		Publish();
	}

	void WidgetWindow::ApplyAcrylic(const AcrylicSettings& globalAcrylic) {
		resolved_ = ResolveAcrylic(globalAcrylic, override_);
		RefreshBackdrop();
	}

	void WidgetWindow::SetStatusHandler(std::function<void()> handler) {
		statusHandler_ = std::move(handler);
	}

	void WidgetWindow::SetCloseHandler(std::function<void()> handler) {
		closeHandler_ = std::move(handler);
	}

	WidgetHostStatus WidgetWindow::Status() const {
		WidgetHostStatus status;
		if (widget_ != nullptr) {
			status.id = widget_->Id();
			status.type = std::wstring(widget_->Type());
			status.text = std::wstring(widget_->DisplayText());
			status.timerDelayMs = widget_->NextTimerDelayMs();
		}
		status.window = Handle();
		if (status.window != nullptr) {
			status.parent = GetParent(status.window);
			status.visible = IsWindowVisible(status.window) != FALSE;
			status.style = static_cast<unsigned long>(GetWindowLongW(status.window, GWL_STYLE));
			status.exStyle = static_cast<unsigned long>(GetWindowLongW(status.window, GWL_EXSTYLE));
			status.topmost = (status.exStyle & WS_EX_TOPMOST) != 0;
		}
		status.clickThrough = ClickThrough();
		status.acrylicRequested = resolved_.enabled;
		status.acrylicSupported = backdrop_.supported;
		status.acrylicActive = backdrop_.active;
		status.redrawRequests = redrawRequests_;
		status.paints = paints_;
		status.error = lastError_;
		status.hasDevice = composition_ != nullptr && composition_->HasDevice();
		status.hasSwapChain = composition_ != nullptr && composition_->HasSwapChain();
		status.hasDirect2D = composition_ != nullptr && composition_->HasDirect2D();
		status.hasDirectWrite = text_ != nullptr && text_->Initialized();
		status.hasComposition = composition_ != nullptr && composition_->HasComposition();
		status.hasBlur = composition_ != nullptr && composition_->HasBlurBackdrop();
		return status;
	}

	void WidgetWindow::Shutdown() {
		if (shutdown_) {
			return;
		}
		shutdown_ = true;
		if (window_ != nullptr) {
			KillTimer(window_->Handle(), kTimerId);
			window_->SetMessageHandler({});
		}
		if (widget_ != nullptr) {
			widget_->Detach();
			widget_->SetRedrawHandler({});
			widget_->SetTimerHandler({});
		}
		widget_.reset();
		renderer_.reset();
		text_.reset();
		composition_.reset();
		input_.reset();
		window_.reset();
	}

	void WidgetWindow::ScheduleTimer() {
		if (window_ == nullptr || widget_ == nullptr) {
			return;
		}
		const UINT delay = std::max(1u, widget_->NextTimerDelayMs());
		SetTimer(window_->Handle(), kTimerId, delay, nullptr);
	}

	void WidgetWindow::RefreshBackdrop() {
		if (window_ == nullptr) {
			return;
		}
		backdrop_ = platform::ApplyDesktopAcrylic(window_->Handle(), resolved_.enabled);
		backdrop_.active = resolved_.enabled && backdrop_.supported && composition_ != nullptr && composition_->HasBlurBackdrop();
		InvalidateRect(window_->Handle(), nullptr, FALSE);
		Publish();
	}

	void WidgetWindow::OnRedrawRequested() {
		++redrawRequests_;
		if (window_ != nullptr) {
			InvalidateRect(window_->Handle(), nullptr, FALSE);
		}
		Publish();
	}

	void WidgetWindow::PaintFrame() {
		renderer_->BeginDraw();
		const bool wantGlass = resolved_.enabled && backdrop_.supported;
		const bool glass = wantGlass && composition_->HasBlurBackdrop();
		backdrop_.active = glass;
		if (wantGlass) {
			renderer_->Clear(0.0f, 0.0f, 0.0f, 0.0f);
			if (glass) {
				renderer_->FillRectangle(
				    0.0f,
				    0.0f,
				    renderer_->Width(),
				    renderer_->Height(),
				    resolved_.tintRed,
				    resolved_.tintGreen,
				    resolved_.tintBlue,
				    resolved_.tintOpacity);
			}
			renderer_->SetPalette(Rgb(0.10f, 0.12f, 0.16f), Rgb(0.28f, 0.32f, 0.38f), Rgb(0.00f, 0.45f, 0.84f));
		} else {
			renderer_->Clear(resolved_.fallbackRed, resolved_.fallbackGreen, resolved_.fallbackBlue, 1.0f);
			renderer_->SetPalette(Rgb(0.96f, 0.97f, 0.98f), Rgb(0.76f, 0.80f, 0.86f), Rgb(0.55f, 0.80f, 1.0f));
		}
		widget_->Paint(*renderer_, *text_);
		renderer_->EndDraw();
		composition_->Present();
		++paints_;
	}

	void WidgetWindow::ShowMenu(HWND window, LPARAM position) {
		HMENU menu = CreatePopupMenu();
		if (menu == nullptr || widget_ == nullptr) {
			if (menu != nullptr) {
				DestroyMenu(menu);
			}
			return;
		}

		std::vector<WidgetMenuItem> items;
		widget_->MenuItems(items);
		for (const WidgetMenuItem& item : items) {
			const UINT flags = MF_STRING | (item.checked ? MF_CHECKED : MF_UNCHECKED);
			AppendMenuW(menu, flags, static_cast<UINT_PTR>(item.id), item.label.c_str());
		}
		if (!items.empty()) {
			AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
		}
		AppendMenuW(menu, MF_STRING | (ClickThrough() ? MF_CHECKED : MF_UNCHECKED), kClickThroughCommand, L"Click-through");
		AppendMenuW(menu, MF_STRING, kHideCommand, L"Hide");
		AppendMenuW(menu, MF_STRING, kCloseCommand, L"Close");

		POINT point{};
		if (position == static_cast<LPARAM>(-1)) {
			RECT rect{};
			GetWindowRect(window, &rect);
			point.x = (rect.left + rect.right) / 2;
			point.y = (rect.top + rect.bottom) / 2;
		} else {
			point.x = static_cast<int>(static_cast<short>(LOWORD(position)));
			point.y = static_cast<int>(static_cast<short>(HIWORD(position)));
		}

		SetForegroundWindow(window);
		const int command = TrackPopupMenu(
		    menu,
		    TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
		    point.x,
		    point.y,
		    0,
		    window,
		    nullptr);
		PostMessageW(window, WM_NULL, 0, 0);
		DestroyMenu(menu);
		if (command == 0 || widget_ == nullptr) {
			return;
		}
		if (command == kClickThroughCommand) {
			SetClickThrough(!ClickThrough());
			return;
		}
		if (command == kHideCommand) {
			Hide();
			return;
		}
		if (command == kCloseCommand) {
			if (closeHandler_) {
				closeHandler_();
			}
			return;
		}
		widget_->OnMenuItem(command);
	}

	void WidgetWindow::Publish() {
		if (statusHandler_) {
			statusHandler_();
		}
	}

	std::optional<LRESULT> WidgetWindow::HandleMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
		if (input_ != nullptr) {
			input_->OnMessage(message, wparam, lparam);
		}
		switch (message) {
		case WM_ERASEBKGND:
			return 1;
		case WM_TIMER:
			if (wparam == kTimerId && widget_ != nullptr) {
				widget_->OnTimer();
				ScheduleTimer();
			}
			return 0;
		case WM_TIMECHANGE:
			if (widget_ != nullptr) {
				widget_->OnTimer();
				ScheduleTimer();
			}
			return 0;
		case WM_APP + 21:
			try {
				if (composition_ != nullptr) {
					composition_->CreateBackdrop();
				}
			} catch (const std::exception& exception) {
				lastError_ = exception.what();
				Publish();
			}
			return 0;
		case WM_APP + 22:
			if (wparam == 0) {
				lastError_ = "The blurred backdrop could not be created.";
			}
			RefreshBackdrop();
			return 0;
		case Widget::AsyncUpdateMessage:
			if (widget_ != nullptr) {
				widget_->OnAsyncUpdate();
			}
			return 0;
		case WM_NCRBUTTONUP:
			// HTCAPTION makes the right-click non-client, so DefWindowProc never sends WM_CONTEXTMENU.
			ShowMenu(window, lparam);
			return 0;
		case WM_CONTEXTMENU:
			ShowMenu(window, lparam);
			return 0;
		case WM_CLOSE:
			if (closeHandler_) {
				closeHandler_();
			} else {
				Hide();
			}
			return 0;
		case WM_PAINT: {
			PAINTSTRUCT paint{};
			BeginPaint(window, &paint);
			try {
				if (composition_ != nullptr && composition_->HasComposition() && renderer_ != nullptr && text_ != nullptr && widget_ != nullptr) {
					PaintFrame();
				}
			} catch (const std::exception& exception) {
				lastError_ = exception.what();
			}
			EndPaint(window, &paint);
			Publish();
			return 0;
		}
		case WM_SIZE:
			if (composition_ != nullptr && LOWORD(lparam) > 0 && HIWORD(lparam) > 0) {
				try {
					composition_->Resize(LOWORD(lparam), HIWORD(lparam));
					InvalidateRect(window, nullptr, FALSE);
				} catch (const std::exception& exception) {
					lastError_ = exception.what();
					Publish();
				}
			}
			return 0;
		case WM_DPICHANGED: {
			const auto* suggested = reinterpret_cast<const RECT*>(lparam);
			SetWindowPos(
			    window,
			    nullptr,
			    suggested->left,
			    suggested->top,
			    suggested->right - suggested->left,
			    suggested->bottom - suggested->top,
			    SWP_NOZORDER | SWP_NOACTIVATE);
			return 0;
		}
		case WM_SETTINGCHANGE:
		case WM_DWMCOMPOSITIONCHANGED:
			RefreshBackdrop();
			return 0;
		default:
			return std::nullopt;
		}
	}

}
