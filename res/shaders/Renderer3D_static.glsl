#type VERTEX
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;
layout (location = 3) in int a_EntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec4 Color;
};

layout (location = 0) out VertexOutput Output;
layout (location = 3) out flat int v_EntityID;

void main()
{
	Output.Color = vec4(1.0f,1.0f,0.0f,1.0f);
	v_EntityID = a_EntityID;

	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type FRAGMENT
#version 450 core

layout (location = 0) out vec4 o_Color;
layout (location = 1) out vec4 BrightColor;
layout (location = 3) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
};

layout (location = 0) in VertexOutput Input;
layout (location = 3) in flat int v_EntityID;

const float gamma = 2.2;

float gammaCorrection(float value) {
    return pow(value, 1.0 / gamma);
}

vec3 gammaCorrection(vec3 value) {
    return pow(value, vec3(1.0 / gamma));
}

void main()
{
    vec4 texColor = Input.Color;

    // if(texColor.a < 0.01)
    //     discard;

    float brightness = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness < 1.0)
        BrightColor = texColor;  // Output bright color for bloom
    else
        BrightColor = vec4(0.0); // No brightness contribution

    o_Color = texColor;
    o_EntityID = v_EntityID;
}
