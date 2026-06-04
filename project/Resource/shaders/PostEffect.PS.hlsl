#include "PostEffect.hlsli"

Texture2D<float4> gCurrentTexture : register(t0);
Texture2D<float4> gBloom1Texture : register(t1); // 1/2ぼかし
Texture2D<float4> gBloom2Texture : register(t2); // 1/4ぼかし
Texture2D<float4> gBloom3Texture : register(t3); // 1/8ぼかし
Texture2D<float> gDepthTexture : register(t4); // 深度はt4に移動
Texture2D<float4> gLensFlareTexture : register(t5); // レンズフレア
Texture2D<float2> gVelocityTexture : register(t6); // RGチャンネルのみの想定

SamplerState gSampler : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct EffectData
{
    int isInversion; // 色反転
    int isGrayscale; // グレースケール
    int isRadialBlur; // 放射状ブラー
    int isDistanceFog; // ディスタンスフォグ

    int isDOF; // 被写界深度
    int isHeightFog; // ハイトフォグ
    float intensity; // 全体のエフェクト強度
    float pad0;

    float2 blurCenter; // 放射状ブラーの中心 (0.0〜1.0)
    float blurWidth; // 放射状ブラーの幅
    int blurSamples; // 放射状ブラーのサンプル数

    float3 distanceFogColor; // ディスタンスフォグの色
    float distanceFogStart; // ディスタンスフォグの開始距離

    float distanceFogEnd; // ディスタンスフォグの終了距離
    float zNear; // 深度の線形化に必要なカメラの近クリップ面
    float zFar; // 深度の線形化に必要なカメラの遠クリップ面
    float pad1;

    // *ハイトフォグ* //
    float3 heightFogColor; // 色
    float heightFogTop; // 高さ(最上値)

    float heightFogBottom; // 高さ(最下値)
    float heightFogDensity; // ハイトフォグの密度（高さによる減衰の強さ）
    float2 pad2;

    float4x4 matInverseViewProjection; // ハイトフォグのワールド位置計算に必要な逆行列

    // *DoF* //
    float focusDistance; // ピントの距離
    float focusRange; // ピントの範囲（この距離内は完全にピントが合う）
    float bokehRadius; // ボケの半径（大きいほど大きくボケる）
    float pad3;

   // *ブルーム* //
    float bloomThreshold; // 光らせる明るさの閾値
    float bloomIntensity; // ブルームの強さ
    float bloomBlurRadius; // ブルームのぼかし半径（広がり具合）
    float pad4;
    
   // *レンズフレア* //
    int isLensFlare; // レンズフレアのON/OFF
    int lensFlareGhostCount; // ゴーストの数
    float lensFlareGhostDispersal; // ゴーストの広がり具合
    float lensFlareHaloWidth; // ヘイローの大きさ
    
    int isACES; // ACESトーンマッピングのON/OFF
    float caIntensity; // 色収差の強さ (0.001f とかが綺麗)
    float2 pad5;
    
    // *モーションブラー* //
    int isMotionBlur; // モーションブラーのON/OFF
    int motionBlurSamples; // モーションブラーのサンプル数
    float motionBlurScale; // モーションブラーの強さ（速度に対するスケール）
    float pad6;
    
    // 色収差
    int isFullScreenCA; // 画面全体の色収差ON/OFF
    float fullScreenCAIntensity; // 画面全体の色収差の強さ
    // ビネット
    int isVignette; // ビネットのON/OFF
    float vignetteIntensity; // ビネットの強さ
    
    // スピードディストーション
    int isSpeedDistortion; // スピードディストーションのON/OFF
    float speedDistortionStrength; // 歪みの強さ
    float2 pad7; // アライメント調整用
    
    // 集中線
    int isConcentrationLines; // ON/OFF
    float concentrationLineIntensity; // 線の濃さ
    float2 concentrationLineCenter; // 中心座標 (通常 0.5, 0.5)

    float concentrationLineDensity; // 線の密度（本数）
    float concentrationLineLength; // 線の長さ（中心からの開始距離 0.0〜1.0）
    float concentrationLineSpeed; // アニメーション速度
    float time;
    
    // ピンチエフェクト
    int isPinch; // ON/OFF
    float pinchStrength; // 歪みの強さ（正の値で吸い込み、負の値で膨張）
    float2 pinchCenter; // 歪みの中心 (通常 0.5, 0.5)

    float pinchRadius; // 歪みが影響する半径
    float3 pad8;
    
    // モノクロ
    int isTwoColor; // 二値化
    float threshold; // 白と黒の境界値 (0.0~1.0)
    float contrast; // コントラストの強さ
    float pad9;
    
    
};
ConstantBuffer<EffectData> gEffectData : register(b0);

