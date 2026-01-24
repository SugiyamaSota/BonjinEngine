#include"Object3d.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

ConstantBuffer<Camera> gCamera : register(b2);

ConstantBuffer<PointLight> gPointLight : register(b3);

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
    
    float32_t3 toEye = normalize(-gCamera.worldPosition - input.worldPosition);
    
    // Blling-Phon
    float32_t3 halfVector = normalize(-gDirectionalLight.direction + toEye);
    float NDotH = dot(normalize(input.normal), halfVector);
    float specularPow = pow(saturate(NDotH), gMaterial.shininess);
    
    // ポイントライト
    float32_t3 pointLightDirection = normalize(gPointLight.position - input.worldPosition);
    float32_t3 halfVector_Point = normalize(pointLightDirection + toEye);
    float NDotH_Point = dot(normalize(input.normal), halfVector_Point);
    float specularPow_Point = pow(saturate(NDotH_Point), gMaterial.shininess);
   
   

    if (gMaterial.enableLighting != 0)
    {
        // --- 拡散反射 (共通計算) ---
        // ディレクショナルライト
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        float32_t3 diffuse_Directional = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intentity;
        float32_t3 specular_Directional = float32_t3(0.0f, 0.0f, 0.0f);
        
        // ポイントライト
        float NdotL_Point = dot(normalize(input.normal), pointLightDirection);
        float cos_Point = pow(NdotL_Point * 0.5f + 0.5f, 2.0f);
        float32_t distance = length(gPointLight.position - input.worldPosition); // ポイントライトとの距離
        float32_t factor = pow(saturate(-distance / gPointLight.radius + 1.f), gPointLight.decay); // 指数によるコントロール
        float32_t3 diffuse_Point = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * factor * cos_Point * gPointLight.intensity;
        float32_t3 specular_Point = float32_t3(0.0f, 0.0f, 0.0f);
    
        if (gMaterial.enableSpecular != 0)// 鏡面反射フラグ
        {
        
            specular_Directional = gDirectionalLight.color.rgb * gDirectionalLight.intentity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
            float32_t3 specular_Point = gPointLight.color.rgb * gPointLight.intensity * factor * specularPow_Point;
            
        }

        // 最終的な色
        output.color.rgb = diffuse_Directional + specular_Directional + diffuse_Point + specular_Point;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}