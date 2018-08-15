#pragma once

#include <ppltasks.h>	// 適用於 create_task

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// 在這一行設定中斷點，以擷取 Win32 應用程式開發介面錯誤。
			throw Platform::Exception::CreateException(hr);
		}
	}

	// 以非同步方式讀取二進位檔案的函式。
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([] (StorageFile^ file) 
		{
			return FileIO::ReadBufferAsync(file);
		}).then([] (Streams::IBuffer^ fileBuffer) -> std::vector<byte> 
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}

	// 將以裝置獨立畫素 (DIP) 為單位的長度轉換成實體像素長度。
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // 四捨五入到最近的整數。
	}

#if defined(_DEBUG)
	// 檢查是否支援 SDK 層。
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // 不必建立實際硬體裝置。
			0,
			D3D11_CREATE_DEVICE_DEBUG,  // 檢查 SDK 層。
			nullptr,                    // 任何功能層級都有效。
			0,
			D3D11_SDK_VERSION,          // 若是 Windows 市集應用程式，這一項一定設為 D3D11_SDK_VERSION。
			nullptr,                    // 不必保留 D3D 裝置參考。
			nullptr,                    // 不必了解功能層級。
			nullptr                     // 不必保留 D3D 裝置內容參考。
			);

		return SUCCEEDED(hr);
	}
#endif
}
