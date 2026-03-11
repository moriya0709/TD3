#include "rayMarching.hlsli"

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
    int pad[2];

    
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

float CloudDensity(float3 p)
{
    float height = (p.y - cloudBottom) / (cloudTop - cloudBottom);

    if (height < 0 || height > 1)
        return 0;

    float3 uv = p * 0.001;
    uv.xz += time * 0.01;
    
    // 低周波ノイズを使って、雲を「塊」にする
    float coverage = fbm(uv * 0.5); // baseよりさらに引き伸ばしたノイズ
    coverage = smoothstep(0.4, 0.5, coverage + density); // 0.4以下の場所は完全に消える

    float base = fbm(uv);
    float detail = fbm(uv * 4.0);

    // baseからdetailを「引く」ことでエッジをギザギザにする
    float localDensity = base - (height * 0.8) - 0.1 + (density * 0.5);
    localDensity += detail * 0.3;

   // マスクを掛けて、隙間を確定させる
    localDensity *= coverage;

    // 高度による制限
    localDensity *= smoothstep(0, 0.15, height);
    localDensity *= smoothstep(1, 0.7, height);

    // 強めにしきい値を設定して、空間をパキッと分ける
    return saturate((localDensity - 0.1) * 2.0);
}

float3 RialLightCloud(float3 p)
{
    float3 lightDir = normalize(sunDir);

    float shadow = 0;

    float3 pos = p;

    for (int i = 0; i < 6; i++)
    {
        pos += lightDir * 50;
        shadow += CloudDensity(pos);
    }

    shadow = exp(-shadow * 0.5);

    float3 sunColor = float3(1.0, 0.95, 0.9);

   // 環境光（アンビエント）を追加
    float3 ambientColor = float3(0.3, 0.4, 0.5); // 青みがかった暗い色
    
    return sunColor * shadow + ambientColor * 0.5; // 少しだけ底上げする
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

    float stepLen = effectiveDist / 64.0;
    // 最初の位置を計算
    float3 pos = cameraPos + rayDir * tStart;
    // UV座標と時間を使ってランダムな値(0.0〜1.0)を作り、1ステップの範囲で開始位置をズラす
    float randomJitter = hash(float3(input.uv * 1000.0, time));
    pos += rayDir * (stepLen * randomJitter);
    
    float3 color = 0;
    float transmittance = 1;
    float3 light;
    
    for (int i = 0; i < 64; i++)
    {
        float density = CloudDensity(pos);
        float opticalDepth = density * stepLen * 0.05; // 0.05を基本の濃さの係数とする

        if (opticalDepth > 0.001)
        {
            if (isRialLight)
             light = RialLightCloud(pos);
            if (isAnimeLight)
             light = AnimeLightCloud(pos);
            // 透過率と同じ係数(opticalDepth)を色にも掛ける！
            color += opticalDepth * light * transmittance;
            transmittance *= exp(-opticalDepth);
    
            if (transmittance < 0.01)
                break;
        }
        pos += rayDir * stepLen;
    }
    
    float alpha = 1.0 - transmittance;
    return float4(color, alpha);
}