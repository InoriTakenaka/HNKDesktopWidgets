#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace mw::platform {

	class InputManager {
	public:
		void OnMessage(UINT message, WPARAM wparam, LPARAM lparam);
		POINT Cursor() const noexcept { return cursor_; }
		bool PrimaryButtonDown() const noexcept { return primaryDown_; }

	private:
		POINT cursor_{};
		bool primaryDown_ = false;
	};

}
