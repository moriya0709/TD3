#include "rayMarching.hlsli"

// 新しく追加する定義
Texture3D<float4> CloudNoiseTex : register(t0); // 3Dノイズテクスチャ
SamplerState LinearRepeatSampler : register(s0); // リピート（繰り返し）設定のサンプラー

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer CloudParam : register(b0)
{
    float4x4 invViewProj;

    float3 cameraPos;
    float time;

    float3 sunDir;
    float density;

    float cloudBottom;
    float cloudTop;
    
    int isRialLight;
    int isAnimeLight;
    int isMoveX;
    int isMoveY;
    
    int isMoveZ;
    int pad[3];
}

float hash(float3 p)
{
    return frac(sin(dot(p, float3(127.1, 311.7, 74.7))) * 43758.5453);
}

float noise(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    
    f = f * f * (3.0 - 2.0 * f);

    float n = lerp(
        lerp(
            lerp(hash(i + float3(0, 0, 0)), hash(i + float3(1, 0, 0)), f.x),
            lerp(hash(i + float3(0, 1, 0)), hash(i + float3(1, 1, 0)), f.x),
            f.y),
        lerp(
            lerp(hash(i + float3(0, 0, 1)), hash(i + float3(1, 0, 1)), f.x),
            lerp(hash(i + float3(0, 1, 1)), hash(i + float3(1, 1, 1)), f.x),
            f.y),
        f.z);

    return n;
}

float fbm(float3 p)
{
    float f = 0;
    float amp = 0.5;

    for (int i = 0; i < 5; i++)
    {
        f += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }

    return f;
}

// ヘルパー関数をファイル上部に追加
float Remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    return newMin + saturate((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin);
}

float CloudDensity(float3 p)
{
    float height = (p.y - cloudBottom) / (cloudTop - cloudBottom);
    if (height < 0.0 || height > 1.0)
        return 0.0;

    // ★ ベース・ディテール用UVW
    float3 uvwBase = p * 0.0008;
    if (isMoveX)
        uvwBase.x += time * 0.008;
    if (isMoveY)
        uvwBase.y += time * 0.008;
    if (isMoveZ)
        uvwBase.z += time * 0.008;

    // ★ Coverage用UVW（より広域・ゆっくり動く）
    float3 uvwCov = p * 0.00018;
    if (isMoveX)
        uvwCov.x += time * 0.003;
    if (isMoveY)
        uvwCov.y += time * 0.003;
    if (isMoveZ)
        uvwCov.z += time * 0.003;

    // ★ この2サンプルだけで fbm×3 + worley 全部を代替
    float4 n = CloudNoiseTex.SampleLevel(LinearRepeatSampler, uvwBase, 0);
    float cov = CloudNoiseTex.SampleLevel(LinearRepeatSampler, uvwCov, 0).b;

    float base = n.r;
    float detail = n.g;

    // 元のロジックをほぼそのまま維持
    float localDensity = base - (height * 0.8) - 0.1 + (density * 0.5);
    localDensity -= detail * 0.3; // detailは足すより引く方がギザギザになる

    float coverageMask = smoothstep(0.4, 0.5, cov + density * 0.3);
    localDensity *= coverageMask;
    localDensity *= smoothstep(0.0, 0.15, height);
    localDensity *= smoothstep(1.0, 0.7, height);

   // 高さによる基本形状（下は平ら、上は丸く減衰）
    float heightGradient = smoothstep(0.0, 0.15, height) * smoothstep(1.0, 0.7, height);
    float baseCloud = base * heightGradient * coverageMask;

    // ★ Remapを使って、雲の「外側（密度が低い部分）」だけをdetailノイズで浸食する
    // detailノイズの影響力を高さによって変える（上部ほど激しく浸食してモコモコに）
    float detailModifier = lerp(detail * 0.2, detail * 0.8, height);
    float finalDensity = Remap(baseCloud, detailModifier, 1.0, 0.0, 1.0);

    // 密度スケールの適用
    finalDensity *= density;

    return saturate(finalDensity);
}

