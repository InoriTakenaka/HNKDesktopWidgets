#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <atomic>
#include <memory>

namespace mw::rendering {

	class CompositionManager {
	public:
		CompositionManager();
		~CompositionManager();

		CompositionManager(const CompositionManager&) = delete;
		CompositionManager& operator=(const CompositionManager&) = delete;

		void Initialize(HWND window);
		void CreateBackdrop();
		void Resize(UINT width, UINT height);
		void Present();

		ID2D1DeviceContext* DeviceContext() const noexcept { return deviceContext_.Get(); }
		bool HasDevice() const noexcept { return device_ != nullptr; }
		bool HasSwapChain() const noexcept { return swapChain_ != nullptr; }
		bool HasDirect2D() const noexcept { return deviceContext_ != nullptr; }
		bool HasComposition() const noexcept { return composition_ != nullptr; }
		bool HasBlurBackdrop() const noexcept { return blurAttached_.load(); }

	private:
		struct CompositionState;

		void CreateDevice();
		void CreateSwapChain();
		void CreateDirect2D();
		void CreateTargetBitmap();
		void RunBackdropThread();

		HWND window_ = nullptr;
		UINT width_ = 0;
		UINT height_ = 0;
		std::atomic<bool> blurAttached_{ false };
		Microsoft::WRL::ComPtr<ID3D11Device> device_;
		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain_;
		Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory_;
		Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice_;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext> deviceContext_;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> targetBitmap_;
		std::unique_ptr<CompositionState> composition_;
	};

}
