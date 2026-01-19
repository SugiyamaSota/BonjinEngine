#include"Object3d.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

ConstantBuffer<Camera> gCamera : register(b2);

Texture2D<float32_t4> gTexture : register(t0);

SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float3 transformedUV = mul(float32_t4(input.texcoord.x, input.texcoord.y, 0.0f, 1.0f), gMaterial.uvTransform).xyz;
    
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
    float32_t3 reflectLight = reflect(-gDirectionalLight.direction, normalize(input.normal));
    
    float RdotE = dot(reflectLight, toEye);
    
    float specularPow = pow(saturate(RdotE*0.9f), gMaterial.shininess);

    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    
        // --- 拡散反射 (共通計算) ---
        // テクスチャの色を反映させるのはここ
        float32_t3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intentity;
    
        float32_t3 specular = float32_t3(0.0f, 0.0f, 0.0f);
    
        if (gMaterial.enableSpecular != 0) // フラグがONなら計算
        {
        // 鏡面反射
        // specularPow が負にならないよう saturate を忘れないこと
        // 強すぎる場合は最後に 0.5f などを掛けて調整
            specular = gDirectionalLight.color.rgb * gDirectionalLight.intentity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
            
        }

        // 最終的な色は 拡散反射 + 鏡面反射
        output.color.rgb = diffuse + specular;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}