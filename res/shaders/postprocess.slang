#api OPENGL
#type VERTEX
#version 410 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 0) out vec2 v_TexCoord;

void main()
{
  v_TexCoord = a_TexCoord;
  gl_Position = vec4(a_Position, 0.0, 1.0);
}

#type FRAGMENT
#version 420 core

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 FragColor;

uniform sampler2D u_Texture;
uniform bool u_PS1Effect;
uniform float u_PS1VirtualHeight;
uniform float u_PS1ColorLevels;

layout(std140, binding = 1) uniform Resolution
{
  vec2 resolution;
};

#define GABGL_FLOAT2 vec2
#define GABGL_FLOAT3 vec3
#define GABGL_INT2 ivec2
#define GABGL_SATURATE(value) clamp(value, 0.0, 1.0)
#include "common/ps1_postprocess.slang"

void main()
{
  if (!u_PS1Effect)
  {
    FragColor = texture(u_Texture, v_TexCoord);
    return;
  }

  vec2 virtualPixel;
  vec2 snappedUV = GabglPS1SampleUV(
    v_TexCoord, resolution, u_PS1VirtualHeight, virtualPixel);

  vec4 source = texture(u_Texture, snappedUV);
  FragColor = vec4(
    GabglPS1Quantize(source.rgb, virtualPixel, u_PS1ColorLevels), source.a);
}

#api DX12
cbuffer PostConstants : register(b0) { float4 Params; float4 EffectParams; };
Texture2D SourceTexture : register(t0); Texture2D BloomTexture : register(t1); SamplerState LinearSampler : register(s0);
struct PSInput { float4 position : SV_POSITION; float2 uv : TEXCOORD; };
#define GABGL_FLOAT2 float2
#define GABGL_FLOAT3 float3
#define GABGL_INT2 int2
#define GABGL_SATURATE(value) saturate(value)
#include "common/ps1_postprocess.slang"
PSInput VSMain(uint id : SV_VertexID) { PSInput o; float2 p=float2((id<<1)&2,id&2); o.uv=p; o.position=float4(p*float2(2,-2)+float2(-1,1),0,1); return o; }
float4 Extract(PSInput input) : SV_TARGET { float3 c=SourceTexture.Sample(LinearSampler,input.uv).rgb; float b=dot(c,float3(0.2126,0.7152,0.0722)); float threshold=max(EffectParams.x,0.0f); return b>threshold ? float4(c,1) : float4(0,0,0,1); }
float4 Blur(PSInput input) : SV_TARGET { float2 d=Params.xy; float3 c=SourceTexture.Sample(LinearSampler,input.uv).rgb*0.227027; c+=SourceTexture.Sample(LinearSampler,input.uv+d*1.384615).rgb*0.316216; c+=SourceTexture.Sample(LinearSampler,input.uv-d*1.384615).rgb*0.316216; c+=SourceTexture.Sample(LinearSampler,input.uv+d*3.230769).rgb*0.070270; c+=SourceTexture.Sample(LinearSampler,input.uv-d*3.230769).rgb*0.070270; return float4(c,1); }
float3 ACES(float3 x) { return saturate((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14)); }
float4 Composite(PSInput input) : SV_TARGET
{
  float3 color = SourceTexture.Sample(LinearSampler, input.uv).rgb;
  color += BloomTexture.Sample(LinearSampler, input.uv).rgb * Params.x;
  color = pow(ACES(color * EffectParams.x), 1.0f / max(EffectParams.w, 0.001f));
  return float4(color, 1.0f);
}

float4 PostProcess(PSInput input) : SV_TARGET
{
  float2 sampleUV = input.uv;
  float2 virtualPixel = 0.0f;
  if (Params.w > 0.5f)
  {
    sampleUV = GabglPS1SampleUV(
      input.uv, Params.yz, EffectParams.y, virtualPixel);
  }

  float3 color = SourceTexture.Sample(LinearSampler, sampleUV).rgb;

  if (Params.w > 0.5f)
  {
    color = GabglPS1Quantize(color, virtualPixel, EffectParams.z);
  }
  return float4(color, 1.0f);
}
