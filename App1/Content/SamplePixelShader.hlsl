// �q�L�����ۦ⾹���C�ӹ�������m��ơC
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

// (�H�����Ȩ��N) ��m��ƪ��q�L�禡�C
float4 main(PixelShaderInput input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}
