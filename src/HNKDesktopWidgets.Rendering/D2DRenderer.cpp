#include "D2DRenderer.h"

#include <d2d1.h>
#include <wrl/client.h>

#include <stdexcept>

#pragma comment(lib, "d2d1.lib")

namespace mw::rendering {
	namespace {

		void ThrowIfFailed(HRESULT result, const char* message) {
			if (FAILED(result)) {
				char text[160]{};
				sprintf_s(text, "%s (0x%08X)", message, static_cast<unsigned>(result));
				throw std::runtime_error(text);
			}
		}

		D2D1_COLOR_F Opaque(DrawColor color) {
			return D2D1::ColorF(color.red, color.green, color.blue, 1.0f);
		}

	}

	D2DRenderer::D2DRenderer(ID2D1DeviceContext* context)
	    : context_(context) {
	}

	void D2DRenderer::EnsureBrush() {
		if (brush_ == nullptr) {
			ThrowIfFailed(context_->CreateSolidColorBrush(Opaque(primary_), &brush_), "CreateSolidColorBrush");
		}
	}

	void D2DRenderer::UseTone(TextTone tone) {
		EnsureBrush();
		const DrawColor color = tone == TextTone::Accent ? accent_ : tone == TextTone::Secondary ? secondary_ : primary_;
		brush_->SetColor(Opaque(color));
	}

	void D2DRenderer::BeginDraw() {
		context_->BeginDraw();
		context_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
		EnsureBrush();
	}

	void D2DRenderer::Clear(float red, float green, float blue, float alpha) {
		context_->Clear(D2D1::ColorF(red, green, blue, alpha));
	}

	void D2DRenderer::SetPalette(DrawColor primary, DrawColor secondary, DrawColor accent) {
		primary_ = primary;
		secondary_ = secondary;
		accent_ = accent;
		UseTone(TextTone::Primary);
	}

	void D2DRenderer::SetBrushColor(float red, float green, float blue, float alpha) {
		EnsureBrush();
		brush_->SetColor(D2D1::ColorF(red, green, blue, alpha));
	}

	void D2DRenderer::FillRectangle(float x, float y, float width, float height, float red, float green, float blue, float alpha) {
		EnsureBrush();
		brush_->SetColor(D2D1::ColorF(red, green, blue, alpha));
		context_->FillRectangle(D2D1::RectF(x, y, x + width, y + height), brush_.Get());
		UseTone(TextTone::Primary);
	}

	void D2DRenderer::FillEllipse(float centerX, float centerY, float radiusX, float radiusY, float red, float green, float blue, float alpha) {
		EnsureBrush();
		brush_->SetColor(D2D1::ColorF(red, green, blue, alpha));
		context_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(centerX, centerY), radiusX, radiusY), brush_.Get());
		UseTone(TextTone::Primary);
	}

	void D2DRenderer::DrawLine(float x1, float y1, float x2, float y2, float strokeWidth, float red, float green, float blue, float alpha) {
		EnsureBrush();
		brush_->SetColor(D2D1::ColorF(red, green, blue, alpha));
		context_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush_.Get(), strokeWidth);
		UseTone(TextTone::Primary);
	}

	void D2DRenderer::DrawString(std::wstring_view text, float x, float y, float width, float height, IDWriteTextFormat* format, TextTone tone) {
		UseTone(tone);
		const D2D1_RECT_F layout = D2D1::RectF(x, y, x + width, y + height);
		context_->DrawText(text.data(), static_cast<UINT32>(text.size()), format, layout, brush_.Get());
	}

	void D2DRenderer::EndDraw() {
		ThrowIfFailed(context_->EndDraw(), "EndDraw");
	}

	float D2DRenderer::Width() const {
		return context_->GetSize().width;
	}

	float D2DRenderer::Height() const {
		return context_->GetSize().height;
	}

}
