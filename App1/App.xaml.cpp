//
// App.xaml.cpp
// App 類別的實作。
//

#include "pch.h"
#include "DirectXPage.xaml.h"

using namespace App1;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
/// <summary>
/// 初始化單一應用程式物件。這是第一行執行之撰寫程式碼，
/// 而且其邏輯相當於 main() 或 WinMain()。
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
	Resuming += ref new EventHandler<Object^>(this, &App::OnResuming);
}

/// <summary>
/// 在應用程式由終端使用者正常啟動時叫用。當啟動應用
/// 程式來開啟特定檔案時，將使用其他進入點來顯示
///搜尋結果等資訊。
/// </summary>
/// <param name="e">關於啟動要求和處理序的詳細資料。</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{
#if _DEBUG
	if (IsDebuggerPresent())
	{
		DebugSettings->EnableFrameRateCounter = true;
	}
#endif

	if (m_directXPage == nullptr)
	{
		m_directXPage = ref new DirectXPage();
	}

	if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
	{
		m_directXPage->LoadInternalState(ApplicationData::Current->LocalSettings->Values);
	}

	auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

	// 當視窗中已有內容時，不重複應用程式初始化，
	// 只確定視窗是作用中
	if (rootFrame == nullptr)
	{
		// 建立框架做為巡覽內容，並與
		// SuspensionManager 機碼產生關聯
		rootFrame = ref new Frame();

		rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler(this, &App::OnNavigationFailed);

		if (rootFrame->Content == nullptr)
		{
			// 在巡覽堆疊未還原時，巡覽至第一頁，
			// 設定新的頁面，方式是透過傳遞必要資訊做為巡覽
			// 參數
			rootFrame->Navigate(TypeName(DirectXPage::typeid), e->Arguments);
		}
		// 將框架放在目前視窗中
		Window::Current->Content = rootFrame;
		// 確定目前視窗是作用中
		Window::Current->Activate();
	}
	else
	{
		if (rootFrame->Content == nullptr)
		{
			// 在巡覽堆疊未還原時，巡覽至第一頁，
			// 設定新的頁面，方式是透過傳遞必要資訊做為巡覽
			// 參數
			rootFrame->Navigate(TypeName(DirectXPage::typeid), e->Arguments);
		}
		// 確定目前視窗是作用中
		Window::Current->Activate();
	}
}

/// <summary>
/// 在應用程式暫停執行時叫用。應用程式狀態會儲存起來，
/// 但不知道應用程式即將結束或繼續，而且仍將記憶體
/// 的內容保持不變。
/// </summary>
/// <param name="sender">暫停之要求的來源。</param>
/// <param name="e">有關暫停之要求的詳細資料。</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// 未使用的參數
	(void) e;	// 未使用的參數

	m_directXPage->SaveInternalState(ApplicationData::Current->LocalSettings->Values);
}

/// <summary>
/// 在應用程式繼續執行時叫用。
/// </summary>
/// <param name="sender">繼續要求的來源。</param>
/// <param name="args">有關繼續要求的詳細資料。</param>
void App::OnResuming(Object ^sender, Object ^args)
{
	(void) sender; // 未使用的參數
	(void) args; // 未使用的參數

	m_directXPage->LoadInternalState(ApplicationData::Current->LocalSettings->Values);
}

/// <summary>
/// 在巡覽至某頁面失敗時叫用
/// </summary>
/// <param name="sender">導致巡覽失敗的框架</param>
/// <param name="e">有關巡覽失敗的詳細資料</param>
void App::OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e)
{
	throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}

