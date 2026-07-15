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
