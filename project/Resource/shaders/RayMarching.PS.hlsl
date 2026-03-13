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
    // 濃い影（本来のshadow）に、薄い影（遠くまで届く散乱光）を足し合わせる
    float3 transmission = 0.0;
    transmission += exp(-shadow); // 単一散乱（強い影）
    transmission += exp(-shadow * 0.25) * 0.7; // 2次散乱（少し奥まで届く）
    transmission += exp(-shadow * 0.05) * 0.15; // 高次散乱（さらに奥まで届く）

    // ② パウダーエフェクト（Beer-Powder）
    // 雲の表面（密度が低い部分）で光が乱反射して白く輝く現象を近似
    float powder = 1.0 - exp(-shadow * 2.0);
    transmission *= powder;

    // ③ Henyey-Greenstein 近似 (前方散乱＋後方散乱のデュアルHG)
    float3 viewDir = normalize(p - cameraPos);
    float cosTheta = dot(-viewDir, lightDir);
    
    // 太陽に近い縁が強く光る（前方散乱）と、太陽を背にしたときの柔らかい光（後方散乱）をブレンド
    float g1 = 0.8;
    float hg1 = (1.0 - g1 * g1) / pow(abs(1.0 + g1 * g1 - 2.0 * g1 * cosTheta), 1.5);
    float g2 = -0.2; // 後方散乱
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

    // 太陽方向へレイを飛ばして、光を遮る雲の量（shadowDensity）を測る
    float stepLen = 40.0;
    for (int i = 0; i < 5; i++)
    {
        pos += lightDir * stepLen;
        // 少し先の密度を測る（先ほど作ったパキッとしたCloudDensityが活きます！）
        shadowDensity += CloudDensity(pos);
    }

    // 物理的な光の透過率を計算（0.0:真っ暗 ～ 1.0:完全に光が届く）
    float transmittance = exp(-shadowDensity * 2.0);

    // トゥーン調影
    float toonShadow = smoothstep(0.25, 0.35, transmittance);

    
    // 日向の色（少しだけ黄・オレンジみを帯びた眩しい白）
    float3 litColor = float3(1.0, 0.98, 0.95);
    
    // 影の色（グレーではなく、空の青や紫を強く入れるのがアニメ風の絶対条件）
    float3 shadowColor = float3(0.35, 0.45, 0.75);

    // 影の色と日向の色を、toonShadow（0 or 1）で切り替えて返す
    return lerp(shadowColor, litColor, toonShadow);
}

float4 main(VSOutput input) : SV_TARGET
{
    // ハイトフォグと同じUV→NDC変換
    float2 ndcXY = input.uv * 2.0f - 1.0f;
    ndcXY.y *= -1.0f;
    
    float4 clip = float4(ndcXY, 1.0, 1.0);
    float4 world = mul(invViewProj, clip); // ハイトフォグと同じ順序
    world.xyz /= world.w;
    
    float3 rayDir = normalize(world.xyz - cameraPos);
    
    float tBottom = (cloudBottom - cameraPos.y) / rayDir.y;
    float tTop = (cloudTop - cameraPos.y) / rayDir.y;
    
    if (abs(rayDir.y) < 0.0001)
        return float4(0, 0, 0, 0); // ゼロ付近での除算爆発を防ぐ

    float tStart = min(tBottom, tTop);
    float tEnd = max(tBottom, tTop);

// 追加：レイが飛ぶ最大距離を制限する（雲のスケールに合わせて適宜調整してください）
    float maxDist = 50000.0;
    tEnd = min(tEnd, maxDist);

    if (tEnd < 0 || tStart >= tEnd)
        return float4(0, 0, 0, 0);
    
    tStart = max(tStart, 0);
    
    float totalDist = tEnd - tStart;
    
    float maxCloudDist = 20000.0; // 雲の中での視界を制限
    float effectiveEnd = min(tStart + maxCloudDist, tEnd);
    float effectiveDist = effectiveEnd - tStart;

     // ★ 修正④: 適応ステップ長（近くは細かく、遠くは粗く）
    float minStep = 80.0;
    float maxStep = 400.0;

    float3 pos = cameraPos + rayDir * tStart;
    float randomJitter = hash(float3(input.uv * 1000.0, time));
    pos += rayDir * (minStep * randomJitter);

    float3 color = 0;
    float transmittance = 1.0;
    float t = tStart;

    for (int i = 0; i < 96; i++)
    {
        if (t >= effectiveEnd)
            break;

        float d = CloudDensity(pos);

        // ★ 修正⑤: 空領域は大きくスキップ（コスト削減の要）
        float stepLen = (d < 0.01) ? maxStep : minStep;
        float opticalDepth = d * stepLen * 0.04;

        if (opticalDepth > 0.001)
        {
            float3 light;
            if (isRialLight)
                light = RialLightCloud(pos);
            if (isAnimeLight)
                light = AnimeLightCloud(pos);

            color += opticalDepth * light * transmittance;
            transmittance *= exp(-opticalDepth);

            if (transmittance < 0.005)
                break;
        }

        pos += rayDir * stepLen;
        t += stepLen;
    }

    float alpha = 1.0 - transmittance;
    return float4(color, alpha);
}