#pragma once

#include <dwrite.h>
#include <wrl/client.h>

namespace mw::rendering {

	class DirectWriteRenderer {
	public:
		void Initialize();

		IDWriteTextFormat* DisplayFormat() const noexcept { return displayFormat_.Get(); }
		IDWriteTextFormat* TitleFormat() const noexcept { return titleFormat_.Get(); }
		IDWriteTextFormat* TimeFormat() const noexcept { return timeFormat_.Get(); }
		IDWriteTextFormat* CaptionFormat() const noexcept { return captionFormat_.Get(); }
		IDWriteTextFormat* DetailFormat() const noexcept { return detailFormat_.Get(); }
		bool Initialized() const noexcept { return factory_ != nullptr; }

	private:
		Microsoft::WRL::ComPtr<IDWriteFactory> factory_;
		Microsoft::WRL::ComPtr<IDWriteInlineObject> displayEllipsis_;
		Microsoft::WRL::ComPtr<IDWriteInlineObject> titleEllipsis_;
		Microsoft::WRL::ComPtr<IDWriteInlineObject> timeEllipsis_;
		Microsoft::WRL::ComPtr<IDWriteInlineObject> captionEllipsis_;
		Microsoft::WRL::ComPtr<IDWriteInlineObject> detailEllipsis_;
		Microsoft::WRL::ComPtr<IDWriteTextFormat> displayFormat_;
		Microsoft::WRL::ComPtr<IDWriteTextFormat> titleFormat_;
		Microsoft::WRL::ComPtr<IDWriteTextFormat> timeFormat_;
		Microsoft::WRL::ComPtr<IDWriteTextFormat> captionFormat_;
		Microsoft::WRL::ComPtr<IDWriteTextFormat> detailFormat_;
	};

}
