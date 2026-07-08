struct LineVertex
{
    float32_t4 position : POSITION0;
    float32_t4 color : COLOR0;
};

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t4 color : COLOR0;
};

struct TransformationMatrix
{
    float32_t4x4 worldViewProjection;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

VertexShaderOutput main(LineVertex input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.worldViewProjection);
    output.color = input.color;
    return output;
}
