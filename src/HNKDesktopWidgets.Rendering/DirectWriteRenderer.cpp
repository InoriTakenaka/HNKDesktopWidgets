#include "DirectWriteRenderer.h"

#include <stdexcept>

#pragma comment(lib, "dwrite.lib")

namespace mw::rendering {
	namespace {

		void ThrowIfFailed(HRESULT result, const char* message) {
			if (FAILED(result)) {
				char text[160]{};
				sprintf_s(text, "%s (0x%08X)", message, static_cast<unsigned>(result));
				throw std::runtime_error(text);
			}
		}

		Microsoft::WRL::ComPtr<IDWriteTextFormat> CreateFormat(
		    IDWriteFactory* factory,
		    Microsoft::WRL::ComPtr<IDWriteInlineObject>& ellipsis,
		    float size,
		    DWRITE_FONT_WEIGHT weight,
		    DWRITE_PARAGRAPH_ALIGNMENT alignment) {
			Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
			ThrowIfFailed(factory->CreateTextFormat(
			                  L"Segoe UI",
			                  nullptr,
			                  weight,
			                  DWRITE_FONT_STYLE_NORMAL,
			                  DWRITE_FONT_STRETCH_NORMAL,
			                  size,
			                  L"en-us",
			                  &format),
			    "CreateTextFormat");
			format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			format->SetParagraphAlignment(alignment);
			if (SUCCEEDED(factory->CreateEllipsisTrimmingSign(format.Get(), &ellipsis))) {
				DWRITE_TRIMMING trimming{};
				trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
				format->SetTrimming(&trimming, ellipsis.Get());
			}
			return format;
		}

	}

	void DirectWriteRenderer::Initialize() {
		ThrowIfFailed(DWriteCreateFactory(
		                  DWRITE_FACTORY_TYPE_SHARED,
		                  __uuidof(IDWriteFactory),
		                  reinterpret_cast<IUnknown**>(factory_.GetAddressOf())),
		    "DWriteCreateFactory");
		displayFormat_ = CreateFormat(factory_.Get(), displayEllipsis_, 52.0f, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		titleFormat_ = CreateFormat(factory_.Get(), titleEllipsis_, 20.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		timeFormat_ = CreateFormat(factory_.Get(), timeEllipsis_, 48.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		captionFormat_ = CreateFormat(factory_.Get(), captionEllipsis_, 16.0f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		detailFormat_ = CreateFormat(factory_.Get(), detailEllipsis_, 14.0f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

}
