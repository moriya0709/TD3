#include "object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float3 pad1; // バイト合わせ
    float4x4 uvTransform;
    float3 emissive;
    float pad2; // バイト合わせ
};
    
struct DirectionalLight
{
    float32_t4 color; // ライトの色 
    float32_t3 direction; // ライトの向き 
    float intensity; // 輝度 
    int isDisplay; // 表示するかどうか 
};

struct AmbientLight
{
    float32_t4 color; // ライトの色 
    float intensity; // 輝度 
    int isDisplay; // 表示するかどうか 
};

struct PointLight
{
    float32_t4 color;
    float32_t3 position;
    float intensity;
    float radius;
    int isDisplay;
};

struct SpotLight
{
    float32_t4 color;
    float32_t3 position;
    float intensity;
    float32_t3 direction;
    float range; // 距離減衰用
    float innerCone; // 内側角度
    float outerCone; // 外側角度
    int isDisplay;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
ConstantBuffer<AmbientLight> gAmbientLight : register(b3);
ConstantBuffer<PointLight> gPointLight : register(b4);
ConstantBuffer<SpotLight> gSpotLight : register(b5);

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
        
    if (gMaterial.enableLighting != 0)
    {
        float3 normal = normalize(input.normal);
        float4 directional = float4(0, 0, 0, 0);
        float4 ambient = float4(0, 0, 0, 0);
        float4 pointLight = float4(0, 0, 0, 0);
        float4 spot = float4(0, 0, 0, 0);
       
        // 平行光
        if (gDirectionalLight.isDisplay)
        {
            float NdotL = dot(normal, -gDirectionalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            directional = gDirectionalLight.color * cos * gDirectionalLight.intensity;
        }
        // 環境光 
        if (gAmbientLight.isDisplay)
        {
            ambient = gAmbientLight.color * gAmbientLight.intensity;
        }
        // ポイントライト
        if (gPointLight.isDisplay)
        {
            float3 lightVec = gPointLight.position - input.worldPosition;
            float distance = length(lightVec);
            float3 L = normalize(lightVec);

            float attenuation = saturate(1.0f - distance / gPointLight.radius);

            float NdotL = dot(normal, L);
            float diffuse = saturate(NdotL);

            pointLight = gPointLight.color * diffuse * attenuation * gPointLight.intensity;
        }
        // スポットライト
        if (gSpotLight.isDisplay)
        {
            float3 lightVec = gSpotLight.position - input.worldPosition;
            float distance = length(lightVec);
            float3 L = normalize(lightVec);
            
            // 距離減衰
            float attenuation = saturate(1.0f - distance / gSpotLight.range);
            // 角度減衰
            float theta = dot(normalize(-L), normalize(gSpotLight.direction));
            float epsilon = gSpotLight.innerCone - gSpotLight.outerCone;
            float angleFactor = saturate((theta - gSpotLight.outerCone) / epsilon);
            // 拡散反射
            float NdotL = saturate(dot(normal, L));

            spot = gSpotLight.color * NdotL * attenuation * angleFactor * gSpotLight.intensity;
        }
        
        float4 lighting = directional + ambient + pointLight + spot;
        float4 finalColor = lighting + float4(gMaterial.emissive, 1.0f);
        output.color = gMaterial.color * textureColor * finalColor;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    return output;
}