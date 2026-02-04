#type VERTEX
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_TilingFactor;
layout(location = 5) in int a_EntityID;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
	mat4 OrtoProjection;
	mat4 NonRotViewProjection;
	vec3 CameraPos;
};

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) out VertexOutput Output;
layout (location = 3) out flat float v_TexIndex;
layout (location = 4) out flat int v_EntityID;

uniform bool u_Is3D;

void main()
{
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	Output.TilingFactor = a_TilingFactor;
	v_TexIndex = a_TexIndex;
	v_EntityID = a_EntityID;

  mat4 projection = u_Is3D ? ViewProjection : OrtoProjection;

	gl_Position = projection * vec4(a_Position, 1.0);
}

#type FRAGMENT
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) in VertexOutput Input;
layout (location = 3) in flat float v_TexIndex;
layout (location = 4) in flat int v_EntityID;

layout (binding = 0) uniform sampler2D u_Textures[32];

void main()
{
  vec4 texColor = Input.Color;

  int index = clamp(int(v_TexIndex), 0, 31);
  texColor *= texture(u_Textures[index], Input.TexCoord * Input.TilingFactor);

	if (texColor.a < 0.01f) discard;

  float brightness = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));
  BrightColor = brightness < 1.0 ? texColor : vec4(0.0);

	o_Color = texColor;
	o_EntityID = v_EntityID;
}
