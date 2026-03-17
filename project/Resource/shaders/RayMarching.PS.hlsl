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
    float cloudCoverage;

    float cloudBottom;
    float cloudTop;   
    int isRialLight;
    int isAnimeLight;
    
    float3 cloudOffset;
    int pad;
}

float hash(float3 p)
{
    p = frac(p * 0.3183099 + .1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
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

    // ---------------------------------------------------
    // 【軽量化1】カバレッジ計算を先にやり、完全に雲が無い場合は計算ストップ
    // ---------------------------------------------------
    float coverage = smoothstep(0.0, 0.5, cloudCoverage);
    if (coverage <= 0.0)
        return 0.0;

    float3 uv = p * 0.001;
    uv += cloudOffset;

    // まず、1つ目のノイズ（ベース形状）だけを取得
    float base = fbm(uv);

    // ディテールを足す前の「仮の密度」を計算する
    float baseDensity = base - (height * 0.8) - 0.1 + (cloudCoverage * 0.5);

    // ---------------------------------------------------
    // ★【軽量化2】アーリーエグジット（超重要）
    // detailノイズ(最大値1.0)に0.3を掛けたものが最大の影響力です。
    // つまり、「仮の密度 + 0.3」が 0 以下になる場所は、
    // この後どんなにディテールを足しても絶対に雲になりません。
    // だったら、ここで計算を打ち切って空(0.0)を返します！
    // ---------------------------------------------------
    if (baseDensity + 0.3 <= 0.0)
    {
        return 0.0;
    }

    // ここまで生き残った場所（雲の内部や輪郭）だけ、重い2回目のノイズを計算！
    float detail = fbm(uv * 4.0);

    // 仮の密度にディテールを合成
    float localDensity = baseDensity + (detail * 0.3);

    // マスクを掛けて、隙間を確定させる
    localDensity *= coverage;

    // 高度による制限
    localDensity *= smoothstep(0.0, 0.15, height);
    localDensity *= smoothstep(1.0, 0.7, height);

    // 強めにしきい値を設定して、空間をパキッと分ける
    return saturate((localDensity - 0.1) * 2.0);
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

    // 水平線の位置調整
    float horizonOffset = 0.05;
    skyRayDir.y += horizonOffset;
    skyRayDir = normalize(skyRayDir);

    // 空の色の計算（フェード前の生の色を取得）
    float3 skyColorRaw = CalculateAtmosphere(cameraPos, skyRayDir, normalizedSunDir);
    float3 skyColor = skyColorRaw;
    
    // ==========================================
    // ★【新規追加】雲の環境光用に、地平線下の影響を受けない「上空の色」を計算する
    // ==========================================
    float3 ambientRayDir = rayDir;
    ambientRayDir.y = max(ambientRayDir.y, 0.05); // 強制的に上空（Y=0.5）を向かせる
    ambientRayDir = normalize(ambientRayDir);
    float3 cloudAmbientSkyColor = CalculateAtmosphere(cameraPos, ambientRayDir, normalizedSunDir);
    
       // 水平線
    if (skyRayDir.y < 0.0)
    {
        // 時間によって海の色を変える
        
      // 1. 太陽の高さ（Y成分）を取得
        // normalizedSunDir.y は、太陽が上にあるとプラス、下（夜）に沈むとマイナスになります。
        float sunHeight = normalizedSunDir.y;

        // 2. 昼・夕方・夜の「海の色」をそれぞれ定義
        float3 daySea = float3(0.05, 0.3, 0.6); // 昼：鮮やかな青
        float3 sunsetSea = float3(0.6, 0.25, 0.1); // 夕方：夕焼けを反射したオレンジ・赤
        float3 nightSea = float3(0.01, 0.015, 0.03); // 夜：暗い紺色

        float3 seaColor;

        // 3. 太陽の高さに合わせて、色をブレンドする
        if (sunHeight > 0.0)
        {
            // 太陽が地平線より上（夕方〜昼）
            float blend = smoothstep(0.0, 0.2, sunHeight);
            seaColor = lerp(sunsetSea, daySea, blend);
        }
        else
        {
            // 太陽が地平線より下（夜〜夕方）
            float blend = smoothstep(-0.2, 0.0, sunHeight);
            seaColor = lerp(nightSea, sunsetSea, blend);
        }

        // さらに、上空の空の色(cloudAmbientSkyColor)を少し足すと、水面の反射っぽくなってより自然に馴染みます！
        seaColor += cloudAmbientSkyColor * 0.1;

        // 4. 元のフェードの計算（空の色と海の色を境界でなじませる）
        float fadeWidth = 0.05;
        float fadeFactor = smoothstep(-fadeWidth, 0.0, skyRayDir.y);
        skyColor = lerp(seaColor, skyColor, fadeFactor);
    }
    
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
               // ==========================================
                // 追加: リアルな雲のライティング計算
                // ==========================================
                // 太陽の向きと視線の角度（内積）
                float cosTheta = dot(rayDir, normalizedSunDir);

                // 1. Henyey-Greenstein 位相関数（太陽の周りが眩しく光る効果）
                // g は光の前方散乱の強さ（0.0〜1.0）。0.7くらいが雲に丁度いいです。
                float g = 0.7;
                float phase = (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5) / (4.0 * 3.14159);
                // 逆光時のフチの輝きを足す
                float phaseBlend = lerp(phase, 1.0, 0.3);

                // 2. パウダー効果（表面が明るく、奥が暗くなる）
                // opticalDepth（密度）を使って、表面付近だけ明るくする
                float powderEffect = 1.0 - exp(-opticalDepth * 2.0);

                // 3. 雲の中の太陽光の強さ（簡易的な影の計算）
                // 本来は太陽方向へもう一度レイマーチングしますが、重いので近似します
                // d は CloudDensity で取得した現在の密度
                float lightScattering = exp(-d * 50.0) * powderEffect * phaseBlend;
                
                // 夕焼けの広がりと強さの計算
                // 1. 太陽と視線の角度（1.0が太陽のド真ん中、-1.0が真後ろ）
                float cosThetaGlobal = dot(rayDir, normalizedSunDir);
                // 2. ★ここで広がりを決める！
                float sunsetSpread = smoothstep(-5.0, 1.0, cosThetaGlobal);
                // 3. 太陽が低い時（夕方）だけ効果を出すための判定（Yが 0.0 近辺の時）
                float sunsetTime = smoothstep(0.3, 0.0, normalizedSunDir.y) * smoothstep(-0.2, 0.0, normalizedSunDir.y);
                // 4. 空全体に広がる夕焼けの基本色（燃えるようなオレンジ）
                float3 wideSunsetColor = float3(1.0, 0.35, 0.1) * sunsetSpread * sunsetTime;
                
                // ==========================================
                // 色の合成
                // ==========================================
                float3 light = float3(0, 0, 0);
                
                // 元のライト関数がある場合は掛け合わせる
                if (isRialLight)
                    light = RialLightCloud(pos);
                if (isAnimeLight)
                    light = AnimeLightCloud(pos);

              
                // ==========================================
                // 夕焼けを広範囲に適用するライティング
                // ==========================================
                
                // (Henyey-Greenstein や powderEffect の計算はそのまま...)

                // 1. 太陽光の「直接光」の色を、時間帯によって白からオレンジに変化させる
                float3 daySunColor = float3(1.0, 0.9, 0.8);
                float3 sunsetSunColor = float3(1.0, 0.4, 0.05); // 濃い夕焼け色
                float3 currentSunColor = lerp(daySunColor, sunsetSunColor, sunsetTime);
                
                // 太陽光の計算
                float3 sunIllumination = currentSunColor * 5.0 * lightScattering;

                // 2. ★環境光（雲の影の色）に、広範囲の夕焼け色をガッツリ足す！
                // これにより、太陽の光が直接当たっていない雲の裏側や、遠くの雲も夕焼け色に染まります。
                float3 cloudAmbient = (cloudAmbientSkyColor * 0.15) + (wideSunsetColor * 0.6);

                // 3. ベースの光に合成
                light = (light * 0.5) + sunIllumination + cloudAmbient;
                
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
    
    // ==========================================
    // ★【新規追加】コントラストと彩度の調整
    // ==========================================
    
    // 1. コントラストの調整（べき乗カーブ）
    // 値を 1.0 より大きくする（例: 1.2 ～ 1.5）と、中間の色が沈み込んでコントラストが強くなります。
    float contrast = 1.5;
    finalColor = pow(finalColor, float3(contrast, contrast, contrast));

    // 2. さらにシネマティックにするためのS字カーブ（お好みで！）
    // 暗い部分をより暗く、明るい部分をより明るくするS字補正です。
    finalColor = finalColor * finalColor * (3.0 - 2.0 * finalColor);

    // 3. （おまけ）彩度（色の鮮やかさ）の調整
    // コントラストを上げると色が濃くなりますが、さらに鮮やかにしたい場合に使います。
    float saturation = 1.2; // 1.0が元の色、大きくすると鮮やかに
    // グレースケール（明るさ）を計算
    float luminance = dot(finalColor, float3(0.299, 0.587, 0.114));
    // モノクロと元の色をブレンドして彩度を調整
    finalColor = lerp(float3(luminance, luminance, luminance), finalColor, saturation);

    return float4(finalColor, 1.0);
    
    // 背景として描画するため、アルファは 1.0
    return float4(finalColor, 1.0);
}