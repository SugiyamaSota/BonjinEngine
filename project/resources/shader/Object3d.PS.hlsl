#include"Object3d.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

ConstantBuffer<Camera> gCamera : register(b2);

ConstantBuffer<PointLight> gPointLight : register(b3);

ConstantBuffer<SpotLight> gSpotLight : register(b4);

Texture2D<float32_t4> gTexture : register(t0);

SamplerState gSampler : register(s0);

TextureCube<float32_t4> gEnvironmentMap : register(t1);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // テクスチャ
    float3 transformedUV = mul(float32_t4(input.texcoord.x, input.texcoord.y, 0.0f, 1.0f), gMaterial.uvTransform).xyz;
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // ディレクショナル
    float32_t3 toEye = normalize(-gCamera.worldPosition - input.worldPosition);
    float32_t3 halfVector = normalize(-gDirectionalLight.direction + toEye);
    float NDotH = dot(normalize(input.normal), halfVector);
    float specularPow = pow(saturate(NDotH), gMaterial.shininess);
    
    // ポイントライト
    float32_t3 pointLightDirection = normalize(gPointLight.position - input.worldPosition);
    float32_t3 halfVector_Point = normalize(pointLightDirection + toEye);
    float NDotH_Point = dot(normalize(input.normal), halfVector_Point);
    float specularPow_Point = pow(saturate(NDotH_Point), gMaterial.shininess);
    
    // スポットライト
    float32_t3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight.position);
    float32_t3 halfVector_Spot = normalize(spotLightDirectionOnSurface + toEye);
    float NDotH_Spot = dot(normalize(input.normal), halfVector_Spot);
    float specularPow_Spot = pow(saturate(NDotH_Spot), gMaterial.shininess);
    float32_t3 spotLightDirection = normalize(gSpotLight.position - input.worldPosition);
    float NdotL_Spot = dot(normalize(input.normal), spotLightDirection);
    float cos_NdotL_Spot = pow(NdotL_Spot * 0.5f + 0.5f, 2.0f);

    if (gMaterial.enableLighting != 0)
    {
        // --- 拡散反射 (共通計算) ---
        // ディレクショナルライト
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        float32_t3 diffuse_Directional = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        float32_t3 specular_Directional = float32_t3(0.0f, 0.0f, 0.0f);
        
        // ポイントライト
        float NdotL_Point = dot(normalize(input.normal), pointLightDirection);
        float cos_Point = pow(NdotL_Point * 0.5f + 0.5f, 2.0f);
        float32_t distance = length(gPointLight.position - input.worldPosition); // ポイントライトとの距離
        float32_t factor = pow(saturate(-distance / gPointLight.radius + 1.f), gPointLight.decay); // 指数によるコントロール
        float32_t3 diffuse_Point = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * factor * cos_Point * gPointLight.intensity;
        float32_t3 specular_Point = float32_t3(0.0f, 0.0f, 0.0f);
        
        // スポットライト
        float32_t cos_Spot = dot(spotLightDirectionOnSurface, gSpotLight.direction);
        float32_t falloffFactor = saturate((cos_Spot - gSpotLight.cosAngle) / (1.f - gSpotLight.cosAngle));
        float32_t distance_Spot = length(gSpotLight.position - input.worldPosition); // ポイントライトとの距離
        float32_t factor_Spot = pow(saturate(1.0f - (distance_Spot / gSpotLight.distance)), gSpotLight.decay); // 指数によるコントロール
        float32_t3 diffuse_Spot = gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb * factor_Spot * cos_NdotL_Spot * gSpotLight.intensity * falloffFactor;
        float32_t3 specular_Spot = float32_t3(0.0f, 0.0f, 0.0f);
    
        if (gMaterial.enableSpecular != 0)// 鏡面反射フラグ
        {
        
            specular_Directional = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
            specular_Point = gPointLight.color.rgb * gPointLight.intensity * factor * specularPow_Point;
            specular_Spot = gSpotLight.color.rgb * gSpotLight.intensity * factor_Spot * specularPow_Spot;
            
        }

        // 最終的な色
        output.color.rgb = diffuse_Directional + specular_Directional + diffuse_Point + specular_Point + diffuse_Spot + specular_Spot;
        
        // 環境マップによる反射
        float32_t3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
        float32_t4 environmentColor = gEnvironmentMap.Sample(gSampler, reflectedVector);
        
        if (gMaterial.enableEnvironmentMap != 0)
        {
        // environmentCoefficient (0.0 ~ 1.0) を掛けて影響度を調整
            output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;
        }
        
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}