cbuffer RootConstants : register(b1)
{
    int gPassId; // パス番号
};

struct SunAndCloudParam
{
    float4x4 invViewProj;
    float3 cameraPos;
    float time;
    float3 sunDir;
    float cloudCoverage;
    float cloudBottom;
    float cloudTop;
    int isRialLight;
    int isAnimeLight;
    float3 cloudOffset;
    int pad;
};
ConstantBuffer<SunAndCloudParam> gSunCloudData : register(b2);

// 簡易的なガウスぼかし関数と、虹色を作るスペクトル関数
float3 SampleGaussian(Texture2D<float4> tex, SamplerState samp, float2 uv, float2 texelSize, float blurSigma)
{
    float3 result = 0;
    float totalWeight = 0;
    // 3x3の簡易ガウスカーネル
    const float kernel[3] = { 0.227027, 0.316216, 0.070270 };
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize * blurSigma;
            float weight = kernel[abs(x)] * kernel[abs(y)];
            result += tex.Sample(samp, uv + offset).rgb * weight;
            totalWeight += weight;
        }
    }
    return result / totalWeight;
}

// 虹色（スペクトル）のグラデーションを作る関数
float3 Spectrum(float t)
{
    float3 r = float3(1.0, 0.0, 0.0); // 赤
    float3 g = float3(0.0, 1.0, 0.0); // 緑
    float3 b = float3(0.0, 0.0, 1.0); // 青
    // コサイン波を組み合わせてスペクトルを作る (t: 0.0～1.0)
    float3 color = cos((t - float3(0.0, 0.5, 1.0)) * 3.14159 * 2.0) * 0.5 + 0.5;
    return color;
}

