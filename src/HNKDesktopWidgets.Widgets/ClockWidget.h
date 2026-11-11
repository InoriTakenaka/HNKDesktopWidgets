#pragma once

#include "Configuration.h"
#include "Widget.h"

#include <ctime>
#include <string>

namespace mw::widgets {

	struct ClockPresentation {
		bool hour24 = true;
		bool showSeconds = true;
		std::wstring timeFormat;
		std::wstring weekdayFormat;
		std::wstring dateFormat;
	};

	struct ClockText {
		std::wstring time;
		std::wstring weekday;
		std::wstring date;
	};

	std::wstring ClockTimePattern(const ClockPresentation& presentation);
	bool ClockShowsSeconds(const ClockPresentation& presentation);
	ClockText FormatClock(const std::tm& local, const ClockPresentation& presentation);
	UINT ClockTimerDelayMs(bool showSeconds);

	class ClockWidget final : public core::Widget {
	public:
		explicit ClockWidget(core::WidgetConfiguration configuration);

		std::wstring_view Type() const override;
		std::wstring_view DisplayText() const override;
		void Attach(HWND window) override;
		void Detach() override;
		void OnTimer() override;
		UINT NextTimerDelayMs() const override;
		void MenuItems(std::vector<core::WidgetMenuItem>& items) const override;
		void OnMenuItem(int id) override;
		void Paint(rendering::D2DRenderer& renderer, rendering::DirectWriteRenderer& text) override;

		bool SetDisplayedTime(const std::tm& local);

	private:
		void UpdateFromLocalTime();

		core::WidgetConfiguration configuration_;
		ClockPresentation presentation_;
		std::wstring timeText_;
		std::wstring weekdayText_;
		std::wstring dateText_;
		mutable std::wstring displayText_;
	};

}
