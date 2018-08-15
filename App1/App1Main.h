#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"
#include "Content\SampleFpsTextRenderer.h"

// 在螢幕上轉譯 Direct2D 和 3D 內容。
namespace App1
{
	class App1Main : public DX::IDeviceNotify
	{
	public:
		App1Main(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~App1Main();
		void CreateWindowSizeDependentResources();
		void StartTracking() { m_sceneRenderer->StartTracking(); }
		void TrackingUpdate(float positionX) { m_pointerLocationX = positionX; }
		void StopTracking() { m_sceneRenderer->StopTracking(); }
		bool IsTracking() { return m_sceneRenderer->IsTracking(); }
		void StartRenderLoop();
		void StopRenderLoop();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		void ProcessInput();
		void Update();
		bool Render();

		// 裝置資源的快取指標。
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: 以您自己的內容轉譯器取代。
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
		std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;

		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// 轉譯迴圈計時器。
		DX::StepTimer m_timer;

		// 追蹤目前的輸入指標位置。
		float m_pointerLocationX;
	};
}