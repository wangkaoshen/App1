#include "pch.h"
#include "App1Main.h"
#include "Common\DirectXHelper.h"

using namespace App1;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

// 載入應用程式時，載入並初始化應用程式資產。
App1Main::App1Main(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources), m_pointerLocationX(0.0f)
{
	// 註冊要在裝置中斷連線或重新建立時收到通知
	m_deviceResources->RegisterDeviceNotify(this);

	// TODO: 以您的應用程式內容初始設定取代此項。
	m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));

	m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

	// TODO: 如果不想使用預設的變化時步模式，請變更計時器設定。
	// 例如若要 60 FPS 固定時步邏輯，請呼叫:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}

App1Main::~App1Main()
{
	// 取消註冊裝置通知
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// 當視窗大小變更 (例如裝置方向變更) 時，更新應用程式狀態
void App1Main::CreateWindowSizeDependentResources() 
{
	// TODO: 以您應用程式內容因大小而異的初始設定取代此項。
	m_sceneRenderer->CreateWindowSizeDependentResources();
}

void App1Main::StartRenderLoop()
{
	// 如果動畫轉譯迴圈已在執行，則不啟動另一個執行緒。
	if (m_renderLoopWorker != nullptr && m_renderLoopWorker->Status == AsyncStatus::Started)
	{
		return;
	}

	// 建立將在背景執行緒上執行的工作。
	auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
	{
		// 計算更新的畫面格，並依照垂直空白區間顯示一次。
		while (action->Status == AsyncStatus::Started)
		{
			critical_section::scoped_lock lock(m_criticalSection);
			Update();
			if (Render())
			{
				m_deviceResources->Present();
			}
		}
	});

	// 在專用的高優先順序背景執行緒上執行工作。
	m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void App1Main::StopRenderLoop()
{
	m_renderLoopWorker->Cancel();
}

// 每一張畫面格都更新一次應用程式狀態。
void App1Main::Update() 
{
	ProcessInput();

	// 更新場景物件。
	m_timer.Tick([&]()
	{
		// TODO: 以您的應用程式內容更新函式取代此項。
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}

// 處理來自使用者的所有輸入，然後再更新遊戲狀態
void App1Main::ProcessInput()
{
	// TODO: 在這裡加入每個畫面格輸入處理。
	m_sceneRenderer->TrackingUpdate(m_pointerLocationX);
}

// 根據目前的應用程式狀態轉譯目前的畫面格。
// 如果已轉譯畫面格並準備好可以顯示則傳回 true。
bool App1Main::Render() 
{
	// 第一次更新之前不要嘗試轉譯任何項目。
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// 重設檢視區，以整個螢幕為目標。
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// 重設螢幕的轉譯目標。
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// 清除背景緩衝區和深度樣板檢視。
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 轉譯場景物件。
	// TODO: 以您的應用程式內容顯示函式取代此項。
	m_sceneRenderer->Render();
	m_fpsTextRenderer->Render();

	return true;
}

// 通知轉譯器，說明必須釋放裝置資源。
void App1Main::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// 通知轉譯器，說明可立即重新建立裝置資源。
void App1Main::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
