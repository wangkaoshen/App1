// �`�ƽw�İϡA�i�x�s�T�Ӱ򥻥D�n��Ʀ�x�}�A��K�c���X��C
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

// �C�ӳ��I����ơA�����I�ۦ⾹����J���e�C
struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 color : COLOR0;
};

// �q�L�����ۦ⾹���C�ӹ�������m��ơC
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

// ²�檺�ۦ⾹�A�i�w�� GPU ���泻�I�B�z�C
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	// �N���I��m�ഫ����g�Ŷ��C
	pos = mul(pos, model);
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

	// ����m�q�L�A�����ק�C
	output.color = input.color;

	return output;
}
