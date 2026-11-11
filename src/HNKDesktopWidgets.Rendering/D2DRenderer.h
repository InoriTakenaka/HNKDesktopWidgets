#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <string_view>

namespace mw::rendering {

	enum class TextTone {
		Primary,
		Secondary,
		Accent,
	};

	struct DrawColor {
		float red = 1.0f;
		float green = 1.0f;
		float blue = 1.0f;
	};

	class D2DRenderer {
	public:
		explicit D2DRenderer(ID2D1DeviceContext* context);

		void BeginDraw();
		void Clear(float red, float green, float blue, float alpha);
		void SetPalette(DrawColor primary, DrawColor secondary, DrawColor accent);
		DrawColor PrimaryColor() const noexcept { return primary_; }
		DrawColor SecondaryColor() const noexcept { return secondary_; }
		DrawColor AccentColor() const noexcept { return accent_; }
		void SetBrushColor(float red, float green, float blue, float alpha);
		void FillRectangle(float x, float y, float width, float height, float red, float green, float blue, float alpha);
		void FillEllipse(float centerX, float centerY, float radiusX, float radiusY, float red, float green, float blue, float alpha);
		void DrawLine(float x1, float y1, float x2, float y2, float strokeWidth, float red, float green, float blue, float alpha);
		void DrawString(std::wstring_view text, float x, float y, float width, float height, IDWriteTextFormat* format, TextTone tone = TextTone::Primary);
		void EndDraw();

		float Width() const;
		float Height() const;

	private:
		void EnsureBrush();
		void UseTone(TextTone tone);

		ID2D1DeviceContext* context_;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_;
		DrawColor primary_{ 0.10f, 0.12f, 0.16f };
		DrawColor secondary_{ 0.28f, 0.32f, 0.38f };
		DrawColor accent_{ 0.00f, 0.40f, 0.78f };
	};

}
