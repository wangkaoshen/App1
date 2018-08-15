#include "pch.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include <windows.ui.xaml.media.dxinterop.h>

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;

namespace DisplayMetrics
{
	// 高解析度顯示畫面可能需要大量的 GPU 與電力才可呈現。
	// 例如，如果遊戲嘗試以每秒 60 個畫面格呈現以求逼真，
	// 高解析度手機的電池壽命就可能會過低。
	// 建議您深思熟慮後，再決定跨所有平台和尺寸以最高逼真度
	// 加以呈現。
	static const bool SupportHighResolutions = false;

	// 定義「高解析度」顯示畫面的預設臨界值。如果超出臨界值，
	// 且 SupportHighResolutions 為 false，則維度的大小
	// 就會調整 50%。
	static const float DpiThreshold = 192.0f;		// 200% 的標準桌面顯示。
	static const float WidthThreshold = 1920.0f;	// 寬度 1080p。
	static const float HeightThreshold = 1080.0f;	// 高度 1080p。
};

// 用來計算螢幕旋轉的常數。
namespace ScreenRotation
{
	// 0 度 Z 旋轉
	static const XMFLOAT4X4 Rotation0( 
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 90 度 Z 旋轉
	static const XMFLOAT4X4 Rotation90(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 180 度 Z 旋轉
	static const XMFLOAT4X4 Rotation180(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 270 度 Z 旋轉
	static const XMFLOAT4X4 Rotation270( 
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);
};

// DeviceResources 的建構函式。
DX::DeviceResources::DeviceResources() : 
	m_screenViewport(),
	m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
	m_d3dRenderTargetSize(),
	m_outputSize(),
	m_logicalSize(),
	m_nativeOrientation(DisplayOrientations::None),
	m_currentOrientation(DisplayOrientations::None),
	m_dpi(-1.0f),
	m_effectiveDpi(-1.0f),
	m_compositionScaleX(1.0f),
	m_compositionScaleY(1.0f),
	m_deviceNotify(nullptr)
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

// 設定與 Direct3D 裝置無關的資源。
void DX::DeviceResources::CreateDeviceIndependentResources()
{
	// 初始化 Direct2D 資源。
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	// 如果專案在偵錯組建中，透過 SDK 層啟用 Direct2D 偵錯。
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	// 初始化 Direct2D Factory。
	DX::ThrowIfFailed(
		D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory3),
			&options,
			&m_d2dFactory
			)
		);

	// 初始化 DirectWrite Factory。
	DX::ThrowIfFailed(
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory3),
			&m_dwriteFactory
			)
		);

	// 初始化 Windows 影像處理元件 (WIC) Factory。
	DX::ThrowIfFailed(
		CoCreateInstance(
			CLSID_WICImagingFactory2,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wicFactory)
			)
		);
}

// 設定 Direct3D 裝置，並儲存它的控制代碼和裝置內容。
void DX::DeviceResources::CreateDeviceResources() 
{
	// 這個旗標會加入支援色彩頻道順序異於預設
	// 應用程式開發介面的表面。必須這樣才能與 Direct2D 相容。
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	if (DX::SdkLayersAvailable())
	{
		// 如果專案在偵錯組建中，使用這個旗標透過 SDK 層啟用偵錯。
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	// 這個陣列定義這個應用程式將支援的一組 DirectX 硬體功能層級。
	// 注意，應保留順序。
	// 不要忘記在描述中宣告應用程式的最低必要功能
	// 層級。除非另外指定，否則所有應用程式都必須支援 9.1。
	D3D_FEATURE_LEVEL featureLevels[] = 
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// 建立 Direct3D 11 應用程式開發介面裝置物件和對應的內容。
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

	HRESULT hr = D3D11CreateDevice(
		nullptr,					// 指定 nullptr 以使用預設配接器。
		D3D_DRIVER_TYPE_HARDWARE,	// 使用硬體圖形驅動程式建立裝置。
		0,							// 應該是 0，除非驅動程式是 D3D_DRIVER_TYPE_SOFTWARE。
		creationFlags,				// 設定偵錯和 Direct2D 相容性旗標。
		featureLevels,				// 這個應用程式可支援的功能層級清單。
		ARRAYSIZE(featureLevels),	// 上述清單的大小。
		D3D11_SDK_VERSION,			// 若是 Windows 市集應用程式，這一項一定設為 D3D11_SDK_VERSION。
		&device,					// 傳回建立的 Direct3D 裝置。
		&m_d3dFeatureLevel,			// 傳回建立的裝置功能層級。
		&context					// 傳回裝置直接內容。
		);

	if (FAILED(hr))
	{
		// 如果初始化失敗，則切換回 WARP 裝置。
		// 如需有關 WARP 的詳細資訊，請參閱: 
		// https://go.microsoft.com/fwlink/?LinkId=286690
		DX::ThrowIfFailed(
			D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_WARP, // 建立 WARP 裝置，而不是硬體裝置。
				0,
				creationFlags,
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				&device,
				&m_d3dFeatureLevel,
				&context
				)
			);
	}

	// 儲存 Direct3D 11.3 API 裝置和直接內容的指標。
	DX::ThrowIfFailed(
		device.As(&m_d3dDevice)
		);

	DX::ThrowIfFailed(
		context.As(&m_d3dContext)
		);

	// 建立 Direct2D 裝置物件和對應的內容。
	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(
		m_d3dDevice.As(&dxgiDevice)
		);

	DX::ThrowIfFailed(
		m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
		);

	DX::ThrowIfFailed(
		m_d2dDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			&m_d2dContext
			)
		);
}

