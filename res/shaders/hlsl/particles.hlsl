cbuffer ParticleConstants : register(b0) { float4x4 ViewProjection; };
struct VSInput { float3 position : POSITION; float4 color : COLOR; float2 localPosition : TEXCOORD; float isSquare : STYLE; };
struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; float2 localPosition : TEXCOORD; nointerpolation float isSquare : STYLE; };
PSInput VSMain(VSInput input) { PSInput o; o.position=mul(ViewProjection,float4(input.position,1)); o.color=input.color; o.localPosition=input.localPosition; o.isSquare=input.isSquare; return o; }
float4 PSMain(PSInput input) : SV_TARGET { float edge=input.isSquare>0.5?1.0:1.0-smoothstep(0.15,1.0,length(input.localPosition)); float4 c=float4(input.color.rgb,input.color.a*edge); clip(c.a-0.01); return c; }