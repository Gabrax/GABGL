#type VERTEX
#version 450 core

layout(location = 0) in vec2 a_QuadPosition;
layout(location = 1) in vec4 a_PositionAndSize;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_Rotation;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
  mat4 OrtoProjection;
  mat4 NonRotViewProjection;
  vec3 CameraPos;
};

uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;

layout(location = 0) out vec2 v_LocalPosition;
layout(location = 1) out vec4 v_Color;

void main()
{
  float cosine = cos(a_Rotation);
  float sine = sin(a_Rotation);
  vec2 rotatedPosition = mat2(cosine, sine, -sine, cosine) * a_QuadPosition;
  vec3 worldPosition = a_PositionAndSize.xyz
    + u_CameraRight * rotatedPosition.x * a_PositionAndSize.w
    + u_CameraUp * rotatedPosition.y * a_PositionAndSize.w;

  v_LocalPosition = a_QuadPosition * 2.0;
  v_Color = a_Color;
  gl_Position = ViewProjection * vec4(worldPosition, 1.0);
}

#type FRAGMENT
#version 450 core

layout(location = 0) in vec2 v_LocalPosition;
layout(location = 1) in vec4 v_Color;
layout(location = 0) out vec4 o_Color;

void main()
{
  float softEdge = 1.0 - smoothstep(0.15, 1.0, length(v_LocalPosition));
  o_Color = vec4(v_Color.rgb, v_Color.a * softEdge);
  if (o_Color.a < 0.01)
    discard;
}
