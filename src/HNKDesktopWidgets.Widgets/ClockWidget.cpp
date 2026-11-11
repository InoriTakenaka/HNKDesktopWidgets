#include "ClockWidget.h"

#include "D2DRenderer.h"
#include "DirectWriteRenderer.h"

#include <chrono>
#include <ctime>

namespace mw::widgets {
	namespace {

		bool PatternContainsConversion(std::wstring_view pattern, wchar_t conversion) {
			for (size_t index = 0; index < pattern.size(); ++index) {
				if (pattern[index] != L'%') {
					continue;
				}
				size_t cursor = index + 1;
				if (cursor < pattern.size() && pattern[cursor] == L'%') {
					index = cursor;
					continue;
				}
				while (cursor < pattern.size() && (pattern[cursor] == L'#' || pattern[cursor] == L'-' || pattern[cursor] == L'_' || pattern[cursor] == L'^' || pattern[cursor] == L'0' || (pattern[cursor] >= L'1' && pattern[cursor] <= L'9'))) {
					++cursor;
				}
				if (cursor < pattern.size() && pattern[cursor] == conversion) {
					return true;
				}
				if (cursor > index) {
					index = cursor;
				}
			}
			return false;
		}

		std::wstring FormatField(const std::tm& local, std::wstring_view pattern) {
			if (pattern.empty()) {
				return {};
			}
			const std::wstring format(pattern);
			wchar_t buffer[128]{};
			if (wcsftime(buffer, std::size(buffer), format.c_str(), &local) == 0) {
				return {};
			}
			return buffer;
		}

	}

	std::wstring ClockTimePattern(const ClockPresentation& presentation) {
		if (!presentation.timeFormat.empty()) {
			return presentation.timeFormat;
		}
		if (presentation.hour24) {
			return presentation.showSeconds ? std::wstring(L"%H:%M:%S") : std::wstring(L"%H:%M");
		}
		return presentation.showSeconds ? std::wstring(L"%#I:%M:%S %p") : std::wstring(L"%#I:%M %p");
	}

	bool ClockShowsSeconds(const ClockPresentation& presentation) {
		if (!presentation.timeFormat.empty()) {
			return PatternContainsConversion(presentation.timeFormat, L'S');
		}
		return presentation.showSeconds;
	}

	ClockText FormatClock(const std::tm& local, const ClockPresentation& presentation) {
		ClockText text;
		const std::wstring weekday = presentation.weekdayFormat.empty() ? std::wstring(L"%A") : presentation.weekdayFormat;
		const std::wstring date = presentation.dateFormat.empty() ? std::wstring(L"%B %#d, %Y") : presentation.dateFormat;
		text.time = FormatField(local, ClockTimePattern(presentation));
		text.weekday = FormatField(local, weekday);
		text.date = FormatField(local, date);
		return text;
	}

	UINT ClockTimerDelayMs(bool showSeconds) {
		SYSTEMTIME now{};
		GetLocalTime(&now);
		if (showSeconds) {
			const UINT remaining = 1000u - now.wMilliseconds;
			return remaining == 0 ? 1000u : remaining;
		}
		UINT remaining = static_cast<UINT>(60 - now.wSecond) * 1000u;
		if (remaining > now.wMilliseconds) {
			remaining -= now.wMilliseconds;
		} else {
			remaining = 1;
		}
		return remaining == 0 ? 60000u : remaining;
	}

	ClockWidget::ClockWidget(core::WidgetConfiguration configuration)
	    : Widget(configuration.id), configuration_(std::move(configuration)) {
		presentation_.hour24 = configuration_.hour24;
		presentation_.showSeconds = configuration_.showSeconds;
		presentation_.timeFormat = configuration_.timeFormat;
		presentation_.weekdayFormat = configuration_.weekdayFormat;
		presentation_.dateFormat = configuration_.dateFormat;
	}

	std::wstring_view ClockWidget::Type() const {
		return L"clock";
	}

	std::wstring_view ClockWidget::DisplayText() const {
		displayText_ = timeText_;
		if (!weekdayText_.empty()) {
			if (!displayText_.empty()) {
				displayText_ += L"  ";
			}
			displayText_ += weekdayText_;
		}
		if (!dateText_.empty()) {
			if (!displayText_.empty()) {
				displayText_ += L"  ";
			}
			displayText_ += dateText_;
		}
		return displayText_;
	}

	void ClockWidget::Attach(HWND) {
		UpdateFromLocalTime();
	}

	void ClockWidget::Detach() {
	}

	void ClockWidget::OnTimer() {
		UpdateFromLocalTime();
	}

	UINT ClockWidget::NextTimerDelayMs() const {
		return ClockTimerDelayMs(ClockShowsSeconds(presentation_));
	}

	void ClockWidget::MenuItems(std::vector<core::WidgetMenuItem>& items) const {
		core::WidgetMenuItem hour;
		hour.id = 1;
		hour.label = presentation_.hour24 ? L"Use 12-hour time" : L"Use 24-hour time";
		items.push_back(std::move(hour));

		core::WidgetMenuItem seconds;
		seconds.id = 2;
		seconds.label = ClockShowsSeconds(presentation_) ? L"Hide seconds" : L"Show seconds";
		items.push_back(std::move(seconds));
	}

	void ClockWidget::OnMenuItem(int id) {
		if (id == 1) {
			presentation_.hour24 = !presentation_.hour24;
			presentation_.timeFormat.clear();
		} else if (id == 2) {
			presentation_.showSeconds = !ClockShowsSeconds(presentation_);
			presentation_.timeFormat.clear();
		} else {
			return;
		}
		UpdateFromLocalTime();
		RequestTimerReschedule();
	}

	bool ClockWidget::SetDisplayedTime(const std::tm& local) {
		const ClockText formatted = FormatClock(local, presentation_);
		if (formatted.time == timeText_ && formatted.weekday == weekdayText_ && formatted.date == dateText_) {
			return false;
		}
		timeText_ = formatted.time;
		weekdayText_ = formatted.weekday;
		dateText_ = formatted.date;
		RequestRedraw();
		return true;
	}

	void ClockWidget::UpdateFromLocalTime() {
		const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm local{};
		if (localtime_s(&local, &now) != 0) {
			return;
		}
		SetDisplayedTime(local);
	}

	void ClockWidget::Paint(rendering::D2DRenderer& renderer, rendering::DirectWriteRenderer& text) {
		const float pad = 22.0f;
		const float width = renderer.Width() > pad * 2.0f ? renderer.Width() - pad * 2.0f : renderer.Width();
		const float block = 72.0f + 30.0f + 24.0f;
		float y = renderer.Height() > block ? (renderer.Height() - block) * 0.5f : 12.0f;
		renderer.DrawString(timeText_, pad, y, width, 72.0f, text.DisplayFormat());
		y += 72.0f;
		renderer.DrawString(weekdayText_, pad, y, width, 28.0f, text.TitleFormat());
		y += 30.0f;
		renderer.DrawString(dateText_, pad, y, width, 24.0f, text.DetailFormat(), rendering::TextTone::Secondary);
	}

}
