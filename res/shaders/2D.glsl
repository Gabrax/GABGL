#type VERTEX
#version 330 core

layout(location = 0) in vec3 aPos;   // Position input
//layout(location = 1) in vec2 aTexCoord; // Texture coordinate input

out vec2 TexCoord; // Pass texture coordinates to fragment shader

void main()
{
    gl_Position = vec4(aPos, 1.0); // Transform the vertex position to clip space
    //TexCoord = aTexCoord;         // Pass through the texture coordinates
}

#type FRAGMENT
#version 330 core

//in vec2 TexCoord;          // Interpolated texture coordinates from vertex shader
out vec4 FragColor;        // Output color

//uniform sampler2D texture1; // Texture sampler

void main()
{
    FragColor = vec4(1,1,1,1);//texture(texture1, TexCoord); // Sample the texture
}