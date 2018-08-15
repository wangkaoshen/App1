// 常數緩衝區，可儲存三個基本主要資料行矩陣，方便構成幾何。
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

// 每個頂點的資料，當做頂點著色器的輸入內容。
struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 color : COLOR0;
};

// 通過像素著色器之每個像素的色彩資料。
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

// 簡單的著色器，可針對 GPU 執行頂點處理。
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	// 將頂點位置轉換成投射空間。
	pos = mul(pos, model);
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

	// 讓色彩通過，不做修改。
	output.color = input.color;

	return output;
}
