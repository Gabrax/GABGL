#api OPENGL
#type VERTEX
#version 450 core

layout(location = 0) in vec2 a_QuadPosition;
layout(location = 1) in vec4 a_PositionAndSize;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_Rotation;
layout(location = 4) in vec4 a_RightAndStyle;
layout(location = 5) in vec4 a_Up;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
  mat4 OrtoProjection;
  mat4 NonRotViewProjection;
  vec3 CameraPos;
};

layout(location = 0) out vec2 v_LocalPosition;
layout(location = 1) out vec4 v_Color;
layout(location = 2) flat out float v_IsSquare;

void main()
{
  float cosine = cos(a_Rotation);
  float sine = sin(a_Rotation);
  vec2 rotatedPosition = mat2(cosine, sine, -sine, cosine) * a_QuadPosition;
  vec3 worldPosition = a_PositionAndSize.xyz
    + a_RightAndStyle.xyz * rotatedPosition.x * a_PositionAndSize.w
    + a_Up.xyz * rotatedPosition.y * a_PositionAndSize.w;

  v_LocalPosition = a_QuadPosition * 2.0;
  v_Color = a_Color;
  v_IsSquare = a_RightAndStyle.w;
  gl_Position = ViewProjection * vec4(worldPosition, 1.0);
}

#type FRAGMENT
#version 450 core

layout(location = 0) in vec2 v_LocalPosition;
layout(location = 1) in vec4 v_Color;
layout(location = 2) flat in float v_IsSquare;
layout(location = 0) out vec4 o_Color;

void main()
{
  float softEdge = v_IsSquare > 0.5
    ? 1.0
    : 1.0 - smoothstep(0.15, 1.0, length(v_LocalPosition));
  o_Color = vec4(v_Color.rgb, v_Color.a * softEdge);
  if (o_Color.a < 0.01)
    discard;
}

#api DX12
cbuffer ParticleConstants : register(b0) { float4x4 ViewProjection; };
struct VSInput { float3 position : POSITION; float4 color : COLOR; float2 localPosition : TEXCOORD; float isSquare : STYLE; };
struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; float2 localPosition : TEXCOORD; nointerpolation float isSquare : STYLE; };
PSInput VSMain(VSInput input) { PSInput o; o.position=mul(ViewProjection,float4(input.position,1)); o.color=input.color; o.localPosition=input.localPosition; o.isSquare=input.isSquare; return o; }
float4 PSMain(PSInput input) : SV_TARGET { float edge=input.isSquare>0.5?1.0:1.0-smoothstep(0.15,1.0,length(input.localPosition)); float4 c=float4(input.color.rgb,input.color.a*edge); clip(c.a-0.01); return c; }