// 每次視窗大小變更時都必須重新建立這些資源。
void DX::DeviceResources::CreateWindowSizeDependentResources() 
{
	// 清除之前視窗大小專屬的內容。
	ID3D11RenderTargetView* nullViews[] = {nullptr};
	m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	m_d2dContext->SetTarget(nullptr);
	m_d2dTargetBitmap = nullptr;
	m_d3dDepthStencilView = nullptr;
	m_d3dContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

	UpdateRenderTargetSize();

	// 交換鏈結的寬度和高度必須以視窗
	// 原生方向的寬度和高度為基礎。如果視窗不是採原生方向，
	// 維度將反轉。
	DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

	bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	m_d3dRenderTargetSize.Width = swapDimensions ? m_outputSize.Height : m_outputSize.Width;
	m_d3dRenderTargetSize.Height = swapDimensions ? m_outputSize.Width : m_outputSize.Height;

	if (m_swapChain != nullptr)
	{
		// 如果交換鏈結已經存在，即調整它的大小。
		HRESULT hr = m_swapChain->ResizeBuffers(
			2, // 雙重緩衝的交換鏈結。
			lround(m_d3dRenderTargetSize.Width),
			lround(m_d3dRenderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
			);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// 如果因任何原因而移除裝置，就必須建立新裝置和交換鏈結。
			HandleDeviceLost();

			// 現在已完成一切設定。不再繼續執行這個方法。HandleDeviceLost 會重新進入這個方法 
			// 並正確設定新裝置。
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
	}
	else
	{
		// 否則，使用與現有 Direct3D 裝置相同的配接器建立新的。
		DXGI_SCALING scaling = DisplayMetrics::SupportHighResolutions ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};

		swapChainDesc.Width = lround(m_d3dRenderTargetSize.Width);		// 比對視窗的大小。
		swapChainDesc.Height = lround(m_d3dRenderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;				// 這是最常見的交換鏈結格式。
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;								// 不使用多重取樣。
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;									// 使用雙重緩衝將延遲降到最低。
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// 所有 Windows 市集應用程式都必須使用 _FLIP_ SwapEffects。
		swapChainDesc.Flags = 0;
		swapChainDesc.Scaling = scaling;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		// 這個序列中包含用來建立上述 Direct3D 裝置的 DXGI Factory。
		ComPtr<IDXGIDevice3> dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		ComPtr<IDXGIFactory4> dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
			);

		// 使用 XAML Interop 時，必須建立交換鏈結以進行組合。
		ComPtr<IDXGISwapChain1> swapChain;
		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForComposition(
				m_d3dDevice.Get(),
				&swapChainDesc,
				nullptr,
				&swapChain
				)
			);

		DX::ThrowIfFailed(
			swapChain.As(&m_swapChain)
			);

		// 將交換鏈結與 SwapChainPanel 產生關聯
		// UI 變更必須分派回 UI 執行緒
		m_swapChainPanel->Dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
		{
			// 正在取回 SwapChainPanel 的原生介面
			ComPtr<ISwapChainPanelNative> panelNative;
			DX::ThrowIfFailed(
				reinterpret_cast<IUnknown*>(m_swapChainPanel)->QueryInterface(IID_PPV_ARGS(&panelNative))
				);

			DX::ThrowIfFailed(
				panelNative->SetSwapChain(m_swapChain.Get())
				);
		}, CallbackContext::Any));

		// 確定 DXGI 不會一次將多個框架排入佇列。這樣可同時降低延遲，並
		// 確保應用程式只會在每次 VSync 後轉譯，使耗電降到最低。
		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);
	}

	// 為交換鏈結設定正確的方向，並產生 2D 和
	// 3D 矩陣轉換，以轉譯成旋轉的交換鏈結。
	// 注意，2D 和 3D 轉換的旋轉角度不同。
	// 這是座標空間不同所致。此外，
	// 3D 矩陣是明確指定，以避免進位誤差。

	switch (displayRotation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
		m_orientationTransform2D = Matrix3x2F::Identity();
		m_orientationTransform3D = ScreenRotation::Rotation0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		m_orientationTransform2D = 
			Matrix3x2F::Rotation(90.0f) *
			Matrix3x2F::Translation(m_logicalSize.Height, 0.0f);
		m_orientationTransform3D = ScreenRotation::Rotation270;
		break;

	case DXGI_MODE_ROTATION_ROTATE180:
		m_orientationTransform2D = 
			Matrix3x2F::Rotation(180.0f) *
			Matrix3x2F::Translation(m_logicalSize.Width, m_logicalSize.Height);
		m_orientationTransform3D = ScreenRotation::Rotation180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		m_orientationTransform2D = 
			Matrix3x2F::Rotation(270.0f) *
			Matrix3x2F::Translation(0.0f, m_logicalSize.Width);
		m_orientationTransform3D = ScreenRotation::Rotation90;
		break;

	default:
		throw ref new FailureException();
	}

	DX::ThrowIfFailed(
		m_swapChain->SetRotation(displayRotation)
		);

	// 設定交換鏈結的反向縮放比例
	DXGI_MATRIX_3X2_F inverseScale = { 0 };
	inverseScale._11 = 1.0f / m_effectiveCompositionScaleX;
	inverseScale._22 = 1.0f / m_effectiveCompositionScaleY;
	ComPtr<IDXGISwapChain2> spSwapChain2;
	DX::ThrowIfFailed(
		m_swapChain.As<IDXGISwapChain2>(&spSwapChain2)
		);

	DX::ThrowIfFailed(
		spSwapChain2->SetMatrixTransform(&inverseScale)
		);

	// 建立交換鏈結背景緩衝區的轉譯目標檢視。
	ComPtr<ID3D11Texture2D1> backBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
		);

	DX::ThrowIfFailed(
		m_d3dDevice->CreateRenderTargetView1(
			backBuffer.Get(),
			nullptr,
			&m_d3dRenderTargetView
			)
		);

	// 建立深度樣板檢視，以視需要用於 3D 轉譯。
	CD3D11_TEXTURE2D_DESC1 depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT, 
		lround(m_d3dRenderTargetSize.Width),
		lround(m_d3dRenderTargetSize.Height),
		1, // 這個深度樣板檢視只有一種材質。
		1, // 使用單一 Mipmap 層次。
		D3D11_BIND_DEPTH_STENCIL
		);

	ComPtr<ID3D11Texture2D1> depthStencil;
	DX::ThrowIfFailed(
		m_d3dDevice->CreateTexture2D1(
			&depthStencilDesc,
			nullptr,
			&depthStencil
			)
		);

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
		m_d3dDevice->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_d3dDepthStencilView
			)
		);

	// 將 3D 轉譯檢視區的目標設為整個視窗。
	m_screenViewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		m_d3dRenderTargetSize.Width,
		m_d3dRenderTargetSize.Height
		);

	m_d3dContext->RSSetViewports(1, &m_screenViewport);

	// 建立與轉譯目標關聯的 Direct2D
	// 目標點陣圖，並將它設為目前目標。
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = 
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			m_dpi,
			m_dpi
			);

	ComPtr<IDXGISurface2> dxgiBackBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
		);

	DX::ThrowIfFailed(
		m_d2dContext->CreateBitmapFromDxgiSurface(
			dxgiBackBuffer.Get(),
			&bitmapProperties,
			&m_d2dTargetBitmap
			)
		);

	m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());
	m_d2dContext->SetDpi(m_effectiveDpi, m_effectiveDpi);

	// 建議所有 Windows 市集應用程式都使用灰階文字消除鋸齒。
	m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

