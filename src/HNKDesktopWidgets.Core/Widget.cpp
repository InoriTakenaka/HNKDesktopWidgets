#include "Widget.h"

namespace mw::core {

	Widget::Widget(std::wstring id)
	    : id_(std::move(id)) {
	}

	void Widget::SetRedrawHandler(std::function<void()> handler) {
		redraw_ = std::move(handler);
	}

	void Widget::RequestRedraw() {
		if (redraw_) {
			redraw_();
		}
	}

	void Widget::SetTimerHandler(std::function<void()> handler) {
		timer_ = std::move(handler);
	}

	void Widget::RequestTimerReschedule() {
		if (timer_) {
			timer_();
		}
	}

	UINT Widget::NextTimerDelayMs() const {
		return 1000;
	}

	void Widget::OnAsyncUpdate() {
	}

	void Widget::MenuItems(std::vector<WidgetMenuItem>&) const {
	}

	void Widget::OnMenuItem(int) {
	}

}
