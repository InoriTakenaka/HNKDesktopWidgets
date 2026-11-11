#include "WeatherData.h"

#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>
#include <system_error>

#pragma comment(lib, "winhttp.lib")

namespace mw::widgets {
	namespace {

		std::wstring Widen(std::string_view text) {
			if (text.empty()) {
				return {};
			}
			const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
			if (size <= 0) {
				return {};
			}
			std::wstring wide(static_cast<size_t>(size), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
			return wide;
		}

		std::string NarrowUtf8(std::wstring_view text) {
			if (text.empty()) {
				return {};
			}
			const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
			if (size <= 0) {
				return {};
			}
			std::string utf8(static_cast<size_t>(size), '\0');
			WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), size, nullptr, nullptr);
			return utf8;
		}

		std::wstring UrlEncode(std::wstring_view text) {
			const std::string utf8 = NarrowUtf8(text);
			std::wstring encoded;
			encoded.reserve(utf8.size() * 3);
			for (const unsigned char character : utf8) {
				if ((character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '-' || character == '_' || character == '.' || character == '~') {
					encoded.push_back(static_cast<wchar_t>(character));
				} else if (character == ' ') {
					encoded.append(L"%20");
				} else {
					wchar_t buffer[8]{};
					swprintf_s(buffer, L"%%%02X", character);
					encoded.append(buffer);
				}
			}
			return encoded;
		}

		class InternetHandle {
		public:
			InternetHandle() = default;
			explicit InternetHandle(HINTERNET value)
			    : value_(value) {
			}
			~InternetHandle() {
				reset();
			}
			InternetHandle(const InternetHandle&) = delete;
			InternetHandle& operator=(const InternetHandle&) = delete;
			InternetHandle(InternetHandle&& other) noexcept
			    : value_(other.value_) {
				other.value_ = nullptr;
			}
			InternetHandle& operator=(InternetHandle&& other) noexcept {
				if (this != &other) {
					reset();
					value_ = other.value_;
					other.value_ = nullptr;
				}
				return *this;
			}

			HINTERNET get() const noexcept { return value_; }
			explicit operator bool() const noexcept { return value_ != nullptr; }
			void reset(HINTERNET value = nullptr) {
				if (value_ != nullptr) {
					WinHttpCloseHandle(value_);
				}
				value_ = value;
			}

		private:
			HINTERNET value_ = nullptr;
		};

		std::optional<double> FindJsonNumber(std::string_view json, std::string_view key) {
			const std::string pattern = std::string("\"") + std::string(key) + "\":";
			size_t position = 0;
			while ((position = json.find(pattern, position)) != std::string_view::npos) {
				position += pattern.size();
				while (position < json.size() && (json[position] == ' ' || json[position] == '\t' || json[position] == '\r' || json[position] == '\n')) {
					++position;
				}
				if (position >= json.size()) {
					break;
				}
				const char head = json[position];
				if (head == '-' || (head >= '0' && head <= '9')) {
					const std::string suffix(json.substr(position));
					double value = 0;
					const std::from_chars_result parsed = std::from_chars(suffix.data(), suffix.data() + suffix.size(), value);
					if (parsed.ec == std::errc{}) {
						return value;
					}
				}
			}
			return std::nullopt;
		}

		std::optional<std::string> FindJsonString(std::string_view json, std::string_view key) {
			const std::string pattern = std::string("\"") + std::string(key) + "\":";
			size_t position = 0;
			while ((position = json.find(pattern, position)) != std::string_view::npos) {
				position += pattern.size();
				while (position < json.size() && (json[position] == ' ' || json[position] == '\t')) {
					++position;
				}
				if (position >= json.size() || json[position] != '"') {
					continue;
				}
				++position;
				std::string value;
				while (position < json.size()) {
					const char character = json[position++];
					if (character == '\\' && position < json.size()) {
						value.push_back(json[position++]);
						continue;
					}
					if (character == '"') {
						return value;
					}
					value.push_back(character);
				}
			}
			return std::nullopt;
		}

		struct HttpResponse {
			DWORD status = 0;
			std::string body;
		};

		HttpResponse HttpGet(HINTERNET session, const wchar_t* host, const std::wstring& path) {
			if (session == nullptr) {
				throw std::runtime_error("WinHTTP session is not open");
			}
			InternetHandle connection(WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0));
			if (!connection) {
				throw std::runtime_error("WinHttpConnect failed (" + std::to_string(GetLastError()) + ")");
			}
			InternetHandle request(WinHttpOpenRequest(
			    connection.get(),
			    L"GET",
			    path.c_str(),
			    nullptr,
			    WINHTTP_NO_REFERER,
			    WINHTTP_DEFAULT_ACCEPT_TYPES,
			    WINHTTP_FLAG_SECURE));
			if (!request) {
				throw std::runtime_error("WinHttpOpenRequest failed (" + std::to_string(GetLastError()) + ")");
			}
			if (WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE) {
				throw std::runtime_error("WinHttpSendRequest failed (" + std::to_string(GetLastError()) + ")");
			}
			if (WinHttpReceiveResponse(request.get(), nullptr) == FALSE) {
				throw std::runtime_error("WinHttpReceiveResponse failed (" + std::to_string(GetLastError()) + ")");
			}

			HttpResponse response;
			DWORD statusSize = sizeof(response.status);
			if (WinHttpQueryHeaders(
			        request.get(),
			        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			        WINHTTP_HEADER_NAME_BY_INDEX,
			        &response.status,
			        &statusSize,
			        WINHTTP_NO_HEADER_INDEX)
			    == FALSE) {
				throw std::runtime_error("WinHttpQueryHeaders failed (" + std::to_string(GetLastError()) + ")");
			}

			// WinHttpQueryDataAvailable can fail with E_ABORT on HTTP/2 responses.
			// Reading fixed-size chunks stops cleanly when a chunk comes back empty.
			for (;;) {
				char chunk[8192]{};
				DWORD read = 0;
				if (WinHttpReadData(request.get(), chunk, sizeof(chunk), &read) == FALSE) {
					throw std::runtime_error("WinHttpReadData failed (" + std::to_string(GetLastError()) + ") after HTTP " + std::to_string(response.status));
				}
				if (read == 0) {
					break;
				}
				if (response.body.size() + read > 1024u * 1024u) {
					throw std::runtime_error("Weather response is too large");
				}
				response.body.append(chunk, chunk + read);
			}
			return response;
		}

