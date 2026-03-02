#include "Outline.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};

cbuffer OutlineParam : register(b1)
{
    float thickness;
    float32_t4 color;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t3 outlineNormal : NORMAL1;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // ワールド空間で押し出す
    float32_t4 worldPos = mul(input.position, gTransformationMatrix.World);
    float32_t3 worldNormal = normalize(mul(input.outlineNormal, (float32_t3x3) gTransformationMatrix.World));
    
    // 押し出し
    worldPos.xyz += worldNormal * thickness;
    
    // WVPをかける
    output.position = mul(worldPos, gTransformationMatrix.WVP);


    return output;
}