// 判斷呈現目標的維度，以及是否會縮小尺寸。
void DX::DeviceResources::UpdateRenderTargetSize()
{
	m_effectiveDpi = m_dpi;
	m_effectiveCompositionScaleX = m_compositionScaleX;
	m_effectiveCompositionScaleY = m_compositionScaleY;

	// 若要提升高解析度裝置的電池壽命，請呈現到較小的呈現目標
	// 並允許 GPU 在輸出存在時調整其大小。
	if (!DisplayMetrics::SupportHighResolutions && m_dpi > DisplayMetrics::DpiThreshold)
	{
		float width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_dpi);
		float height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_dpi);

		// 當裝置為直式方向，且長度 > 寬度時，將
		// 較大的維度與寬度臨界值相比，並將較小的維度
		// 與長度臨界值相比。
		if (max(width, height) > DisplayMetrics::WidthThreshold && min(width, height) > DisplayMetrics::HeightThreshold)
		{
			// 為了調整應用程式的大小，我們變更了有效的 DPI。邏輯大小不會變更。
			m_effectiveDpi /= 2.0f;
			m_effectiveCompositionScaleX /= 2.0f;
			m_effectiveCompositionScaleY /= 2.0f;
		}
	}

	// 計算必要的呈現目標大小 (像素)。
	m_outputSize.Width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_effectiveDpi);
	m_outputSize.Height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_effectiveDpi);

	// 避免建立大小為零的 DirectX 內容。
	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);
}

