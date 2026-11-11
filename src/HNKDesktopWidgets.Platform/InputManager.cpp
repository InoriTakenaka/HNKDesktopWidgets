#include "InputManager.h"

namespace mw::platform {

	void InputManager::OnMessage(UINT message, WPARAM, LPARAM lparam) {
		switch (message) {
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			cursor_.x = static_cast<SHORT>(LOWORD(lparam));
			cursor_.y = static_cast<SHORT>(HIWORD(lparam));
			break;
		default:
			break;
		}

		if (message == WM_LBUTTONDOWN) {
			primaryDown_ = true;
		} else if (message == WM_LBUTTONUP) {
			primaryDown_ = false;
		}
	}

}
