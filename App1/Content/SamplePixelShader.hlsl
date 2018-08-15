// 通過像素著色器之每個像素的色彩資料。
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

// (以內插值取代) 色彩資料的通過函式。
float4 main(PixelShaderInput input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}
