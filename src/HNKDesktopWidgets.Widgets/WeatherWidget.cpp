#include "WeatherWidget.h"

#include "D2DRenderer.h"
#include "DirectWriteRenderer.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <string>
#include <thread>

namespace mw::widgets {
	namespace {

		enum class IconKind {
			Sun,
			PartlyCloudy,
			Cloud,
			Fog,
			Rain,
			Snow,
			Storm,
		};

		IconKind IconForCode(int code) {
			if (code == 0) {
				return IconKind::Sun;
			}
			if (code == 1 || code == 2) {
				return IconKind::PartlyCloudy;
			}
			if (code == 3) {
				return IconKind::Cloud;
			}
			if (code == 45 || code == 48) {
				return IconKind::Fog;
			}
			if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
				return IconKind::Snow;
			}
			if (code >= 95) {
				return IconKind::Storm;
			}
			if (code >= 51) {
				return IconKind::Rain;
			}
			return IconKind::Cloud;
		}

		std::wstring FormatDegrees(double celsius) {
			return std::to_wstring(static_cast<int>(std::lround(celsius))) + L"\u00B0";
		}

		std::wstring DetailLine(const WeatherReading& reading) {
			std::wstring text;
			auto append = [&text](const std::wstring& part) {
				if (!text.empty()) {
					text += L"    ";
				}
				text += part;
			};
			if (reading.hasFeelsLike) {
				append(L"Feels like " + FormatDegrees(reading.feelsLikeC));
			}
			if (reading.hasHumidity) {
				append(L"Humidity " + std::to_wstring(reading.humidityPercent) + L"%");
			}
			if (reading.hasWind) {
				append(L"Wind " + std::to_wstring(static_cast<int>(std::lround(reading.windKmh))) + L" km/h");
			}
			return text;
		}

		void DrawCloud(rendering::D2DRenderer& renderer, float x, float y, float size, rendering::DrawColor color) {
			const float centerX = x + size * 0.50f;
			const float centerY = y + size * 0.56f;
			renderer.FillEllipse(centerX - size * 0.16f, centerY - size * 0.06f, size * 0.16f, size * 0.13f, color.red, color.green, color.blue, 1.0f);
			renderer.FillEllipse(centerX + size * 0.02f, centerY - size * 0.12f, size * 0.20f, size * 0.16f, color.red, color.green, color.blue, 1.0f);
			renderer.FillEllipse(centerX + size * 0.18f, centerY - size * 0.02f, size * 0.13f, size * 0.11f, color.red, color.green, color.blue, 1.0f);
			renderer.FillEllipse(centerX, centerY + size * 0.05f, size * 0.30f, size * 0.12f, color.red, color.green, color.blue, 1.0f);
		}

		void DrawSun(rendering::D2DRenderer& renderer, float centerX, float centerY, float radius, rendering::DrawColor color) {
			renderer.FillEllipse(centerX, centerY, radius, radius, color.red, color.green, color.blue, 1.0f);
			constexpr float pi = 3.14159265f;
			const float inner = radius * 1.7f;
			const float outer = radius * 2.45f;
			const float stroke = std::max(1.6f, radius * 0.18f);
			for (int ray = 0; ray < 8; ++ray) {
				const float angle = (pi * 2.0f * static_cast<float>(ray)) / 8.0f;
				const float cosine = std::cos(angle);
				const float sine = std::sin(angle);
				renderer.DrawLine(
				    centerX + cosine * inner,
				    centerY + sine * inner,
				    centerX + cosine * outer,
				    centerY + sine * outer,
				    stroke,
				    color.red,
				    color.green,
				    color.blue,
				    1.0f);
			}
		}

