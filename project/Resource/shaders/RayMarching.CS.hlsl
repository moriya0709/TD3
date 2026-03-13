RWTexture3D<float4> cloudTexture : register(u0);

float hash(float3 p)
{
    p = frac(p * 0.3183099 + 0.1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(float3 x)
{
    float3 i = floor(x);
    float3 f = frac(x);
    f = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(lerp(hash(i + float3(0, 0, 0)), hash(i + float3(1, 0, 0)), f.x),
                     lerp(hash(i + float3(0, 1, 0)), hash(i + float3(1, 1, 0)), f.x), f.y),
                lerp(lerp(hash(i + float3(0, 0, 1)), hash(i + float3(1, 0, 1)), f.x),
                     lerp(hash(i + float3(0, 1, 1)), hash(i + float3(1, 1, 1)), f.x), f.y), f.z);
}

float fbm(float3 p)
{
    float f = 0.0, amp = 0.5;
    for (int i = 0; i < 4; i++)
    {
        f += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return f;
}

// =====================================================
// ★ 設計変更：「1個の塊」ではなく「タイル可能なノイズ場」を焼く
//
// 旧: テクスチャ全体が1個の雲 → タイリングすると規則的に繰り返す
// 新: テクスチャ内に周期的なノイズ場 → タイリングしても継ぎ目が見えない
// =====================================================

// タイル対応ノイズ（テクスチャ解像度で折り返す）
float noiseTiling(float3 x, float period)
{
    float3 i = floor(x);
    float3 f = frac(x);
    f = f * f * (3.0 - 2.0 * f);

    // インデックスを period でmod → タイルの境界でラップアラウンド
#define H(o) hash(fmod(i + (o), period))
    return lerp(lerp(lerp(H(float3(0,0,0)), H(float3(1,0,0)), f.x),
                     lerp(H(float3(0,1,0)), H(float3(1,1,0)), f.x), f.y),
                lerp(lerp(H(float3(0,0,1)), H(float3(1,0,1)), f.x),
                     lerp(H(float3(0,1,1)), H(float3(1,1,1)), f.x), f.y), f.z);
#undef H
}

// タイル対応FBM
float fbmTiling(float3 p, float basePeriod)
{
    float f = 0.0, amp = 0.5, period = basePeriod;
    for (int i = 0; i < 4; i++)
    {
        f += amp * noiseTiling(p, period);
        p *= 2.0;
        period *= 2.0; // オクターブに合わせてperiodも倍に
        amp *= 0.5;
    }
    return f;
}

// タイル対応Worley
float worleyTiling(float3 p, float period)
{
    float3 ip = floor(p);
    float minDist = 1e9;
    for (int z = -1; z <= 1; z++)
        for (int y = -1; y <= 1; y++)
            for (int x = -1; x <= 1; x++)
            {
                float3 neighbor = fmod(ip + float3(x, y, z) + period, period);
                float3 featurePoint = neighbor + float3(
            hash(neighbor + float3(0.1, 0.0, 0.0)),
            hash(neighbor + float3(0.0, 0.2, 0.0)),
            hash(neighbor + float3(0.0, 0.0, 0.3))
        );
        // 距離計算もタイル境界をまたぐケアが必要
                float3 diff = frac((p - featurePoint) / period + 0.5) * period - period * 0.5;
                minDist = min(minDist, length(diff));
            }
    return minDist;
}

float worleyNoise(float3 p)
{
    float3 ip = floor(p); // pが属するセルの整数座標
    float minDist = 1e9;

    // 自分のセルと隣接する26セル（3×3×3）を調べる
    for (int z = -1; z <= 1; z++)
        for (int y = -1; y <= 1; y++)
            for (int x = -1; x <= 1; x++)
            {
                float3 neighbor = ip + float3(x, y, z);

        // そのセルにランダムな「特徴点」を1個置く
                float3 featurePoint = neighbor + float3(
            hash(neighbor + float3(0.1, 0.0, 0.0)),
            hash(neighbor + float3(0.0, 0.2, 0.0)),
            hash(neighbor + float3(0.0, 0.0, 0.3))
        );

        // 自分から特徴点までの距離を測り、最小値を記録
                minDist = min(minDist, length(p - featurePoint));
            }

    // 戻り値 = 最近傍距離（0=特徴点の真上, 1=セルの端）
    return minDist;
}

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 uvw = (float3) DTid / 128.0;

    // R: base（fbm + Worley合成）← PSのfbm(uv) + worley の代替
    float b = fbm(uvw * 4.0);
    float w = 1.0 - saturate(worleyNoise(uvw * 4.0) * 1.8);
    float baseNoise = saturate(b * 0.55 + w * 0.45);

    // G: detail（高周波fbm）← PSのfbm(uv * 4.0) の代替
    float detailNoise = fbm(uvw * 12.0);

    // B: coverage（超低周波fbm）← PSのfbm(uv * 0.5) の代替
    // ドメインワープで規則性を壊す
    float3 warp = float3(
        fbm(uvw * 2.0 + float3(1.7, 9.2, 3.4)),
        fbm(uvw * 2.0 + float3(8.3, 2.8, 5.1)),
        fbm(uvw * 2.0 + float3(4.1, 7.6, 1.9))
    ) * 0.5;
    float rawCov = fbm(uvw * 2.0 + warp);
    float coverage = saturate((rawCov - 0.35) * 2.5);

    cloudTexture[DTid] = float4(baseNoise, detailNoise, coverage, 0);
}