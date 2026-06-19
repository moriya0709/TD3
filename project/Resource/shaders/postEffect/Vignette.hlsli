float3 Vignette(float4 color, float vignetteIntensity, float2 uv)
{
    // Distance from the center
    float dist = distance(uv, float2(0.5f, 0.5f));
            
    // Gradient
    float vignetteWeight = smoothstep(0.3f, 0.8f, dist);
    vignetteWeight *= saturate(vignetteIntensity);

    // Color
    float3 damageColor = float3(0.6f, 0.0f, 0.0f);

    return color.rgb = lerp(color.rgb, damageColor, vignetteWeight);
}