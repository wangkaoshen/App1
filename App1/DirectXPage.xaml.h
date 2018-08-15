//
// DirectXPage.xaml.h
// DirectXPage 類別的宣告。
//

#pragma once

#include "DirectXPage.g.h"

#include "Common\DeviceResources.h"
#include "App1Main.h"

namespace App1
{
	/// <summary>
	/// 裝載 DirectX SwapChainPanel 的頁面。
	/// </summary>
	public ref class DirectXPage sealed
	{
	public:
		DirectXPage();
		virtual ~DirectXPage();

		void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
		void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

	private:
		// XAML 低階轉譯事件處理常式。
		void OnRendering(Platform::Object^ sender, Platform::Object^ args);

		// 視窗事件處理常式。
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);

		// DisplayInformation 事件處理常式。
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

		// 其他事件處理常式。
		void AppBarButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Object^ args);
		void OnSwapChainPanelSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);

		// 在背景工作執行緒上追蹤我們獨立的輸入。
		Windows::Foundation::IAsyncAction^ m_inputLoopWorker;
		Windows::UI::Core::CoreIndependentInputSource^ m_coreInput;

		// 獨立的輸入處理函式。
		void OnPointerPressed(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);
		void OnPointerMoved(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);
		void OnPointerReleased(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);

		// 用來轉譯 XAML 頁面背景中 DirectX 內容的資源。
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::unique_ptr<App1Main> m_main; 
		bool m_windowVisible;
	};
}