		WeatherReading Failed(std::wstring location, std::wstring detail) {
			WeatherReading reading;
			reading.location = std::move(location);
			reading.detail = std::move(detail);
			return reading;
		}

	}

	std::wstring WeatherConditionName(int weatherCode) {
		switch (weatherCode) {
		case 0:
			return L"Clear";
		case 1:
			return L"Mainly clear";
		case 2:
			return L"Partly cloudy";
		case 3:
			return L"Overcast";
		case 45:
		case 48:
			return L"Fog";
		case 51:
		case 53:
		case 55:
			return L"Drizzle";
		case 56:
		case 57:
			return L"Freezing drizzle";
		case 61:
			return L"Light rain";
		case 63:
			return L"Rain";
		case 65:
			return L"Heavy rain";
		case 66:
		case 67:
			return L"Freezing rain";
		case 71:
			return L"Light snow";
		case 73:
			return L"Snow";
		case 75:
			return L"Heavy snow";
		case 77:
			return L"Snow grains";
		case 80:
			return L"Light showers";
		case 81:
			return L"Showers";
		case 82:
			return L"Heavy showers";
		case 85:
		case 86:
			return L"Snow showers";
		case 95:
			return L"Thunderstorm";
		case 96:
		case 99:
			return L"Thunderstorm and hail";
		default:
			return L"Unknown conditions";
		}
	}

	WeatherReading MakeSampleWeather(std::wstring location) {
		WeatherReading reading;
		reading.ok = true;
		reading.sample = true;
		reading.location = location.empty() ? std::wstring(L"Sample City") : std::move(location);
		reading.condition = L"Partly cloudy";
		reading.detail = L"Sample data";
		reading.weatherCode = 2;
		reading.temperatureC = 22;
		reading.feelsLikeC = 21;
		reading.humidityPercent = 55;
		reading.windKmh = 12;
		reading.hasFeelsLike = true;
		reading.hasHumidity = true;
		reading.hasWind = true;
		return reading;
	}

	bool ParseOpenMeteoLocation(std::string_view json, double& latitude, double& longitude, std::wstring& name) {
		const std::optional<double> parsedLatitude = FindJsonNumber(json, "latitude");
		const std::optional<double> parsedLongitude = FindJsonNumber(json, "longitude");
		const std::optional<std::string> parsedName = FindJsonString(json, "name");
		if (!parsedLatitude.has_value() || !parsedLongitude.has_value() || !parsedName.has_value()) {
			return false;
		}
		latitude = *parsedLatitude;
		longitude = *parsedLongitude;
		name = Widen(*parsedName);
		return !name.empty();
	}

	WeatherReading ParseOpenMeteoForecast(std::string_view json, std::wstring location) {
		WeatherReading reading;
		reading.location = std::move(location);
		const std::optional<double> temperature = FindJsonNumber(json, "temperature_2m");
		const std::optional<double> code = FindJsonNumber(json, "weather_code");
		if (!temperature.has_value() || !code.has_value()) {
			reading.detail = L"Unexpected weather response";
			return reading;
		}

		reading.ok = true;
		reading.temperatureC = *temperature;
		reading.weatherCode = static_cast<int>(std::lround(*code));
		reading.condition = WeatherConditionName(reading.weatherCode);
		if (const std::optional<double> feels = FindJsonNumber(json, "apparent_temperature")) {
			reading.feelsLikeC = *feels;
			reading.hasFeelsLike = true;
		}
		if (const std::optional<double> humidity = FindJsonNumber(json, "relative_humidity_2m")) {
			reading.humidityPercent = static_cast<int>(std::lround(*humidity));
			reading.hasHumidity = true;
		}
		if (const std::optional<double> wind = FindJsonNumber(json, "wind_speed_10m")) {
			reading.windKmh = *wind;
			reading.hasWind = true;
		}
		return reading;
	}

	WeatherReading FetchOpenMeteo(std::wstring location, bool hasCoordinates, double latitude, double longitude) {
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
		const WeatherReading reading = FetchOpenMeteo(session, std::move(location), hasCoordinates, latitude, longitude);
		if (session != nullptr) {
			WinHttpCloseHandle(session);
		}
		return reading;
	}

	WeatherReading FetchOpenMeteo(HINTERNET session, std::wstring location, bool hasCoordinates, double latitude, double longitude) {
		try {
			if (session == nullptr) {
				return Failed(std::move(location), L"Weather unavailable: HTTP session was not created");
			}
			if (!hasCoordinates) {
				if (location.empty()) {
					return Failed({}, L"Weather location is not configured");
				}
				const HttpResponse geocode = HttpGet(
				    session,
				    L"geocoding-api.open-meteo.com",
				    L"/v1/search?name=" + UrlEncode(location) + L"&count=1&language=en&format=json");
				if (geocode.status != 200) {
					return Failed(location, L"Weather unavailable: location lookup returned HTTP " + std::to_wstring(geocode.status));
				}
				std::wstring resolvedName;
				if (!ParseOpenMeteoLocation(geocode.body, latitude, longitude, resolvedName)) {
					return Failed(std::move(location), L"Weather unavailable: location was not found");
				}
				location = std::move(resolvedName);
			} else if (location.empty()) {
				wchar_t coordinates[64]{};
				swprintf_s(coordinates, L"%.2f, %.2f", latitude, longitude);
				location = coordinates;
			}

			wchar_t path[512]{};
			swprintf_s(
			    path,
			    L"/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m&wind_speed_unit=kmh&timezone=auto",
			    latitude,
			    longitude);
			const HttpResponse forecast = HttpGet(session, L"api.open-meteo.com", path);
			if (forecast.status != 200) {
				return Failed(std::move(location), L"Weather unavailable: forecast returned HTTP " + std::to_wstring(forecast.status));
			}
			WeatherReading reading = ParseOpenMeteoForecast(forecast.body, location);
			if (!reading.ok && reading.detail.empty()) {
				reading.detail = L"Unexpected weather response";
			}
			return reading;
		} catch (const std::exception& exception) {
			return Failed(std::move(location), L"Weather unavailable: " + Widen(exception.what()));
		}
	}

}
