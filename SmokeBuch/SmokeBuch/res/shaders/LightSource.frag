#version 330 core


out vec4 FragColor;  
in vec3 ourColor;
in vec2 TexCoord;


uniform sampler2D Texture1;
uniform sampler2D Texture2;
uniform vec3 lightColor;
uniform vec3 objectColor;
  
void main()
{

  FragColor = vec4(1.0);

}