float3 RialLightCloud(float3 p)
{
    float3 lightDir = normalize(sunDir);
    float shadow = 0.0;
    float3 pos = p;

    float lightStepLen = 100.0;
    for (int i = 0; i < 6; i++)
    {
        pos += lightDir * lightStepLen;
        shadow += CloudDensity(pos) * lightStepLen * 0.04;
    }

    // ① 多重散乱の近似 (光の減衰を複数のオクターブで計算)
    float3 transmission = 0.0;
    transmission += exp(-shadow);
    transmission += exp(-shadow * 0.25) * 0.7;
    transmission += exp(-shadow * 0.05) * 0.15;

    // ② パウダーエフェクト（Beer-Powder）
    float powder = 1.0 - exp(-shadow * 2.0);
    transmission *= powder;

    // ③ Henyey-Greenstein 近似
    float3 viewDir = normalize(p - cameraPos);
    float cosTheta = dot(-viewDir, lightDir);
    
    float g1 = 0.8;
    float hg1 = (1.0 - g1 * g1) / pow(abs(1.0 + g1 * g1 - 2.0 * g1 * cosTheta), 1.5);
    float g2 = -0.2;
    float hg2 = (1.0 - g2 * g2) / pow(abs(1.0 + g2 * g2 - 2.0 * g2 * cosTheta), 1.5);
    float scatter = lerp(hg1, hg2, 0.5) * 0.25 * 3.14;

    float3 sunColor = float3(1.0, 0.95, 0.85);
    float3 ambientColor = float3(0.25, 0.35, 0.5);

    return sunColor * transmission * scatter + ambientColor * (1.0 - transmission * 0.5);
}

float3 AnimeLightCloud(float3 p)
{
    float3 lightDir = normalize(sunDir);
    float shadowDensity = 0;
    float3 pos = p;

    float stepLen = 40.0;
    for (int i = 0; i < 5; i++)
    {
        pos += lightDir * stepLen;
        shadowDensity += CloudDensity(pos);
    }

    float transmittance = exp(-shadowDensity * 2.0);
    float toonShadow = smoothstep(0.25, 0.35, transmittance);
    
    float3 litColor = float3(1.0, 0.98, 0.95);
    float3 shadowColor = float3(0.35, 0.45, 0.75);

    return lerp(shadowColor, litColor, toonShadow);
}

// =====================================================================
// ヘルパー関数: レイと地球（球体）の交差判定
// =====================================================================
float2 IntersectSphere(float3 ro, float3 rd, float radius)
{
    // 球の中心は原点 (0,0,0) と仮定
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float h = b * b - c;
    
    // 交差しない場合
    if (h < 0.0)
        return float2(-1.0, -1.0);
    
    h = sqrt(h);
    return float2(-b - h, -b + h); // 交差する2点までの距離
}

