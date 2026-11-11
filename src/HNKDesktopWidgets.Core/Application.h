#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "WidgetRegistry.h"

#include <memory>
#include <string>
#include <string_view>

namespace mw::core {

	class Application {
	public:
		Application();
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		WidgetRegistry& Registry();

		void Start();
		void Stop();

		bool Started() const;
		bool HasWidget(std::wstring_view type) const;
		bool WidgetVisible(std::wstring_view type) const;
		void SetWidgetVisible(std::wstring_view type, bool visible);
		bool WidgetClickThrough(std::wstring_view type) const;
		void SetWidgetClickThrough(std::wstring_view type, bool enabled);

		bool AcrylicEnabled() const;
		bool AcrylicActive() const;
		void SetAcrylicEnabled(bool enabled);

		std::wstring StatusText() const;

	private:
		struct State;
		static LRESULT CALLBACK QueueProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

		void DrainDestroyRequests();
		void PublishStatus();

		std::unique_ptr<State> state_;
	};

}
