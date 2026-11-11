#include "CompositionManager.h"

#include <unknwn.h>
#include <dispatcherqueue.h>
#include <d2d1.h>
#include <d2d1effects.h>
#include <wrl/implements.h>
#include <windows.graphics.effects.interop.h>
#include <windows.ui.composition.interop.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Effects.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

#include <stdexcept>
#include <string>
#include <thread>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "CoreMessaging.lib")

namespace mw::rendering {
	namespace {

		void ThrowIfFailed(HRESULT result, const char* message) {
			if (FAILED(result)) {
				char text[160]{};
				sprintf_s(text, "%s (0x%08X)", message, static_cast<unsigned>(result));
				throw std::runtime_error(text);
			}
		}

		class BackdropBlurEffect : public Microsoft::WRL::RuntimeClass<
		    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
		    ABI::Windows::Graphics::Effects::IGraphicsEffect,
		    ABI::Windows::Graphics::Effects::IGraphicsEffectSource,
		    ABI::Windows::Graphics::Effects::IGraphicsEffectD2D1Interop,
		    Microsoft::WRL::FtmBase> {
			InspectableClass(L"Windows.Graphics.Effects.IGraphicsEffect", BaseTrust);

		public:
			explicit BackdropBlurEffect(float deviation) {
				AttachProperty(winrt::Windows::Foundation::PropertyValue::CreateSingle(deviation), deviation_);
				AttachProperty(winrt::Windows::Foundation::PropertyValue::CreateUInt32(D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED), optimization_);
				AttachProperty(winrt::Windows::Foundation::PropertyValue::CreateUInt32(D2D1_BORDER_MODE_HARD), borderMode_);
			}

			void SetSource(ABI::Windows::Graphics::Effects::IGraphicsEffectSource* source) {
				source_ = source;
			}

			HRESULT STDMETHODCALLTYPE get_Name(HSTRING* name) override {
				if (name == nullptr) {
					return E_POINTER;
				}
				return WindowsCreateString(name_.c_str(), static_cast<UINT32>(name_.size()), name);
			}

			HRESULT STDMETHODCALLTYPE put_Name(HSTRING name) override {
				UINT32 length = 0;
				const wchar_t* raw = WindowsGetStringRawBuffer(name, &length);
				name_.assign(raw == nullptr ? L"" : raw, length);
				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE GetEffectId(GUID* id) override {
				if (id == nullptr) {
					return E_POINTER;
				}
				*id = GUID{ 0x1feb6d69, 0x2fe6, 0x4ac9, { 0x8c, 0x58, 0x1d, 0x7f, 0x93, 0xe7, 0xa6, 0xa5 } };
				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE GetNamedPropertyMapping(
			    LPCWSTR name,
			    UINT* index,
			    ABI::Windows::Graphics::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) override {
				if (name == nullptr || index == nullptr || mapping == nullptr) {
					return E_POINTER;
				}
				if (wcscmp(name, L"BlurAmount") == 0 || wcscmp(name, L"StandardDeviation") == 0) {
					*index = D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION;
					*mapping = ABI::Windows::Graphics::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
					return S_OK;
				}
				return E_INVALIDARG;
			}

			HRESULT STDMETHODCALLTYPE GetPropertyCount(UINT* count) override {
				if (count == nullptr) {
					return E_POINTER;
				}
				*count = 3;
				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE GetProperty(UINT index, ABI::Windows::Foundation::IPropertyValue** value) override {
				if (value == nullptr) {
					return E_POINTER;
				}
				*value = nullptr;
				if (index == D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION) {
					return deviation_.CopyTo(value);
				}
				if (index == D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION) {
					return optimization_.CopyTo(value);
				}
				if (index == D2D1_GAUSSIANBLUR_PROP_BORDER_MODE) {
					return borderMode_.CopyTo(value);
				}
				return E_INVALIDARG;
			}

			HRESULT STDMETHODCALLTYPE GetSource(UINT index, ABI::Windows::Graphics::Effects::IGraphicsEffectSource** source) override {
				if (source == nullptr) {
					return E_POINTER;
				}
				*source = nullptr;
				if (index != 0 || source_ == nullptr) {
					return E_INVALIDARG;
				}
				return source_.CopyTo(source);
			}

			HRESULT STDMETHODCALLTYPE GetSourceCount(UINT* count) override {
				if (count == nullptr) {
					return E_POINTER;
				}
				*count = 1;
				return S_OK;
			}

		private:
			static void AttachProperty(
			    winrt::Windows::Foundation::IInspectable const& property,
			    Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IPropertyValue>& destination) {
				auto value = property.as<winrt::Windows::Foundation::IPropertyValue>();
				destination.Attach(reinterpret_cast<ABI::Windows::Foundation::IPropertyValue*>(winrt::detach_abi(value)));
			}

			std::wstring name_{ L"BackdropBlur" };
			Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IPropertyValue> deviation_;
			Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IPropertyValue> optimization_;
			Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IPropertyValue> borderMode_;
			Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Effects::IGraphicsEffectSource> source_;
		};

	}

	struct CompositionManager::CompositionState {
		std::thread thread;
		std::atomic<DWORD> threadId{ 0 };
		std::string error;
	};

	CompositionManager::CompositionManager() = default;

	CompositionManager::~CompositionManager() {
		if (deviceContext_ != nullptr) {
			deviceContext_->SetTarget(nullptr);
		}
		if (composition_ != nullptr && composition_->thread.joinable()) {
			DWORD threadId = composition_->threadId.load();
			const DWORD started = GetTickCount();
			while (threadId == 0 && GetTickCount() - started < 2000) {
				Sleep(10);
				threadId = composition_->threadId.load();
			}
			if (threadId != 0) {
				PostThreadMessageW(threadId, WM_QUIT, 0, 0);
			}
			composition_->thread.join();
		}
		composition_.reset();
	}

	void CompositionManager::Initialize(HWND window) {
		window_ = window;
		RECT client{};
		GetClientRect(window_, &client);
		width_ = static_cast<UINT>(client.right - client.left);
		height_ = static_cast<UINT>(client.bottom - client.top);
		if (width_ == 0 || height_ == 0) {
			throw std::runtime_error("Widget window has an empty client area.");
		}

		CreateDevice();
		CreateSwapChain();
		CreateDirect2D();
		CreateTargetBitmap();
	}

	void CompositionManager::CreateDevice() {
		const D3D_FEATURE_LEVEL levels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_0,
		};
		const HRESULT hardware = D3D11CreateDevice(
		    nullptr,
		    D3D_DRIVER_TYPE_HARDWARE,
		    nullptr,
		    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		    levels,
		    ARRAYSIZE(levels),
		    D3D11_SDK_VERSION,
		    &device_,
		    nullptr,
		    nullptr);
		if (SUCCEEDED(hardware)) {
			return;
		}

		ThrowIfFailed(D3D11CreateDevice(
		                  nullptr,
		                  D3D_DRIVER_TYPE_WARP,
		                  nullptr,
		                  D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		                  levels,
		                  ARRAYSIZE(levels),
		                  D3D11_SDK_VERSION,
		                  &device_,
		                  nullptr,
		                  nullptr),
		    "D3D11CreateDevice");
	}

	void CompositionManager::CreateSwapChain() {
		Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
		ThrowIfFailed(device_.As(&dxgiDevice), "Query DXGI device");

		Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
		ThrowIfFailed(dxgiDevice->GetAdapter(&adapter), "Get DXGI adapter");

		Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
		ThrowIfFailed(adapter->GetParent(IID_PPV_ARGS(&factory)), "Get DXGI factory");

		DXGI_SWAP_CHAIN_DESC1 description{};
		description.Width = width_;
		description.Height = height_;
		description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		description.SampleDesc.Count = 1;
		description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		description.BufferCount = 2;
		description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
		description.Scaling = DXGI_SCALING_STRETCH;
		ThrowIfFailed(factory->CreateSwapChainForComposition(device_.Get(), &description, nullptr, &swapChain_),
		    "CreateSwapChainForComposition");
	}

	void CompositionManager::CreateDirect2D() {
		D2D1_FACTORY_OPTIONS options{};
		ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, d2dFactory_.GetAddressOf()),
		    "D2D1CreateFactory");

		Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
		ThrowIfFailed(device_.As(&dxgiDevice), "Query DXGI device for Direct2D");
		ThrowIfFailed(d2dFactory_->CreateDevice(dxgiDevice.Get(), &d2dDevice_), "Create Direct2D device");
		ThrowIfFailed(d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &deviceContext_),
		    "Create Direct2D device context");
	}

	void CompositionManager::CreateTargetBitmap() {
		Microsoft::WRL::ComPtr<IDXGISurface> surface;
		ThrowIfFailed(swapChain_->GetBuffer(0, IID_PPV_ARGS(&surface)), "Get swap chain buffer");

		const D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
		    D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
		ThrowIfFailed(deviceContext_->CreateBitmapFromDxgiSurface(surface.Get(), &properties, &targetBitmap_),
		    "CreateBitmapFromDxgiSurface");
		deviceContext_->SetTarget(targetBitmap_.Get());

		const float dpi = static_cast<float>(GetDpiForWindow(window_));
		deviceContext_->SetDpi(dpi == 0.0f ? 96.0f : dpi, dpi == 0.0f ? 96.0f : dpi);
	}

	void CompositionManager::CreateBackdrop() {
		if (composition_ != nullptr) {
			return;
		}
		composition_ = std::make_unique<CompositionState>();
		composition_->thread = std::thread([this] {
			RunBackdropThread();
		});
	}

	void CompositionManager::RunBackdropThread() {
		const HWND window = window_;
		MSG message{};
		PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
		composition_->threadId.store(GetCurrentThreadId());
		winrt::init_apartment(winrt::apartment_type::single_threaded);
		DispatcherQueueOptions queueOptions{};
		queueOptions.dwSize = sizeof(queueOptions);
		queueOptions.threadType = DQTYPE_THREAD_CURRENT;
		queueOptions.apartmentType = DQTAT_COM_STA;
		winrt::com_ptr<ABI::Windows::System::IDispatcherQueueController> queueController;
		try {
			ThrowIfFailed(CreateDispatcherQueueController(queueOptions, queueController.put()),
			    "CreateDispatcherQueueController");
			auto compositor = winrt::Windows::UI::Composition::Compositor();
			winrt::com_ptr<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop> desktopInterop;
			ThrowIfFailed(winrt::get_unknown(compositor)->QueryInterface(
			                  __uuidof(ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop),
			                  desktopInterop.put_void()),
			    "Query compositor desktop interop");

			ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget* rawTarget = nullptr;
			ThrowIfFailed(desktopInterop->CreateDesktopWindowTarget(window, TRUE, &rawTarget),
			    "CreateDesktopWindowTarget");
			auto target = winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget(rawTarget, winrt::take_ownership_from_abi);

			auto root = compositor.CreateContainerVisual();
			const winrt::Windows::Foundation::Numerics::float2 windowSize{
			    static_cast<float>(width_),
			    static_cast<float>(height_),
			};
			root.Size(windowSize);
			target.Root(root);

			constexpr float blurDeviation = 4.0f;
			auto effect = Microsoft::WRL::Make<BackdropBlurEffect>(blurDeviation);
			auto source = winrt::Windows::UI::Composition::CompositionEffectSourceParameter(L"backdrop");
			Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Effects::IGraphicsEffectSource> sourceAbi;
			ThrowIfFailed(winrt::get_unknown(source)->QueryInterface(IID_PPV_ARGS(&sourceAbi)),
			    "Query effect source");
			effect->SetSource(sourceAbi.Get());
			ABI::Windows::Graphics::Effects::IGraphicsEffect* rawEffect = nullptr;
			ThrowIfFailed(effect.CopyTo(&rawEffect), "Query graphics effect");
			ABI::Windows::UI::Composition::ICompositor* compositorAbi = nullptr;
			ThrowIfFailed(winrt::get_unknown(compositor)->QueryInterface(IID_PPV_ARGS(&compositorAbi)),
			    "Query compositor");
			ABI::Windows::UI::Composition::ICompositionEffectFactory* factoryRaw = nullptr;
			const HRESULT factoryHr = compositorAbi->CreateEffectFactory(rawEffect, &factoryRaw);
			compositorAbi->Release();
			rawEffect->Release();
			ThrowIfFailed(factoryHr, "CreateEffectFactory");
			auto factory = winrt::Windows::UI::Composition::CompositionEffectFactory(factoryRaw, winrt::take_ownership_from_abi);
			auto blurred = factory.CreateBrush();
			blurred.SetSourceParameter(L"backdrop", compositor.CreateHostBackdropBrush());

			auto backdrop = compositor.CreateSpriteVisual();
			backdrop.Size(windowSize);
			backdrop.Brush(blurred);

			winrt::com_ptr<ABI::Windows::UI::Composition::ICompositorInterop> compositorInterop;
			ThrowIfFailed(winrt::get_unknown(compositor)->QueryInterface(
			                  __uuidof(ABI::Windows::UI::Composition::ICompositorInterop),
			                  compositorInterop.put_void()),
			    "Query compositor interop");
			winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionSurface> surfaceInterop;
			ThrowIfFailed(compositorInterop->CreateCompositionSurfaceForSwapChain(swapChain_.Get(), surfaceInterop.put()),
			    "CreateCompositionSurfaceForSwapChain");

			auto surfaceBrush = compositor.CreateSurfaceBrush(
			    surfaceInterop.as<winrt::Windows::UI::Composition::ICompositionSurface>());
			surfaceBrush.Stretch(winrt::Windows::UI::Composition::CompositionStretch::Fill);
			auto content = compositor.CreateSpriteVisual();
			content.Size(windowSize);
			content.Brush(surfaceBrush);

			root.Children().InsertAtBottom(backdrop);
			root.Children().InsertAtTop(content);
			blurAttached_.store(true);
			PostMessageW(window, WM_APP + 22, 1, 0);

			while (GetMessageW(&message, nullptr, 0, 0) > 0) {
				if (message.hwnd == nullptr && message.message == WM_APP + 1) {
					const winrt::Windows::Foundation::Numerics::float2 size{
					    static_cast<float>(LOWORD(message.wParam)),
					    static_cast<float>(HIWORD(message.wParam)),
					};
					root.Size(size);
					backdrop.Size(size);
					content.Size(size);
					continue;
				}
				DispatchMessageW(&message);
			}
		} catch (const std::exception& exception) {
			composition_->error = exception.what();
			PostMessageW(window, WM_APP + 22, 0, 0);
		}
		winrt::uninit_apartment();
	}

	void CompositionManager::Resize(UINT width, UINT height) {
		if (swapChain_ == nullptr || width == 0 || height == 0 || (width == width_ && height == height_)) {
			return;
		}

		width_ = width;
		height_ = height;
		deviceContext_->SetTarget(nullptr);
		targetBitmap_.Reset();
		ThrowIfFailed(swapChain_->ResizeBuffers(0, width_, height_, DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers");
		CreateTargetBitmap();
		const DWORD threadId = composition_ == nullptr ? 0 : composition_->threadId.load();
		if (threadId != 0) {
			const WPARAM packed = static_cast<WPARAM>(width_ & 0xFFFF) | (static_cast<WPARAM>(height_ & 0xFFFF) << 16);
			PostThreadMessageW(threadId, WM_APP + 1, packed, 0);
		}
	}

	void CompositionManager::Present() {
		ThrowIfFailed(swapChain_->Present(1, 0), "Present");
	}

}
