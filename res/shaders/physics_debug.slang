#api OPENGL
#type VERTEX
#version 460 core

layout(location = 0) in vec3 aPos;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
  mat4 OrtoProjection;
  mat4 NonRotViewProjection;
  vec3 CameraPos;
};

layout(std430, binding = 13) readonly buffer InstanceTransforms
{
  mat4 instanceTransforms[];
};

void main()
{
  mat4 model = instanceTransforms[gl_BaseInstance + gl_InstanceID];
  gl_Position = ViewProjection * model * vec4(aPos, 1.0);
}

#type FRAGMENT
#version 460 core

layout(location = 0) out vec4 FragColor;
uniform vec4 u_Color;

void main()
{
  FragColor = u_Color;
}

#api DX12
cbuffer PhysicsDebugConstants : register(b0)
{
  float4x4 ViewProjection;
  float4x4 Model;
  float4 Color;
};
struct VSInput { float3 position : POSITION; };
float4 VSMain(VSInput input) : SV_POSITION
{
  return mul(ViewProjection, mul(Model, float4(input.position, 1.0f)));
}
float4 PSMain() : SV_TARGET { return Color; }
