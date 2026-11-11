#include "Application.h"

#include "Configuration.h"
#include "WidgetManager.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mw::core {
	namespace {

		std::wstring Widen(std::string_view text) {
			return std::wstring(text.begin(), text.end());
		}

		std::string Utf8(std::wstring_view text) {
			if (text.empty()) {
				return {};
			}
			const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
			if (size <= 0) {
				return {};
			}
			std::string utf8(static_cast<size_t>(size), '\0');
			WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), size, nullptr, nullptr);
			for (char& character : utf8) {
				if (character == '\n' || character == '\r') {
					character = ' ';
				}
			}
			return utf8;
		}

		std::filesystem::path VerificationLogPath() {
			std::wstring buffer(MAX_PATH, L'\0');
			const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			buffer.resize(length);
			return std::filesystem::path(buffer).replace_filename(L"hnkdesktopwidgets-verify.log");
		}

		const char* Flag(bool value) {
			return value ? "ok" : "missing";
		}

		const char* Bool(bool value) {
			return value ? "true" : "false";
		}

	}

	struct Application::State {
		State()
		    : widgets(registry) {
		}

		WidgetRegistry registry;
		WidgetManager widgets;
		HWND queue = nullptr;
		std::vector<std::wstring> pendingDestroy;
		bool started = false;
		std::string lastError;
	};

	Application::Application()
	    : state_(std::make_unique<State>()) {
	}

	Application::~Application() {
		Stop();
	}

	WidgetRegistry& Application::Registry() {
		return state_->registry;
	}

	bool Application::Started() const {
		return state_->started;
	}

	bool Application::HasWidget(std::wstring_view type) const {
		return state_->widgets.HasType(type);
	}

	bool Application::WidgetVisible(std::wstring_view type) const {
		return state_->widgets.Visible(type);
	}

	void Application::SetWidgetVisible(std::wstring_view type, bool visible) {
		if (!state_->started) {
			return;
		}
		state_->widgets.SetVisible(type, visible);
		PublishStatus();
	}

	bool Application::WidgetClickThrough(std::wstring_view type) const {
		return state_->widgets.ClickThrough(type);
	}

	void Application::SetWidgetClickThrough(std::wstring_view type, bool enabled) {
		if (!state_->started) {
			return;
		}
		state_->widgets.SetClickThrough(type, enabled);
		PublishStatus();
	}

	void Application::Start() {
		if (state_->started) {
			return;
		}

		try {
			static bool queueClassRegistered = false;
			if (!queueClassRegistered) {
				WNDCLASSEXW windowClass{};
				windowClass.cbSize = sizeof(windowClass);
				windowClass.lpfnWndProc = &Application::QueueProc;
				windowClass.hInstance = GetModuleHandleW(nullptr);
				windowClass.lpszClassName = L"HNKDesktopWidgets.HostQueue";
				if (RegisterClassExW(&windowClass) == 0) {
					throw std::runtime_error("RegisterClassExW failed (" + std::to_string(GetLastError()) + ")");
				}
				queueClassRegistered = true;
			}
			state_->queue = CreateWindowExW(
			    0,
			    L"HNKDesktopWidgets.HostQueue",
			    L"",
			    0,
			    0,
			    0,
			    0,
			    0,
			    HWND_MESSAGE,
			    nullptr,
			    GetModuleHandleW(nullptr),
			    this);
			if (state_->queue == nullptr) {
				throw std::runtime_error("CreateWindowExW failed for the host queue (" + std::to_string(GetLastError()) + ")");
			}

			state_->widgets.SetCloseHandler([this](const std::wstring& id) {
				state_->pendingDestroy.push_back(id);
				if (state_->queue != nullptr) {
					PostMessageW(state_->queue, WM_APP + 40, 0, 0);
				}
			});
			state_->widgets.SetStatusHandler([this] {
				PublishStatus();
			});

			const Configuration configuration = Configuration::LoadDefault();
			state_->widgets.Load(configuration);
			state_->widgets.ShowAll();
			state_->started = true;
			PublishStatus();
		} catch (const std::exception& exception) {
			state_->lastError = exception.what();
			PublishStatus();
			Stop();
			throw;
		}
	}

	void Application::Stop() {
		state_->pendingDestroy.clear();
		state_->widgets.SetStatusHandler({});
		state_->widgets.SetCloseHandler({});
		state_->widgets.Shutdown();
		if (state_->queue != nullptr) {
			DestroyWindow(state_->queue);
			state_->queue = nullptr;
		}
		state_->started = false;
	}

	bool Application::AcrylicEnabled() const {
		return state_->widgets.Acrylic().enabled;
	}

	bool Application::AcrylicActive() const {
		return state_->widgets.AcrylicActive();
	}

	void Application::SetAcrylicEnabled(bool enabled) {
		if (!state_->started) {
			return;
		}
		state_->widgets.SetAcrylicEnabled(enabled);
		PublishStatus();
	}

	void Application::DrainDestroyRequests() {
		const std::vector<std::wstring> ids = std::move(state_->pendingDestroy);
		state_->pendingDestroy.clear();
		for (const std::wstring& id : ids) {
			state_->widgets.Destroy(id);
		}
		PublishStatus();
	}

	LRESULT CALLBACK Application::QueueProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
		if (message == WM_NCCREATE) {
			const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
			SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
		}
		auto* application = reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
		if (application != nullptr && message == WM_APP + 40) {
			application->DrainDestroyRequests();
			return 0;
		}
		return DefWindowProcW(window, message, wparam, lparam);
	}

	void Application::PublishStatus() {
		const std::vector<WidgetHostStatus> statuses = state_->widgets.Statuses();
		bool devices = !statuses.empty();
		bool swapChains = !statuses.empty();
		bool direct2d = !statuses.empty();
		bool directWrite = !statuses.empty();
		bool composition = !statuses.empty();
		bool topmost = false;
		int redraws = 0;
		int paints = 0;
		std::wstring combined;
		for (const WidgetHostStatus& status : statuses) {
			devices = devices && status.hasDevice;
			swapChains = swapChains && status.hasSwapChain;
			direct2d = direct2d && status.hasDirect2D;
			directWrite = directWrite && status.hasDirectWrite;
			composition = composition && status.hasComposition;
			topmost = topmost || status.topmost;
			redraws += status.redrawRequests;
			paints += status.paints;
			if (!combined.empty()) {
				combined += L" | ";
			}
			combined += status.text;
		}

		std::ostringstream stream;
		stream << "started=" << Bool(state_->started) << '\n';
		stream << "visible=" << Bool(state_->widgets.Visible(L"clock") || state_->widgets.Visible(L"weather")) << '\n';
		stream << "widget_count=" << statuses.size() << '\n';
		stream << "topmost=" << Bool(topmost) << '\n';
		stream << "game_loop=false\n";
		stream << "d3d11=" << Flag(devices) << '\n';
		stream << "dxgi=" << Flag(swapChains) << '\n';
		stream << "d2d=" << Flag(direct2d) << '\n';
		stream << "dwrite=" << Flag(directWrite) << '\n';
		stream << "dcomp=" << Flag(composition) << '\n';
		stream << "redraw_requests=" << redraws << '\n';
		stream << "paints=" << paints << '\n';
		stream << "last_text=" << Utf8(combined) << '\n';
		stream << "acrylic_requested=" << Bool(state_->widgets.Acrylic().enabled) << '\n';
		stream << "acrylic_active=" << Bool(state_->widgets.AcrylicActive()) << '\n';
		for (size_t index = 0; index < statuses.size(); ++index) {
			const WidgetHostStatus& status = statuses[index];
			const std::string prefix = "widget." + std::to_string(index) + ".";
			stream << prefix << "id=" << Utf8(status.id) << '\n';
			stream << prefix << "type=" << Utf8(status.type) << '\n';
			stream << prefix << "hwnd=0x" << std::hex << reinterpret_cast<std::uintptr_t>(status.window) << std::dec << '\n';
			stream << prefix << "parent=0x" << std::hex << reinterpret_cast<std::uintptr_t>(status.parent) << std::dec << '\n';
			stream << prefix << "visible=" << Bool(status.visible) << '\n';
			stream << prefix << "click_through=" << Bool(status.clickThrough) << '\n';
			stream << prefix << "topmost=" << Bool(status.topmost) << '\n';
			stream << prefix << "toolwindow=" << Bool((status.exStyle & WS_EX_TOOLWINDOW) != 0) << '\n';
			stream << prefix << "caption=" << Bool((status.style & WS_CAPTION) != 0) << '\n';
			stream << prefix << "style=0x" << std::hex << status.style << std::dec << '\n';
			stream << prefix << "exstyle=0x" << std::hex << status.exStyle << std::dec << '\n';
			stream << prefix << "acrylic_requested=" << Bool(status.acrylicRequested) << '\n';
			stream << prefix << "acrylic_supported=" << Bool(status.acrylicSupported) << '\n';
			stream << prefix << "acrylic_active=" << Bool(status.acrylicActive) << '\n';
			stream << prefix << "redraw_requests=" << status.redrawRequests << '\n';
			stream << prefix << "paints=" << status.paints << '\n';
			stream << prefix << "timer_ms=" << status.timerDelayMs << '\n';
			stream << prefix << "text=" << Utf8(status.text) << '\n';
			if (!status.error.empty()) {
				stream << prefix << "error=" << status.error << '\n';
			}
		}
		if (!state_->lastError.empty()) {
			stream << "error=" << state_->lastError << '\n';
		}

		std::ofstream output(VerificationLogPath(), std::ios::trunc);
		output << stream.str();
	}

	std::wstring Application::StatusText() const {
		std::wstring text;
		if (!state_->lastError.empty()) {
			text += L"Startup error: ";
			text += Widen(state_->lastError);
			text += L"\n";
		}
		if (!state_->started) {
			text += L"Widget host is not running.\n";
			text += L"Widget updates are event-driven. There is no render loop.";
			return text;
		}

		const std::vector<WidgetHostStatus> statuses = state_->widgets.Statuses();
		text += L"Independent floating windows: " + std::to_wstring(statuses.size()) + L"\n";
		text += state_->widgets.Acrylic().enabled ? L"Acrylic default: enabled\n" : L"Acrylic default: disabled\n";
		if (statuses.empty()) {
			text += L"No widget windows are open.\n";
		}
		for (const WidgetHostStatus& status : statuses) {
			text += status.type + L" (" + status.id + L") ";
			text += status.visible ? L"visible" : L"hidden";
			if (status.clickThrough) {
				text += L", click-through";
			}
			text += status.topmost ? L", topmost\n" : L", not topmost\n";
			text += L"Acrylic: ";
			if (status.acrylicActive) {
				text += L"blurred backdrop active.";
			} else if (status.acrylicRequested) {
				text += L"requested, using the solid fallback.";
			} else {
				text += L"disabled, solid background.";
			}
			text += L"\nRedraw requests: " + std::to_wstring(status.redrawRequests);
			text += L"\nPaints: " + std::to_wstring(status.paints);
			text += L"\nDisplayed: " + status.text + L"\n";
		}
		text += L"Drag a widget to move it. Drag an edge to resize. Right-click for time format, refresh, click-through, hide, and close.\n";
		text += L"Each widget redraws only when its content changes. There is no render loop.";
		return text;
	}

}