		void DrawWeatherIcon(rendering::D2DRenderer& renderer, IconKind kind, float x, float y, float size) {
			const rendering::DrawColor accent = renderer.AccentColor();
			const rendering::DrawColor cloud = renderer.SecondaryColor();
			const rendering::DrawColor ink = renderer.PrimaryColor();
			const float stroke = std::max(1.75f, size * 0.03f);
			switch (kind) {
			case IconKind::Sun:
				DrawSun(renderer, x + size * 0.50f, y + size * 0.50f, size * 0.16f, accent);
				break;
			case IconKind::PartlyCloudy:
				DrawSun(renderer, x + size * 0.36f, y + size * 0.34f, size * 0.11f, accent);
				DrawCloud(renderer, x, y + size * 0.08f, size, cloud);
				break;
			case IconKind::Cloud:
				DrawCloud(renderer, x, y, size, cloud);
				break;
			case IconKind::Fog:
				for (int line = 0; line < 4; ++line) {
					const float yLine = y + size * (0.28f + 0.14f * static_cast<float>(line));
					const float inset = (line % 2 == 0) ? size * 0.16f : size * 0.26f;
					renderer.DrawLine(x + inset, yLine, x + size - inset, yLine, stroke, cloud.red, cloud.green, cloud.blue, 1.0f);
				}
				break;
			case IconKind::Rain:
				DrawCloud(renderer, x, y - size * 0.06f, size * 0.92f, cloud);
				for (int drop = 0; drop < 3; ++drop) {
					const float dropX = x + size * (0.30f + 0.18f * static_cast<float>(drop));
					const float dropY = y + size * 0.64f;
					renderer.DrawLine(dropX, dropY, dropX - size * 0.06f, dropY + size * 0.16f, stroke, ink.red, ink.green, ink.blue, 1.0f);
				}
				break;
			case IconKind::Snow:
				DrawCloud(renderer, x, y - size * 0.08f, size * 0.92f, cloud);
				for (int flake = 0; flake < 3; ++flake) {
					const float flakeX = x + size * (0.32f + 0.18f * static_cast<float>(flake));
					renderer.FillEllipse(flakeX, y + size * 0.74f, size * 0.035f, size * 0.035f, ink.red, ink.green, ink.blue, 1.0f);
				}
				break;
			case IconKind::Storm:
				DrawCloud(renderer, x, y - size * 0.08f, size * 0.92f, cloud);
				renderer.DrawLine(x + size * 0.56f, y + size * 0.52f, x + size * 0.44f, y + size * 0.70f, stroke, accent.red, accent.green, accent.blue, 1.0f);
				renderer.DrawLine(x + size * 0.44f, y + size * 0.70f, x + size * 0.58f, y + size * 0.70f, stroke, accent.red, accent.green, accent.blue, 1.0f);
				renderer.DrawLine(x + size * 0.58f, y + size * 0.70f, x + size * 0.46f, y + size * 0.90f, stroke, accent.red, accent.green, accent.blue, 1.0f);
				break;
			}
		}