// 這個方法是在建立 (或重新建立) XAML 控制項時呼叫。
void DX::DeviceResources::SetSwapChainPanel(SwapChainPanel^ panel)
{
	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_swapChainPanel = panel;
	m_logicalSize = Windows::Foundation::Size(static_cast<float>(panel->ActualWidth), static_cast<float>(panel->ActualHeight));
	m_nativeOrientation = currentDisplayInformation->NativeOrientation;
	m_currentOrientation = currentDisplayInformation->CurrentOrientation;
	m_compositionScaleX = panel->CompositionScaleX;
	m_compositionScaleY = panel->CompositionScaleY;
	m_dpi = currentDisplayInformation->LogicalDpi;
	m_d2dContext->SetDpi(m_dpi, m_dpi);

	CreateWindowSizeDependentResources();
}

// 這個方法是在 SizeChanged 事件的事件處理常式中呼叫。
void DX::DeviceResources::SetLogicalSize(Windows::Foundation::Size logicalSize)
{
	if (m_logicalSize != logicalSize)
	{
		m_logicalSize = logicalSize;
		CreateWindowSizeDependentResources();
	}
}

// 這個方法是在 DpiChanged 事件的事件處理常式中呼叫。
void DX::DeviceResources::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		m_dpi = dpi;
		m_d2dContext->SetDpi(m_dpi, m_dpi);
		CreateWindowSizeDependentResources();
	}
}

// 這個方法是在 OrientationChanged 事件的事件處理常式中呼叫。
void DX::DeviceResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
{
	if (m_currentOrientation != currentOrientation)
	{
		m_currentOrientation = currentOrientation;
		CreateWindowSizeDependentResources();
	}
}

// 這個方法是在 CompositionScaleChanged 事件的事件處理常式中呼叫。
void DX::DeviceResources::SetCompositionScale(float compositionScaleX, float compositionScaleY)
{
	if (m_compositionScaleX != compositionScaleX ||
		m_compositionScaleY != compositionScaleY)
	{
		m_compositionScaleX = compositionScaleX;
		m_compositionScaleY = compositionScaleY;
		CreateWindowSizeDependentResources();
	}
}

