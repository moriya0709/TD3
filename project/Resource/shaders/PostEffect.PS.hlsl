Texture2D<float4> gCurrentTexture : register(t0);
Texture2D<float4> gPreviousTexture : register(t1);
Texture2D<float> gDepthTexture : register(t2);

SamplerState gSampler : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct EffectData
{
    int isInversion; // B0
    int isGrayscale;
    int isRadialBlur;
    int isDistanceFog;

    int isDOF; // B1
    int isHeightFog;
    float intensity;
    float pad0;

    float2 blurCenter; // B2
    float blurWidth;
    int blurSamples;

    float3 distanceFogColor; // B3
    float distanceFogStart;

    float distanceFogEnd; // B4
    float zNear;
    float zFar;
    float pad1;

    float3 heightFogColor; // B5
    float heightFogTop;

    float heightFogBottom; // B6
    float heightFogDensity;
    float2 pad2; // float2にまとめて行列を16バイト境界へ

    float4x4 matInverseViewProjection; // B7-10

    float focusDistance; // B11 (行列の直後)
    float focusRange;
    float bokehRadius;
    float pad3;

    float4 finalPad[4]; // 16byte * 4 = 64byte
};
ConstantBuffer<EffectData> gEffectData : register(b0);

float4 main(PSInput input) : SV_TARGET
{
    float4 color = gCurrentTexture.Sample(gSampler, input.uv);

    // モノクロ
    if (gEffectData.isGrayscale)
    {
        float gray = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
        color.rgb = float3(gray, gray, gray);
    }

    // 色反転
    if (gEffectData.isInversion)
    {
        color.rgb = 1.0f - color.rgb;
    }
    
    // 放射状ブラー
    if (gEffectData.isRadialBlur)
    {
        float2 direction = input.uv - gEffectData.blurCenter;
        float4 blurColor = color;

        for (int i = 1; i < gEffectData.blurSamples; i++)
        {
            // 中心に向かってサンプリング点をずらしていく
            float2 offset = direction * gEffectData.blurWidth * float(i);
            blurColor += gCurrentTexture.Sample(gSampler, input.uv - offset);
        }
        
        // 合計をサンプル数で割って平均化
        color = blurColor / float(gEffectData.blurSamples);
    } 
    // ディスタンスフォグ
    if (gEffectData.isDistanceFog)
    {
        // 1. 深度値の取得 (通常 0.0 ～ 1.0 の非線形な値)
        float depth = gDepthTexture.Sample(gSampler, input.uv);

        // 2. 深度値の線形化 (実際のカメラからの距離に変換)
        float linearDepth = (gEffectData.zNear * gEffectData.zFar) / (gEffectData.zFar - depth * (gEffectData.zFar - gEffectData.zNear));

        // 3. フォグ係数の計算 (saturateで 0.0 ～ 1.0 に収める)
        float fogFactor = saturate((linearDepth - gEffectData.distanceFogStart) / (gEffectData.distanceFogEnd - gEffectData.distanceFogStart));

        // 4. 元の色とフォグ色を合成
        color.rgb = lerp(color.rgb, gEffectData.distanceFogColor, fogFactor);
    }
    // ハイトフォグ
    if (gEffectData.isHeightFog)
    {
        // 深度値を取得
        float depth = gDepthTexture.Sample(gSampler, input.uv);

        // 2. 画面空間(UV + Depth)からワールド座標を復元
        // NDC座標を作成 (x: -1~1, y: -1~1, z: 0~1)
        float2 ndcXY = input.uv * 2.0f - 1.0f;
        ndcXY.y *= -1.0f; // UVのYは下がプラスなので反転
        float4 ndcPos = float4(ndcXY, depth, 1.0f);

        // 逆行列を掛けてワールド座標へ
        float4 worldPosWithW = mul(gEffectData.matInverseViewProjection, ndcPos);
        float3 worldPos = worldPosWithW.xyz / worldPosWithW.w;

        // 3. 高さに基づいたフォグ係数の計算
        // 指定したTop～Bottomの間で線形補間
        float heightFactor = saturate((gEffectData.heightFogTop - worldPos.y) / (gEffectData.heightFogTop - gEffectData.heightFogBottom));
        
        // 密度(Density)を適用して濃さを調整
        heightFactor = pow(heightFactor, gEffectData.heightFogDensity);

        // 4. 合成 
        color.rgb = lerp(color.rgb, gEffectData.heightFogColor, heightFactor);
    }
    // DOF
    if (gEffectData.isDOF)
    {
        float depth = gDepthTexture.Sample(gSampler, input.uv);
        float linearDepth = (gEffectData.zNear * gEffectData.zFar) / (gEffectData.zFar - depth * (gEffectData.zFar - gEffectData.zNear));

    // 錯乱円(CoC)の計算
        float coc = saturate((abs(linearDepth - gEffectData.focusDistance) - gEffectData.focusRange) / gEffectData.bokehRadius);
        // CoCを計算した後に追加
        float edgeFade = saturate(input.uv.x * 10.0) * saturate((1.0 - input.uv.x) * 10.0) *
                 saturate(input.uv.y * 10.0) * saturate((1.0 - input.uv.y) * 10.0);

        coc *= edgeFade; // 画面端では強制的にピントを合わせる（ボケを消す）
        
        if (coc > 0.0)
        {
            float4 accumColor = 0;
            float totalWeight = 0;
        
        // サンプル数を増やすと綺麗になります（例: 32〜64）
            const int sampleCount = 32;
            const float GOLDEN_ANGLE = 2.39996323; // 黄金角（ラジアン）

            for (int i = 0; i < sampleCount; i++)
            {
                float r = sqrt(float(i) / float(sampleCount));
                float theta = i * GOLDEN_ANGLE;
    
                float2 offset = float2(cos(theta), sin(theta)) * r * coc * 0.02;

    // --- ここでガードを入れる ---
                float2 sampleUV = input.uv + offset;
    
    // 画面外をサンプリングしないように 0.0～1.0 に収める
                sampleUV = saturate(sampleUV);

                float4 sampleColor = gCurrentTexture.Sample(gSampler, sampleUV);

                float weight = dot(sampleColor.rgb, float3(0.299, 0.587, 0.114));
                weight = pow(weight, 2.0) + 0.1;

                accumColor += sampleColor * weight;
                totalWeight += weight;
            }
            color = accumColor / totalWeight;
        }
    }
    
    color.rgb *= gEffectData.intensity;
    return color;
}
