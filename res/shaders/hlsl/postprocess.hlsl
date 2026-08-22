cbuffer PostConstants : register(b0) { float4 Params; float4 EffectParams; };
Texture2D SourceTexture : register(t0); Texture2D BloomTexture : register(t1); SamplerState LinearSampler : register(s0);
struct PSInput { float4 position : SV_POSITION; float2 uv : TEXCOORD; };
PSInput VSMain(uint id : SV_VertexID) { PSInput o; float2 p=float2((id<<1)&2,id&2); o.uv=p; o.position=float4(p*float2(2,-2)+float2(-1,1),0,1); return o; }
float4 Extract(PSInput input) : SV_TARGET { float3 c=SourceTexture.Sample(LinearSampler,input.uv).rgb; float b=dot(c,float3(0.2126,0.7152,0.0722)); float threshold=max(EffectParams.x,0.0f); return b>threshold ? float4(c,1) : float4(0,0,0,1); }
float4 Blur(PSInput input) : SV_TARGET { float2 d=Params.xy; float3 c=SourceTexture.Sample(LinearSampler,input.uv).rgb*0.227027; c+=SourceTexture.Sample(LinearSampler,input.uv+d*1.384615).rgb*0.316216; c+=SourceTexture.Sample(LinearSampler,input.uv-d*1.384615).rgb*0.316216; c+=SourceTexture.Sample(LinearSampler,input.uv+d*3.230769).rgb*0.070270; c+=SourceTexture.Sample(LinearSampler,input.uv-d*3.230769).rgb*0.070270; return float4(c,1); }
float3 ACES(float3 x) { return saturate((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14)); }
static const float Bayer4x4[16] = {
   0.0f,  8.0f,  2.0f, 10.0f,
  12.0f,  4.0f, 14.0f,  6.0f,
   3.0f, 11.0f,  1.0f,  9.0f,
  15.0f,  7.0f, 13.0f,  5.0f
};
float4 Composite(PSInput input) : SV_TARGET
{
  float2 sampleUV = input.uv;
  float2 virtualPixel = 0.0f;
  if (Params.w > 0.5f)
  {
    const float2 resolution = max(Params.yz, float2(1.0f, 1.0f));
    const float aspect = resolution.x / resolution.y;
    const float2 virtualResolution = float2(
      max(floor(max(EffectParams.y, 1.0f) * aspect + 0.5f), 1.0f), max(EffectParams.y, 1.0f));
    virtualPixel = floor(input.uv * virtualResolution);
    sampleUV = (virtualPixel + 0.5f) / virtualResolution;
  }

  float3 color = SourceTexture.Sample(LinearSampler, sampleUV).rgb;
  color += BloomTexture.Sample(LinearSampler, sampleUV).rgb * Params.x;
  color = pow(ACES(color * EffectParams.x), 1.0f / max(EffectParams.w, 0.001f));

  if (Params.w > 0.5f)
  {
    const uint2 ditherCoord = (uint2)virtualPixel & uint2(3, 3);
    const uint ditherIndex = ditherCoord.x + ditherCoord.y * 4;
    const float threshold = (Bayer4x4[ditherIndex] + 0.5f) / 16.0f;
    const float colorLevels = max(EffectParams.z, 1.0f);
    color = floor(saturate(color) * colorLevels + threshold) / colorLevels;
  }
  return float4(color, 1.0f);
}