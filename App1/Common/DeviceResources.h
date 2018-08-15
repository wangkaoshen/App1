#pragma once

namespace DX
{
	// 提供介面，以通知擁有 DeviceResources 的應用程式，說明裝置即將中斷連線或已建立。
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	// 控制所有 DirectX 裝置資源。
	class DeviceResources
	{
	public:
		DeviceResources();
		void SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void SetCompositionScale(float compositionScaleX, float compositionScaleY);
		void ValidateDevice();
		void HandleDeviceLost();
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		void Trim();
		void Present();

		// 呈現目標的大小 (像素)。
		Windows::Foundation::Size	GetOutputSize() const					{ return m_outputSize; }

		// 呈現目標的大小 (DIP)。
		Windows::Foundation::Size	GetLogicalSize() const					{ return m_logicalSize; }
		float						GetDpi() const							{ return m_effectiveDpi; }

		// D3D 存取子。
		ID3D11Device3*				GetD3DDevice() const					{ return m_d3dDevice.Get(); }
		ID3D11DeviceContext3*		GetD3DDeviceContext() const				{ return m_d3dContext.Get(); }
		IDXGISwapChain3*			GetSwapChain() const					{ return m_swapChain.Get(); }
		D3D_FEATURE_LEVEL			GetDeviceFeatureLevel() const			{ return m_d3dFeatureLevel; }
		ID3D11RenderTargetView1*	GetBackBufferRenderTargetView() const	{ return m_d3dRenderTargetView.Get(); }
		ID3D11DepthStencilView*		GetDepthStencilView() const				{ return m_d3dDepthStencilView.Get(); }
		D3D11_VIEWPORT				GetScreenViewport() const				{ return m_screenViewport; }
		DirectX::XMFLOAT4X4			GetOrientationTransform3D() const		{ return m_orientationTransform3D; }

		// D2D 存取子。
		ID2D1Factory3*				GetD2DFactory() const					{ return m_d2dFactory.Get(); }
		ID2D1Device2*				GetD2DDevice() const					{ return m_d2dDevice.Get(); }
		ID2D1DeviceContext2*		GetD2DDeviceContext() const				{ return m_d2dContext.Get(); }
		ID2D1Bitmap1*				GetD2DTargetBitmap() const				{ return m_d2dTargetBitmap.Get(); }
		IDWriteFactory3*			GetDWriteFactory() const				{ return m_dwriteFactory.Get(); }
		IWICImagingFactory2*		GetWicImagingFactory() const			{ return m_wicFactory.Get(); }
		D2D1::Matrix3x2F			GetOrientationTransform2D() const		{ return m_orientationTransform2D; }

	private:
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();
		void UpdateRenderTargetSize();
		DXGI_MODE_ROTATION ComputeDisplayRotation();

		// Direct3D 物件。
		Microsoft::WRL::ComPtr<ID3D11Device3>			m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext3>	m_d3dContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain3>			m_swapChain;

		// Direct3D 轉譯物件。3D 的必要項。
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView1>	m_d3dRenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	m_d3dDepthStencilView;
		D3D11_VIEWPORT									m_screenViewport;

		// Direct2D 繪製元件。
		Microsoft::WRL::ComPtr<ID2D1Factory3>		m_d2dFactory;
		Microsoft::WRL::ComPtr<ID2D1Device2>		m_d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext2>	m_d2dContext;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1>		m_d2dTargetBitmap;

		// DirectWrite 繪製元件。
		Microsoft::WRL::ComPtr<IDWriteFactory3>		m_dwriteFactory;
		Microsoft::WRL::ComPtr<IWICImagingFactory2>	m_wicFactory;

		// XAML 面板的快取參考。
		Windows::UI::Xaml::Controls::SwapChainPanel^	m_swapChainPanel;

		// 快取的裝置屬性。
		D3D_FEATURE_LEVEL								m_d3dFeatureLevel;
		Windows::Foundation::Size						m_d3dRenderTargetSize;
		Windows::Foundation::Size						m_outputSize;
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;
		float											m_compositionScaleX;
		float											m_compositionScaleY;

		// 該變數會將應用程式是否支援高解析度螢幕列入考量。
		float											m_effectiveDpi;
		float											m_effectiveCompositionScaleX;
		float											m_effectiveCompositionScaleY;

		// 用於顯示方向的轉換。
		D2D1::Matrix3x2F	m_orientationTransform2D;
		DirectX::XMFLOAT4X4	m_orientationTransform3D;

		// 可以直接持有 IDeviceNotify，因為它擁有 DeviceResources。
		IDeviceNotify* m_deviceNotify;
	};
}