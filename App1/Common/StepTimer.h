#pragma once

#include <wrl.h>

namespace DX
{
	// 動畫和模擬計時的協助程式類別。
	class StepTimer
	{
	public:
		StepTimer() : 
			m_elapsedTicks(0),
			m_totalTicks(0),
			m_leftOverTicks(0),
			m_frameCount(0),
			m_framesPerSecond(0),
			m_framesThisSecond(0),
			m_qpcSecondCounter(0),
			m_isFixedTimeStep(false),
			m_targetElapsedTicks(TicksPerSecond / 60)
		{
			if (!QueryPerformanceFrequency(&m_qpcFrequency))
			{
				throw ref new Platform::FailureException();
			}

			if (!QueryPerformanceCounter(&m_qpcLastTime))
			{
				throw ref new Platform::FailureException();
			}

			// 初始化誤差上限為 1/10 秒。
			m_qpcMaxDelta = m_qpcFrequency.QuadPart / 10;
		}

		// 取得自上次 Update 呼叫以來經過的時間。
		uint64 GetElapsedTicks() const						{ return m_elapsedTicks; }
		double GetElapsedSeconds() const					{ return TicksToSeconds(m_elapsedTicks); }

		// 取得自啟動程式以來的總共時間。
		uint64 GetTotalTicks() const						{ return m_totalTicks; }
		double GetTotalSeconds() const						{ return TicksToSeconds(m_totalTicks); }

		// 取得自動程式後的總更新次數。
		uint32 GetFrameCount() const						{ return m_frameCount; }

		// 取得目前的畫面播放速率。
		uint32 GetFramesPerSecond() const					{ return m_framesPerSecond; }

		// 設定要使用固定還是變化時步模式。
		void SetFixedTimeStep(bool isFixedTimestep)			{ m_isFixedTimeStep = isFixedTimestep; }

		// 設定在固定時步模式下，呼叫 Update 的頻率。
		void SetTargetElapsedTicks(uint64 targetElapsed)	{ m_targetElapsedTicks = targetElapsed; }
		void SetTargetElapsedSeconds(double targetElapsed)	{ m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

		// 整數格式表示以每秒 10,000,000 個刻度表示的時間。
		static const uint64 TicksPerSecond = 10000000;

		static double TicksToSeconds(uint64 ticks)			{ return static_cast<double>(ticks) / TicksPerSecond; }
		static uint64 SecondsToTicks(double seconds)		{ return static_cast<uint64>(seconds * TicksPerSecond); }

		// 在故意中斷計時 (例如封鎖 IO 作業) 之後，
		// 請呼叫此函式，以免固定時步邏輯嘗試一組更新
		// Update 呼叫。

		void ResetElapsedTime()
		{
			if (!QueryPerformanceCounter(&m_qpcLastTime))
			{
				throw ref new Platform::FailureException();
			}

			m_leftOverTicks = 0;
			m_framesPerSecond = 0;
			m_framesThisSecond = 0;
			m_qpcSecondCounter = 0;
		}

		// 適當地呼叫指定的 Update 函式數次，更新計時器狀態。
		template<typename TUpdate>
		void Tick(const TUpdate& update)
		{
			// 查詢目前的時間。
			LARGE_INTEGER currentTime;

			if (!QueryPerformanceCounter(&currentTime))
			{
				throw ref new Platform::FailureException();
			}

			uint64 timeDelta = currentTime.QuadPart - m_qpcLastTime.QuadPart;

			m_qpcLastTime = currentTime;
			m_qpcSecondCounter += timeDelta;

			// 縮短過大的時間差 (例如 在偵錯工具中暫停之後)。
			if (timeDelta > m_qpcMaxDelta)
			{
				timeDelta = m_qpcMaxDelta;
			}

			// 將 QPC 單位轉換成標準刻度格式。由於之前有縮短時間差，因此這裡不能溢位。
			timeDelta *= TicksPerSecond;
			timeDelta /= m_qpcFrequency.QuadPart;

			uint32 lastFrameCount = m_frameCount;

			if (m_isFixedTimeStep)
			{
				// 固定時步更新邏輯

				// 如果應用程式非常接近目標經過時間 (在 1/4 毫秒內)，那麼只要將
				// 時脈限制為完全符合目標值即可。如此可避免長時間累積造成的
				//細微和不相關的錯誤。如果不透過這種夾鉗方式，固定更新頻率為 60 FPS 的遊戲
				//在已啟用 vsync 的 59.94 NTSC 顯示器上執行時，最後終究會
				// 累積足夠的細微錯誤而導致遺落畫面格。最好是將這些
				// 小誤差降至零，讓一切功能正常執行。

				if (abs(static_cast<int64>(timeDelta - m_targetElapsedTicks)) < TicksPerSecond / 4000)
				{
					timeDelta = m_targetElapsedTicks;
				}

				m_leftOverTicks += timeDelta;

				while (m_leftOverTicks >= m_targetElapsedTicks)
				{
					m_elapsedTicks = m_targetElapsedTicks;
					m_totalTicks += m_targetElapsedTicks;
					m_leftOverTicks -= m_targetElapsedTicks;
					m_frameCount++;

					update();
				}
			}
			else
			{
				// 變化時步更新邏輯。
				m_elapsedTicks = timeDelta;
				m_totalTicks += timeDelta;
				m_leftOverTicks = 0;
				m_frameCount++;

				update();
			}

			// 追蹤目前的畫面播放速率。
			if (m_frameCount != lastFrameCount)
			{
				m_framesThisSecond++;
			}

			if (m_qpcSecondCounter >= static_cast<uint64>(m_qpcFrequency.QuadPart))
			{
				m_framesPerSecond = m_framesThisSecond;
				m_framesThisSecond = 0;
				m_qpcSecondCounter %= m_qpcFrequency.QuadPart;
			}
		}

	private:
		// 來源計時資料是使用 QPC 單位。
		LARGE_INTEGER m_qpcFrequency;
		LARGE_INTEGER m_qpcLastTime;
		uint64 m_qpcMaxDelta;

		// 衍生的計時資料則是使用標準刻度格式。
		uint64 m_elapsedTicks;
		uint64 m_totalTicks;
		uint64 m_leftOverTicks;

		// 用來追蹤畫面播放速率的成員。
		uint32 m_frameCount;
		uint32 m_framesPerSecond;
		uint32 m_framesThisSecond;
		uint64 m_qpcSecondCounter;

		// 用來設定固定時步模式的成員。
		bool m_isFixedTimeStep;
		uint64 m_targetElapsedTicks;
	};
}