// =====================================================================
// 大気散乱関数 (Nishita Single Scattering 近似)
// =====================================================================
float3 CalculateAtmosphere(float3 cameraPos, float3 rayDir, float3 sunDir)
{
    // 地球と大気のスケール設定（1単位 = 1メートル想定）
    float planetRadius = 6371000.0; // 地球の半径（約6371km）
    float atmosphereRadius = 6471000.0; // 大気圏の果て（上空100km）

    // カメラ位置を「地球の表面付近」として計算空間に合わせる
    float3 ro = float3(0.0, planetRadius + cameraPos.y, 0.0);

    // 大気圏との交差判定（どこまで計算するか）
    float2 atmosphereHit = IntersectSphere(ro, rayDir, atmosphereRadius);
    if (atmosphereHit.y < 0.0)
        return float3(0, 0, 0); // 大気圏外を向いている

    float tMin = max(0.0, atmosphereHit.x);
    float tMax = atmosphereHit.y;
    
    // 視線が地面（地球）にぶつかる場合は、そこを終点にする
    float2 planetHit = IntersectSphere(ro, rayDir, planetRadius);
    if (planetHit.x > 0.0)
    {
        tMax = min(tMax, planetHit.x);
    }

    // 散乱係数（RGBの波長ごとの散乱の強さ）
    float3 rayleighScatteringBase = float3(5.8, 13.5, 33.1) * 1e-6; // 青い光ほど強く散乱する
    float mieScatteringBase = 21.0 * 1e-6; // 白っぽい霞み

    // スケールハイト（大気の密度が 1/e になる高度）
    float rayleighScaleHeight = 8000.0; // レイリー散乱は高度8km
    float mieScaleHeight = 1200.0; // ミー散乱は高度1.2km

    // 計算の分割数（背景用なので16回で十分綺麗です）
    int numSteps = 16;
    float stepSize = (tMax - tMin) / float(numSteps);
    float currentPos = tMin + stepSize * 0.5;
    
    float2 opticalDepth = float2(0.0, 0.0);
    float3 totalRayleigh = float3(0, 0, 0);
    float3 totalMie = float3(0, 0, 0);

    // 位相関数（光が視線に向かってどう散乱するか）
    float cosTheta = dot(rayDir, sunDir);
    // レイリー位相関数（空全体に広がる光）
    float phaseRayleigh = 3.0 / (16.0 * 3.14159) * (1.0 + cosTheta * cosTheta);
    // ミー位相関数（太陽の周りに強く出る光）
    float g = 0.76;
    float phaseMie = 3.0 / (8.0 * 3.14159) * ((1.0 - g * g) * (1.0 + cosTheta * cosTheta)) /
                     ((2.0 + g * g) * pow(abs(1.0 + g * g - 2.0 * g * cosTheta), 1.5));

    // 大気の中を進みながら光を足し合わせるループ
    for (int i = 0; i < numSteps; i++)
    {
        float3 samplePos = ro + rayDir * currentPos;
        float height = length(samplePos) - planetRadius;

        // 現在の高度での空気の密度
        float densityRayleigh = exp(-height / rayleighScaleHeight) * stepSize;
        float densityMie = exp(-height / mieScaleHeight) * stepSize;
        opticalDepth += float2(densityRayleigh, densityMie);

        // 太陽からの光の減衰（簡易近似）
        // ※太陽が低いほど分厚い大気を通るため、青が散乱しきって赤（夕焼け）が残る
        float sunZenithCosAngle = dot(normalize(samplePos), sunDir);
        float sunOpticalDepthR = exp(-height / rayleighScaleHeight) * (1.0 / (max(sunZenithCosAngle, 0.0) + 0.1));
        float sunOpticalDepthM = exp(-height / mieScaleHeight) * (1.0 / (max(sunZenithCosAngle, 0.0) + 0.1));
        float2 opticalDepthLight = float2(sunOpticalDepthR, sunOpticalDepthM) * 8000.0;

        // 光の透過率
        float3 transmittance = exp(-(rayleighScatteringBase * (opticalDepth.x + opticalDepthLight.x) +
                                     mieScatteringBase * (opticalDepth.y + opticalDepthLight.y)));

        // 散乱した光を蓄積
        totalRayleigh += densityRayleigh * transmittance;
        totalMie += densityMie * transmittance;

        currentPos += stepSize;
    }

    // 太陽の光の強さ（数値を上げると全体が眩しくなります）
    float3 sunColorIntensity = float3(20.0, 20.0, 20.0);

    // 最終的な空の色を合成
    float3 skyColor = (phaseRayleigh * rayleighScatteringBase * totalRayleigh +
                       phaseMie * mieScatteringBase * totalMie) * sunColorIntensity;

    return skyColor;
}

