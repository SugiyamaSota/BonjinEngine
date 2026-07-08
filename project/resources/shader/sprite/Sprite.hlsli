struct VertexShaderOutput
{
	float32_t4 position : SV_POSITION;
	float32_t2 texcoord : TEXCOORD0;
};

struct Material
{
	float32_t4 color;
	int32_t enableLighting;
	int32_t enableSpecular;
	float32_t2 padding1;
	float32_t4x4 uvTransform;
	float32_t shininess;
	uint32_t enableEnvironmentMap;
	float32_t environmentCoefficient;
	float32_t padding2;
};
