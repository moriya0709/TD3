#include "PostEffect.hlsli"

Texture2D<float4> gCurrentTexture : register(t0);
Texture2D<float4> gBloom1Texture : register(t1); // 1/2ぼかし
Texture2D<float4> gBloom2Texture : register(t2); // 1/4ぼかし
Texture2D<float4> gBloom3Texture : register(t3); // 1/8ぼかし
Texture2D<float> gDepthTexture : register(t4); // 深度はt4に移動

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
    float2 pad2;

    float4x4 matInverseViewProjection; // B7-10

    float focusDistance; // B11
    float focusRange;
    float bokehRadius;
    float pad3;

   // --- ここを変更：ブルーム用パラメータを追加 ---
    float bloomThreshold; // 光らせる明るさの閾値
    float bloomIntensity; // ブルームの強さ
    float bloomBlurRadius; // ★新規追加：ブルームのぼかし半径（広がり具合）
    float pad4; // パディングを1つに減らす

    float4 finalPad[3]; // 残りのパディング (16byte * 3 = 48byte に減らす)
};
ConstantBuffer<EffectData> gEffectData : register(b0);

cbuffer RootConstants : register(b1)
{
    int gPassId; // C++から直接パス番号がねじ込まれる！
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 color = gCurrentTexture.Sample(gSampler, input.uv);

    // ★ 削除：もう input.passId は使いません！(gPassId を直接使います)

    // ==========================================
    // パス1：高輝度抽出
    // ==========================================
    if (gPassId == 1) // ★ gPassId に変更
    {
        float brightness = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
        
        // ★ C++から送られてくる bloomThreshold を使う！
        if (brightness > gEffectData.bloomThreshold)
        {
            // ★ 赤ではなく、元の綺麗な色をそのまま抽出する！
            return color;
        }
        else
        {
            // 閾値以下の暗い部分は黒にする（光らせない）
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    // ==========================================
    // パス2＆3：ガウスぼかし（X方向 / Y方向）
    // ==========================================
    if (gPassId == 2 || gPassId == 3) // ★ gPassId に変更
    {
        uint width, height;
        gCurrentTexture.GetDimensions(width, height);
        float2 texelSize = 1.0f / float2(width, height);
        
        // ★ 修正：ここも gPassId を使うように変更
        float2 direction = (gPassId == 2) ? float2(1.0f, 0.0f) : float2(0.0f, 1.0f);

        // バイリニアサンプリングを利用した効率的なウェイト
        float offset[3] = { 0.0, 1.384615, 3.230769 };
        float weight[3] = { 0.227027, 0.316216, 0.070270 };

        float3 result = gCurrentTexture.Sample(gSampler, input.uv).rgb * weight[0];
        for (int i = 1; i < 3; i++)
        {
            // ★ gEffectData.bloomBlurRadius をオフセットに掛け合わせる！
            float2 uvOffset = direction * texelSize * offset[i] * gEffectData.bloomBlurRadius;
            // ★ ここを修正：サンプリング位置を 0.0f〜1.0f の範囲にクランプする
            float2 uvSample1 = saturate(input.uv + uvOffset);
            float2 uvSample2 = saturate(input.uv - uvOffset);
            
            result += gCurrentTexture.Sample(gSampler, uvSample1).rgb * weight[i];
            result += gCurrentTexture.Sample(gSampler, uvSample2).rgb * weight[i];
        }
        return float4(result, 1.0);
    }

    // ==========================================
    // パス0：通常描画 ＆ 最終合成（コンポジット）
    // ==========================================
    if (gPassId == 0) // ★ gPassId に変更
    {
        // --- 既存のエフェクト処理 ---
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
                float2 offset = direction * gEffectData.blurWidth * float(i);
                blurColor += gCurrentTexture.Sample(gSampler, input.uv - offset);
            }
            color = blurColor / float(gEffectData.blurSamples);
        }

        // ディスタンスフォグ
        if (gEffectData.isDistanceFog)
        {
            float depth = gDepthTexture.Sample(gSampler, input.uv);
            float linearDepth = (gEffectData.zNear * gEffectData.zFar) / (gEffectData.zFar - depth * (gEffectData.zFar - gEffectData.zNear));
            float fogFactor = saturate((linearDepth - gEffectData.distanceFogStart) / (gEffectData.distanceFogEnd - gEffectData.distanceFogStart));
            color.rgb = lerp(color.rgb, gEffectData.distanceFogColor, fogFactor);
        }

        // ハイトフォグ
        if (gEffectData.isHeightFog)
        {
            float depth = gDepthTexture.Sample(gSampler, input.uv);
            float2 ndcXY = input.uv * 2.0f - 1.0f;
            ndcXY.y *= -1.0f;
            float4 ndcPos = float4(ndcXY, depth, 1.0f);
            float4 worldPosWithW = mul(gEffectData.matInverseViewProjection, ndcPos);
            float3 worldPos = worldPosWithW.xyz / worldPosWithW.w;

            float heightFactor = saturate((gEffectData.heightFogTop - worldPos.y) / (gEffectData.heightFogTop - gEffectData.heightFogBottom));
            heightFactor = pow(heightFactor, gEffectData.heightFogDensity);
            color.rgb = lerp(color.rgb, gEffectData.heightFogColor, heightFactor);
        }

        // DOF
        if (gEffectData.isDOF)
        {
            float depth = gDepthTexture.Sample(gSampler, input.uv);
            float linearDepth = (gEffectData.zNear * gEffectData.zFar) / (gEffectData.zFar - depth * (gEffectData.zFar - gEffectData.zNear));

            float coc = saturate((abs(linearDepth - gEffectData.focusDistance) - gEffectData.focusRange) / gEffectData.bokehRadius);
            float edgeFade = saturate(input.uv.x * 10.0) * saturate((1.0 - input.uv.x) * 10.0) *
                             saturate(input.uv.y * 10.0) * saturate((1.0 - input.uv.y) * 10.0);
            coc *= edgeFade;
            
            if (coc > 0.0)
            {
                float4 accumColor = 0;
                float totalWeight = 0;
                const int sampleCount = 32;
                const float GOLDEN_ANGLE = 2.39996323;

                for (int i = 0; i < sampleCount; i++)
                {
                    float r = sqrt(float(i) / float(sampleCount));
                    float theta = i * GOLDEN_ANGLE;
                    float2 offset = float2(cos(theta), sin(theta)) * r * coc * 0.02;
                    float2 sampleUV = saturate(input.uv + offset);
                    float4 sampleColor = gCurrentTexture.Sample(gSampler, sampleUV);
                    
                    float weight = dot(sampleColor.rgb, float3(0.299, 0.587, 0.114));
                    weight = pow(weight, 2.0) + 0.1;

                    accumColor += sampleColor * weight;
                    totalWeight += weight;
                }
                color = accumColor / totalWeight;
            }
        }
        
     // 1. ブルームの加算（3つのサイズをすべて重ね合わせる）
    // ★ 型を float3 に修正：.rgb は3つの成分なので float3 で受け取ります
        float3 b1 = gBloom1Texture.Sample(gSampler, input.uv).rgb * 1.0f;
        float3 b2 = gBloom2Texture.Sample(gSampler, input.uv).rgb * 0.4f;
        float3 b3 = gBloom3Texture.Sample(gSampler, input.uv).rgb * 0.2f;
    
        float3 totalBloom = b1 + b2 + b3;

    // ブルーム強度を掛けて加算
        color.rgb += totalBloom * gEffectData.bloomIntensity;

    // 2. 全体の強度調整
        color.rgb *= gEffectData.intensity;

    // 3. トーンマッピング（指数トーンマッピング）
    // $$color_{out} = 1 - e^{-color_{in} \cdot exposure}$$
        float exposure = 1.0f;
        color.rgb = 1.0f - exp(-color.rgb * exposure);
    
    // 計算済みの color を返す（アルファ値は元の画像のまま）
        return color;
    }

    return color;
}