// ---------------------------------------------------------
// メイン処理（空と雲の統合版）
// ---------------------------------------------------------
float4 main(VSOutput input) : SV_TARGET
{
    // UV→NDC変換
    float2 ndcXY = input.uv * 2.0f - 1.0f;
    ndcXY.y *= -1.0f;
    
    float4 clip = float4(ndcXY, 1.0, 1.0);
    float4 world = mul(invViewProj, clip);
    world.xyz /= world.w;
    
    float3 rayDir = normalize(world.xyz - cameraPos);

    // ==========================================
    // 1. 大気散乱（背景の空）の計算
    // ==========================================
    float3 normalizedSunDir = normalize(-sunDir);
    float3 skyRayDir = rayDir;

    // 水平線の位置調整（必要に応じて 0.05 等の数値を調整）
    float horizonOffset = 0.05;
    skyRayDir.y += horizonOffset;
    skyRayDir = normalize(skyRayDir);

    // 視線が下を向いた時に真っ黒になるのを防ぐトリック
    skyRayDir.y = max(skyRayDir.y, -0.01);
    skyRayDir = normalize(skyRayDir);

    // 空の色の計算（CalculateAtmosphere は includeファイル等に定義されている想定）
    float3 skyColor = CalculateAtmosphere(cameraPos, skyRayDir, normalizedSunDir);

    // ==========================================
    // 2. 雲の交差判定と初期化
    // ==========================================
    float3 color = float3(0, 0, 0);
    float transmittance = 1.0;
    bool hitClouds = true;

    // 真横や範囲外の場合は雲の描画のみをスキップする
    if (abs(rayDir.y) < 0.0001)
    {
        hitClouds = false;
    }

    float tBottom = (cloudBottom - cameraPos.y) / rayDir.y;
    float tTop = (cloudTop - cameraPos.y) / rayDir.y;
    
    float tStart = min(tBottom, tTop);
    float tEnd = max(tBottom, tTop);

    float maxDist = 50000.0;
    tEnd = min(tEnd, maxDist);

    if (tEnd < 0 || tStart >= tEnd)
    {
        hitClouds = false;
    }
    
    // ==========================================
    // 3. 雲のレイマーチング (雲と交差する時のみ実行)
    // ==========================================
    if (hitClouds)
    {
        tStart = max(tStart, 0);
        float maxCloudDist = 20000.0;
        float effectiveEnd = min(tStart + maxCloudDist, tEnd);

        float minStep = 80.0;
        float maxStep = 400.0;

        float3 pos = cameraPos + rayDir * tStart;
        float randomJitter = hash(float3(input.uv * 1000.0, time));
        pos += rayDir * (minStep * randomJitter);

        float t = tStart;

        for (int i = 0; i < 96; i++)
        {
            if (t >= effectiveEnd)
                break;

            float d = CloudDensity(pos);
            float stepLen = (d < 0.01) ? maxStep : minStep;
            float opticalDepth = d * stepLen * 0.04;

            if (opticalDepth > 0.001)
            {
                float3 light = float3(0, 0, 0);
                if (isRialLight)
                    light = RialLightCloud(pos);
                if (isAnimeLight)
                    light = AnimeLightCloud(pos);

                // ★ 追加: 空の環境光（skyColor）を雲の光に馴染ませる
                float3 cloudAmbient = skyColor * 0.15;
                light = (light * skyColor * 0.8) + cloudAmbient;

                color += opticalDepth * light * transmittance;
                transmittance *= exp(-opticalDepth);

                if (transmittance < 0.005)
                    break;
            }

            pos += rayDir * stepLen;
            t += stepLen;
        }
    }

    // ==========================================
    // 4. 最終合成 (空 + 雲)
    // ==========================================
    // 雲がない場所（hitClouds = false）では、color=0, transmittance=1 なので skyColor がそのまま出力される
    float3 finalColor = color + skyColor * transmittance;

    // トーンマッピング（明るさの調整）
    float exposure = 1.5;
    finalColor = 1.0 - exp(-finalColor * exposure);

    // 背景として描画するため、アルファは 1.0
    return float4(finalColor, 1.0);
}