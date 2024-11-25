#type VERTEX
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    // vs_out.FragPos = vec3(model * vec4(aPos, 1.0));   
    // vs_out.TexCoords = aTexCoords;
    //
    // mat3 normalMatrix = transpose(inverse(mat3(model)));
    // vs_out.Normal = normalize(normalMatrix * aNormal);
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
 

#type FRAGMENT
#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform vec4 lightColor;

// Gamma correction parameters
const float gamma = 2.2;

float gammaCorrection(float value) {
    return pow(value, 1.0 / gamma);
}

vec3 gammaCorrection(vec3 value) {
    return pow(value, vec3(1.0 / gamma));
}

void main()
{           
    FragColor = lightColor;
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness < 1.0)
      BrightColor = FragColor;
    else
      BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}