// 這個方法是在 DisplayContentsInvalidated 事件的事件處理常式中呼叫。
void DX::DeviceResources::ValidateDevice()
{
	// 如有以下情況，則 D3D 裝置不再有效: 預設配接器在裝置建立後變更過，
	// 或是裝置已移除。

	// 首先，取得建立裝置時之預設配接器的資訊。

	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> deviceAdapter;
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&deviceAdapter));

	ComPtr<IDXGIFactory2> deviceFactory;
	DX::ThrowIfFailed(deviceAdapter->GetParent(IID_PPV_ARGS(&deviceFactory)));

	ComPtr<IDXGIAdapter1> previousDefaultAdapter;
	DX::ThrowIfFailed(deviceFactory->EnumAdapters1(0, &previousDefaultAdapter));

	DXGI_ADAPTER_DESC1 previousDesc;
	DX::ThrowIfFailed(previousDefaultAdapter->GetDesc1(&previousDesc));

	// 接著，取得目前預設配接器的資訊。

	ComPtr<IDXGIFactory4> currentFactory;
	DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&currentFactory)));

	ComPtr<IDXGIAdapter1> currentDefaultAdapter;
	DX::ThrowIfFailed(currentFactory->EnumAdapters1(0, &currentDefaultAdapter));

	DXGI_ADAPTER_DESC1 currentDesc;
	DX::ThrowIfFailed(currentDefaultAdapter->GetDesc1(&currentDesc));

	// 如果配接器 LUID 不符，或裝置回報它已被移除，
	// 必須建立新的 D3D 裝置。

	if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
		previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
		FAILED(m_d3dDevice->GetDeviceRemovedReason()))
	{
		// 釋放舊裝置相關資源的參考。
		dxgiDevice = nullptr;
		deviceAdapter = nullptr;
		deviceFactory = nullptr;
		previousDefaultAdapter = nullptr;

		// 建立新裝置和交換鏈結。
		HandleDeviceLost();
	}
}

// 重新建立所有裝置資源，並設回目前狀態。
void DX::DeviceResources::HandleDeviceLost()
{
	m_swapChain = nullptr;

	if (m_deviceNotify != nullptr)
	{
		m_deviceNotify->OnDeviceLost();
	}

	CreateDeviceResources();
	m_d2dContext->SetDpi(m_dpi, m_dpi);
	CreateWindowSizeDependentResources();

	if (m_deviceNotify != nullptr)
	{
		m_deviceNotify->OnDeviceRestored();
	}
}

// 註冊在裝置中斷連線和建立時要通知我們的 DeviceNotify。
void DX::DeviceResources::RegisterDeviceNotify(DX::IDeviceNotify* deviceNotify)
{
	m_deviceNotify = deviceNotify;
}

// 當應用程式暫停時呼叫這個方法。它是提示驅動程式，表示應用程式 
// 即將進入閒置狀態，而且暫存緩衝區可回收供其他應用程式使用。
void DX::DeviceResources::Trim()
{
	ComPtr<IDXGIDevice3> dxgiDevice;
	m_d3dDevice.As(&dxgiDevice);

	dxgiDevice->Trim();
}

// 將交換鏈結的內容提供給螢幕。
void DX::DeviceResources::Present() 
{
	// 第一個引數會指示 DXGI 在 VSync 之前封鎖住，使應用程式
	// 直到下一次 VSync 前都保持睡眠狀態。這樣可確保不會浪費任何循環轉譯
	// 絕不會顯示在螢幕上的畫面。
	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	HRESULT hr = m_swapChain->Present1(1, 0, &parameters);

	// 捨棄轉譯目標的內容。
	// 只有在即將完全覆寫現有內容時，這才是有效的
	//作業。如果使用 dirty 或 scroll rects，應修改這個呼叫。
	m_d3dContext->DiscardView1(m_d3dRenderTargetView.Get(), nullptr, 0);

	// 捨棄深度樣板的內容。
	m_d3dContext->DiscardView1(m_d3dDepthStencilView.Get(), nullptr, 0);

	// 如果因中斷連線或驅動程式升級而移除裝置，就 
	// 必須重新建立所有裝置資源。
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		HandleDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

// 此方法會決定顯示裝置的原生方向與目前顯示方向
// 之間的旋轉。
DXGI_MODE_ROTATION DX::DeviceResources::ComputeDisplayRotation()
{
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	// 注意: NativeOrientation 只可以是 Landscape 或 Portrait，即使
	// DisplayOrientations 列舉有其他值。
	switch (m_nativeOrientation)
	{
	case DisplayOrientations::Landscape:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;

	case DisplayOrientations::Portrait:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}
	return rotation;
}