		std::wstring Summary(const WeatherReading& reading) {
			std::wstring text = reading.location + L"  " + FormatDegrees(reading.temperatureC);
			if (!reading.condition.empty()) {
				text += L"  " + reading.condition;
			}
			const std::wstring detail = DetailLine(reading);
			if (!detail.empty()) {
				text += L"  " + detail;
			}
			return text;
		}

	}

	struct WeatherWidget::FetchState {
		std::mutex mutex;
		bool alive = true;
		bool busy = false;
		HWND window = nullptr;
		HINTERNET session = nullptr;
		WeatherReading pending{};
		bool hasPending = false;
		int generation = 0;
		int pendingGeneration = 0;
		std::thread worker;
	};

	WeatherWidget::WeatherWidget(core::WidgetConfiguration configuration)
	    : Widget(configuration.id), configuration_(std::move(configuration)), fetch_(std::make_shared<FetchState>()) {
		reading_.location = configuration_.location;
		reading_.detail = L"Loading weather...";
	}

	WeatherWidget::~WeatherWidget() {
		Detach();
	}

	std::wstring_view WeatherWidget::Type() const {
		return L"weather";
	}

	std::wstring_view WeatherWidget::DisplayText() const {
		if (reading_.sample) {
			displayText_ = L"Sample data  " + Summary(reading_);
			return displayText_;
		}
		if (!reading_.ok) {
			displayText_ = reading_.detail.empty() ? std::wstring(L"Weather unavailable") : reading_.detail;
			return displayText_;
		}
		displayText_ = Summary(reading_);
		return displayText_;
	}

	void WeatherWidget::ApplyReading(WeatherReading reading) {
		const std::wstring previous = std::wstring(DisplayText());
		reading_ = std::move(reading);
		if (std::wstring(DisplayText()) != previous) {
			RequestRedraw();
		}
	}

	void WeatherWidget::Attach(HWND window) {
		{
			std::lock_guard<std::mutex> lock(fetch_->mutex);
			fetch_->alive = true;
			fetch_->window = window;
		}
		if (configuration_.sampleWeather) {
			ApplyReading(MakeSampleWeather(configuration_.location));
			return;
		}
		WeatherReading loading;
		loading.location = configuration_.location;
		loading.detail = L"Loading weather...";
		ApplyReading(loading);
		StartFetch();
	}

	void WeatherWidget::Detach() {
		if (fetch_ == nullptr) {
			return;
		}
		{
			std::lock_guard<std::mutex> lock(fetch_->mutex);
			fetch_->alive = false;
			fetch_->window = nullptr;
		}
		CancelFetch();
	}

	void WeatherWidget::OnTimer() {
		if (configuration_.sampleWeather) {
			return;
		}
		StartFetch();
	}

	UINT WeatherWidget::NextTimerDelayMs() const {
		const int configured = configuration_.refreshIntervalMs > 0 ? configuration_.refreshIntervalMs : configuration_.samplingIntervalMs;
		return static_cast<UINT>(std::max(60000, configured));
	}

	void WeatherWidget::OnAsyncUpdate() {
		WeatherReading reading;
		{
			std::lock_guard<std::mutex> lock(fetch_->mutex);
			if (!fetch_->alive || !fetch_->hasPending || fetch_->pendingGeneration != fetch_->generation) {
				fetch_->hasPending = false;
				return;
			}
			reading = std::move(fetch_->pending);
			fetch_->hasPending = false;
		}
		ApplyReading(std::move(reading));
	}

	void WeatherWidget::MenuItems(std::vector<core::WidgetMenuItem>& items) const {
		if (configuration_.sampleWeather) {
			return;
		}
		core::WidgetMenuItem refresh;
		refresh.id = 1;
		refresh.label = L"Refresh";
		items.push_back(std::move(refresh));
	}

	void WeatherWidget::OnMenuItem(int id) {
		if (id == 1 && !configuration_.sampleWeather) {
			CancelFetch();
			StartFetch();
		}
	}

	void WeatherWidget::CancelFetch() {
		if (fetch_ == nullptr) {
			return;
		}
		std::thread worker;
		HINTERNET session = nullptr;
		{
			std::lock_guard<std::mutex> lock(fetch_->mutex);
			++fetch_->generation;
			session = fetch_->session;
			fetch_->session = nullptr;
			fetch_->busy = false;
			worker = std::move(fetch_->worker);
		}
		if (session != nullptr) {
			WinHttpCloseHandle(session);
		}
		if (worker.joinable()) {
			worker.join();
		}
	}

	void WeatherWidget::StartFetch() {
		if (fetch_ == nullptr || configuration_.sampleWeather) {
			return;
		}

		std::thread finished;
		{
			std::lock_guard<std::mutex> lock(fetch_->mutex);
			if (!fetch_->alive || fetch_->window == nullptr || fetch_->busy) {
				return;
			}
			finished = std::move(fetch_->worker);
		}
		if (finished.joinable()) {
			finished.join();
		}

		const std::wstring location = configuration_.location;
		const bool hasCoordinates = configuration_.hasCoordinates;
		const double latitude = configuration_.latitude;
		const double longitude = configuration_.longitude;
		int generation = 0;
		{
			std::lock_guard<std::mutex> lock(fetch_->mutex);
			if (!fetch_->alive || fetch_->window == nullptr) {
				return;
			}
			fetch_->busy = true;
			generation = ++fetch_->generation;
		}

		std::shared_ptr<FetchState> state = fetch_;
		fetch_->worker = std::thread([state, generation, location, hasCoordinates, latitude, longitude] {
			WeatherReading reading;
			HINTERNET session = WinHttpOpen(
			    L"HNKDesktopWidgets/1.0",
			    WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
			    WINHTTP_NO_PROXY_NAME,
			    WINHTTP_NO_PROXY_BYPASS,
			    0);
			if (session == nullptr) {
				session = WinHttpOpen(
				    L"HNKDesktopWidgets/1.0",
				    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
				    WINHTTP_NO_PROXY_NAME,
				    WINHTTP_NO_PROXY_BYPASS,
				    0);
			}
			if (session != nullptr) {
				WinHttpSetTimeouts(session, 4000, 4000, 8000, 8000);
				DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
				protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
				WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
			}

			bool cancelled = false;
			HINTERNET active = nullptr;
			{
				std::lock_guard<std::mutex> lock(state->mutex);
				if (!state->alive || state->generation != generation) {
					cancelled = true;
				} else {
					state->session = session;
					active = session;
					session = nullptr;
				}
			}
			if (cancelled) {
				if (session != nullptr) {
					WinHttpCloseHandle(session);
				}
				reading.location = location;
				reading.detail = L"Weather unavailable";
			} else {
				reading = FetchOpenMeteo(active, location, hasCoordinates, latitude, longitude);
			}

			HWND target = nullptr;
			HINTERNET ownedSession = nullptr;
			{
				std::lock_guard<std::mutex> lock(state->mutex);
				ownedSession = state->session;
				state->session = nullptr;
				if (state->alive && state->generation == generation) {
					state->pending = std::move(reading);
					state->pendingGeneration = generation;
					state->hasPending = true;
					target = state->window;
				}
				state->busy = false;
			}
			if (ownedSession != nullptr) {
				WinHttpCloseHandle(ownedSession);
			}
			if (target != nullptr) {
				PostMessageW(target, core::Widget::AsyncUpdateMessage, 0, 0);
			}
		});
	}

	void WeatherWidget::Paint(rendering::D2DRenderer& renderer, rendering::DirectWriteRenderer& text) {
		const float pad = 20.0f;
		const float width = renderer.Width();
		const float contentWidth = width > pad * 2.0f ? width - pad * 2.0f : width;
		float y = 16.0f;

		if (!reading_.ok) {
			const std::wstring title = reading_.location.empty() ? std::wstring(L"Weather") : reading_.location;
			renderer.DrawString(title, pad, y, contentWidth, 28.0f, text.TitleFormat());
			y += 36.0f;
			const std::wstring message = reading_.detail.empty() ? std::wstring(L"Weather unavailable") : reading_.detail;
			renderer.DrawString(message, pad, y, contentWidth, 48.0f, text.DetailFormat(), rendering::TextTone::Secondary);
			return;
		}

		if (reading_.sample) {
			renderer.DrawString(L"Sample data", pad, y, contentWidth, 20.0f, text.DetailFormat(), rendering::TextTone::Accent);
			y += 24.0f;
		}

		renderer.DrawString(reading_.location, pad, y, contentWidth, 22.0f, text.DetailFormat(), rendering::TextTone::Secondary);
		y += 24.0f;

		const float icon = std::min(76.0f, std::max(52.0f, renderer.Height() * 0.32f));
		const float temperatureWidth = std::max(0.0f, contentWidth - icon - 8.0f);
		renderer.DrawString(FormatDegrees(reading_.temperatureC), pad, y, temperatureWidth, icon, text.DisplayFormat());
		DrawWeatherIcon(renderer, IconForCode(reading_.weatherCode), width - pad - icon, y, icon);
		y += icon + 2.0f;
		renderer.DrawString(reading_.condition, pad, y, contentWidth, 28.0f, text.TitleFormat());
		y += 30.0f;
		renderer.DrawString(DetailLine(reading_), pad, y, contentWidth, 24.0f, text.DetailFormat(), rendering::TextTone::Secondary);
	}

}