// ACESトーンマッピング関数
float3 ACESFitted(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// 非線形・距離依存の色収差サンプリング
float3 SampleWithCA(Texture2D<float4> tex, SamplerState samp,
                    float2 uv, float2 toCenter, float caIntensity)
{
    float dist = length(toCenter);
    float caScale = caIntensity * dist * dist * 8.0f; // 二乗則：周辺ほど強く
    float2 caDir = normalize(toCenter + 0.0001f);

    float r = tex.Sample(samp, saturate(uv + caDir * caScale * 1.0f)).r;
    float g = tex.Sample(samp, saturate(uv + caDir * caScale * 0.5f)).g;
    float b = tex.Sample(samp, saturate(uv - caDir * caScale * 0.5f)).b;
    return float3(r, g, b);
}

float4 main(VSOutput input) : SV_TARGET
{
    float4 color = gCurrentTexture.Sample(gSampler, input.uv);
    
    // パス1：高輝度抽出
    if (gPassId == 1)
    {
        float brightness = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
        
        if (brightness > gEffectData.bloomThreshold)
        {
            return color;
        }
        else
        {
            // 閾値以下は黒
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    // パス2＆3：ガウスぼかし（X方向 / Y方向）
    if (gPassId == 2 || gPassId == 3)
    {
        uint width, height;
        // 一番ぼけているBloom3からサイズを取得する
        gBloom3Texture.GetDimensions(width, height);
        float2 texelSize = (width > 0 && height > 0) ? (1.0f / float2(width, height)) : float2(0.001f, 0.001f);
    
        float2 direction = (gPassId == 2) ? float2(1.0f, 0.0f) : float2(0.0f, 1.0f);

        // バイリニアサンプリングを利用した効率的なウェイト
        float offset[3] = { 0.0, 1.384615, 3.230769 };
        float weight[3] = { 0.227027, 0.316216, 0.070270 };

        float3 result = gCurrentTexture.Sample(gSampler, input.uv).rgb * weight[0];
        for (int i = 1; i < 3; i++)
        {
            // gEffectData.bloomBlurRadius をオフセットに掛け合わせる
            float2 uvOffset = direction * texelSize * offset[i] * gEffectData.bloomBlurRadius;
            // サンプリング位置を 0.0f〜1.0f の範囲にクランプする
            float2 uvSample1 = saturate(input.uv + uvOffset);
            float2 uvSample2 = saturate(input.uv - uvOffset);
            
            result += gCurrentTexture.Sample(gSampler, uvSample1).rgb * weight[i];
            result += gCurrentTexture.Sample(gSampler, uvSample2).rgb * weight[i];
        }
        return float4(result, 1.0);
    }
    
    // レンズフレア
    if (gPassId == 4)
    {
        // 共通の変数定義
        float2 uv = input.uv;
        float2 toCenter = float2(0.5f, 0.5f) - uv;

        uint width, height;
        gCurrentTexture.GetDimensions(width, height);
        float2 texelSize = (width > 0 && height > 0) ? (1.0f / float2(width, height)) : float2(0.001f, 0.001f);

        // ヘイロー用の uvAspect の計算
        float aspectRatio = (float) width / (float) height;
        float2 uvAspect = float2((uv.x - 0.5f) * aspectRatio, uv.y - 0.5f);

        float3 result = float3(0.0f, 0.0f, 0.0f);

        float dynamicDispersal = gEffectData.lensFlareGhostDispersal;

        // ブレンド用の係数 (lerpFactor) を計算
        float lerpFactor = saturate((dynamicDispersal - 0.2f) / (0.8f - 0.2f));

        float baseCAIntensity = gEffectData.caIntensity;
        float caIntensity = baseCAIntensity * lerp(1.0f, 2.0f, lerpFactor);
        
        // ゴーストの生成
        int numGhosts = gEffectData.lensFlareGhostCount;
        
        if (numGhosts > 0)
        {
            for (int i = 1; i < numGhosts; ++i)
            {
                float2 offset = uv + toCenter * dynamicDispersal * (float) i;
                float dist = length(0.5f - offset);
                float2 caOffset = toCenter * caIntensity * dist;
                
                float baseSigma = 3.0f + (float) i * 1.5f;
                float blurSigma = baseSigma * lerp(1.0f, 1.2f, lerpFactor);
                
                float r = SampleGaussian(gBloom3Texture, gSampler, saturate(offset + caOffset), texelSize, blurSigma).r;
                float g = SampleGaussian(gBloom3Texture, gSampler, saturate(offset), texelSize, blurSigma).g;
                float b = SampleGaussian(gBloom3Texture, gSampler, saturate(offset - caOffset), texelSize, blurSigma).b;
                
                float weight = pow(1.0f - (float(i) / max(1.0f, float(numGhosts))), 3.0f);
                
                // 画面の端に発生する不自然なゴーストを暗くして消す
                float distFromCenter = length(toCenter);
                float vignette = 1.0f - smoothstep(0.2f, 0.6f, distFromCenter);
                
                // 元のウェイトに vignette を掛け合わせて出力を抑える
                result += float3(r, g, b) * weight * vignette * 0.3f;
            }
        }

        // *ヘイロー* //
        float haloRadius = gEffectData.lensFlareHaloWidth;
        float haloThickness = 0.04f;

        float distToCenter = length(uvAspect);

        float innerEdge = haloRadius - haloThickness;
        float outerEdge = haloRadius + haloThickness;
        float weightHalo = smoothstep(innerEdge, haloRadius, distToCenter)
                         - smoothstep(haloRadius, outerEdge, distToCenter);

        float centerFade = smoothstep(0.0f, 0.15f, distToCenter);
        float edgeFade = smoothstep(0.0f, 0.25f, 1.0f - distToCenter * 1.8f);
        weightHalo = saturate(weightHalo) * centerFade * edgeFade;

        if (weightHalo > 0.001f)
        {
            float2 haloOffsetScalar = haloRadius / max(0.001f, length(toCenter));
            float2 haloSampleUV = saturate(uv + toCenter * haloOffsetScalar);

            // gCurrentTexture から非線形色収差でサンプリング
            float3 haloColor = SampleWithCA(gCurrentTexture, gSampler,
                                            haloSampleUV, toCenter, caIntensity * 0.5f);

            // 改良スペクトル（内側:青紫 → 外側:赤橙）
            float ringT = saturate((distToCenter - innerEdge) / max(0.0001f, outerEdge - innerEdge));
            float3 haloSpectrum = Spectrum(ringT);

            result += haloColor * haloSpectrum * weightHalo * 2.5f;
        }

        return float4(result, 1.0f);
    }

    // *通常描画 ＆ 最終合成* //
    if (gPassId == 0)
    {
        
        // スピードディストーション
        if (gEffectData.isSpeedDistortion)
        {
            // 画面中心(0.5, 0.5)からのベクトルを計算
            float2 toCenter = input.uv - float2(0.5f, 0.5f);
            
            // 中心からの距離の2乗（画面端に行くほど急激に歪むようにする）
            float distSq = dot(toCenter, toCenter);
            
            // 歪み係数の計算 
            float warpFactor = 1.0f + (gEffectData.speedDistortionStrength * distSq);
            
            // UV座標を上書きして歪ませる
            input.uv = float2(0.5f, 0.5f) + toCenter * warpFactor;
            
            // 画面外のサンプリングによるアーティファクトを防ぐためにクランプ
            input.uv = saturate(input.uv);
            
            // 歪んだUVを使って、一番最初のベースカラーを再サンプリングして上書き
            color = gCurrentTexture.Sample(gSampler, input.uv);
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
        
        // モーションブラー処理
        if (gEffectData.isMotionBlur)
        {
            // 現在のピクセルの速度ベクトルを取得 (RG16Fなどを想定)
            float2 velocity = gVelocityTexture.Sample(gSampler, input.uv).rg;
        
            // 速度のスケール調整（強すぎる場合はここで抑える）
            velocity *= gEffectData.motionBlurScale;

            // 速度が極端に小さい場合は処理をスキップ（軽量化）
            if (length(velocity) > 0.0001f)
            {
                // サンプル1回あたりの移動量
                float2 texelStep = velocity / (float) gEffectData.motionBlurSamples;
            
                float4 accumColor = color;
                float2 currentUV = input.uv;

                // 速度ベクトルの方向に向かって複数回サンプリング
                for (int i = 1; i < gEffectData.motionBlurSamples; ++i)
                {
                    currentUV -= texelStep;
                
                    // 画面外のサンプリングを防ぐためのクランプ
                    currentUV = saturate(currentUV);
                
                    accumColor += gCurrentTexture.Sample(gSampler, currentUV);
                }
            
                // 平均化
                color = accumColor / (float) gEffectData.motionBlurSamples;
            }
        }
        
        // ピンチエフェクト
        if (gEffectData.isPinch)
        {
            // 中心からの方向と距離を計算
            float2 toCenter = input.uv - gEffectData.pinchCenter;
            float dist = length(toCenter);

            // 中心に近いほど強く歪ませる計算
            if (dist < gEffectData.pinchRadius)
            {
                // 歪みの減衰（半径の端にいくほど 0 になるように）
                float percent = dist / gEffectData.pinchRadius; // 0.0 ~ 1.0
            
                // 指数関数を使って「中心ほど吸い込まれる」ように補間
                // pinchStrength が大きいほど中心に収束する
                float weight = pow(percent, gEffectData.pinchStrength);
            
                // 新しいUVを再構成
                input.uv = gEffectData.pinchCenter + normalize(toCenter) * weight * gEffectData.pinchRadius;
            }
            
            color = gCurrentTexture.Sample(gSampler, input.uv);
        }
        
        // フルスクリーン色収差
        if (gEffectData.isFullScreenCA)
        {
            float2 toCenter = float2(0.5f, 0.5f) - input.uv;
            // 既存の便利な関数を再利用して、ベースカラーをズレた色で上書き
            color.rgb = SampleWithCA(gCurrentTexture, gSampler, input.uv, toCenter, gEffectData.fullScreenCAIntensity);
        }
        
         // 集中線の実装
        if (gEffectData.isConcentrationLines)
        {
            float2 toCenter = input.uv - gEffectData.concentrationLineCenter;
            float dist = length(toCenter);
            float angle = (atan2(toCenter.y, toCenter.x) + 3.14159f) / (2.0f * 3.14159f);

            // ランダムロジック
            // 角度を分割して「線のID」を作る
            float lineId = floor(angle * gEffectData.concentrationLineDensity);
        
            // 時間でシードを変化させて動かす (floorを使うとカチカチと切り替わる)
            float timeSeed = floor(gEffectData.time * gEffectData.concentrationLineSpeed);
        
            // 3種類のノイズを生成（シード値をずらして計算）
            float n_exists = frac(sin(lineId * 12.9898f + timeSeed) * 43758.5453f); // 出現判定
            float n_length = frac(sin(lineId * 39.1231f + timeSeed) * 753.5453f); // 長さのバラツキ
            float n_weight = frac(sin(lineId * 71.3521f + timeSeed) * 921.5453f); // 濃さのバラツキ

            // 線の描画判定
            float lineMask = step(0.5f, n_exists);

            // 長さのバラツキ設定
            float randomStart = gEffectData.concentrationLineLength + (n_length * 0.2f);
            float distMask = smoothstep(randomStart, randomStart + 0.1f, dist);

            // 最終的な合成
            float finalAlpha = lineMask * distMask * gEffectData.concentrationLineIntensity * n_weight;

            // 背景色を維持しつつ、黒い線を乗せる (lerpを使用)
            float3 lineColor = float3(1, 1, 1);
            color.rgb = lerp(color.rgb, lineColor, finalAlpha);
        }
        
        // ビネット
        if (gEffectData.isVignette)
        {
            // 画面中心からの距離を計算 (中心0.0 ～ 四隅約0.707)
            float dist = distance(input.uv, float2(0.5f, 0.5f));
            
            // 距離0.3〜0.8の範囲で 0.0 → 1.0 になるグラデーションを作成
            float vignetteWeight = smoothstep(0.3f, 0.8f, dist);
            
            // C++から渡された強さを掛ける
            vignetteWeight *= saturate(gEffectData.vignetteIntensity);

            // ダメージ用の色（暗めの血の色をイメージ）
            float3 damageColor = float3(0.6f, 0.0f, 0.0f);

            // 元の色とダメージ色を、vignetteWeightの強さで合成
            color.rgb = lerp(color.rgb, damageColor, vignetteWeight);

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
        
        // ブルームの加算
        float3 b1 = gBloom1Texture.Sample(gSampler, input.uv).rgb * 1.0f;
        float3 b2 = gBloom2Texture.Sample(gSampler, input.uv).rgb * 0.4f;
        float3 b3 = gBloom3Texture.Sample(gSampler, input.uv).rgb * 0.2f;

        float3 totalBloom = b1 + b2 + b3;

      
        // ブルーム強度を掛けて加算
        color.rgb += totalBloom * gEffectData.bloomIntensity;
    
        // *レンズフレアを最終カラーに加算* //
        if (gEffectData.isLensFlare)
        {
            float3 lensFlare = gLensFlareTexture.Sample(gSampler, input.uv).rgb;
            color.rgb += lensFlare;
        }
        
        // モノクロ
        if (gEffectData.isGrayscale)
        {
            float gray = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
            if (gEffectData.isTwoColor)
            {
            
                // コントラスト調整
                gray = saturate((gray - 0.5f) * gEffectData.contrast + 0.5f);

                // 二値化
                gray = smoothstep(gEffectData.threshold - 0.01f, gEffectData.threshold + 0.01f, gray);
            }
            color.rgb = float3(gray, gray, gray);
        }
        
        // 色反転
        if (gEffectData.isInversion)
        {
            color.rgb = 1.0f - color.rgb;
        }

        color.rgb *= gEffectData.intensity;

        // ACESトーンマッピング
        if (gEffectData.isACES)
        {
            color.rgb = ACESFitted(color.rgb);
        }
        else
        {
            // Reinhard (指数トーンマッピング)
            float exposure = 1.0f;
            color.rgb = 1.0f - exp(-color.rgb * exposure);
        }
    }

    return color;
}