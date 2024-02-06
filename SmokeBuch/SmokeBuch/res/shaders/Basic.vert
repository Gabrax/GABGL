#version 330 core
layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor; // the color variable has attribute position 1

//vec4 gl_Position;  
out vec3 ourColor; // output a color to the fragment shader
out vec2 TexCoord;

// light
out vec3 FragPos;
out vec3 Normal;

//uniform mat4 transform;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal; 

    gl_Position =   projection * view * vec4(FragPos, 1.0);
    ourColor = aColor; // set ourColor to the input color we got from the vertex data
    TexCoord = aTexCoord;
}