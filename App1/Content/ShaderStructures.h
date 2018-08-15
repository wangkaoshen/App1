#pragma once

namespace App1
{
	// 用來將 MVP 矩陣傳送至頂點著色器的常數緩衝區。
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// 用來將每個頂點的資料傳送至頂點著色器。
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};
}