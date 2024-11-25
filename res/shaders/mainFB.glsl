#type VERTEX
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}  

#type FRAGMENT
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;  
uniform sampler2D mainDepthTexture;  
uniform sampler2D bloomTexture; 
uniform sampler2D bloomDepthTexture;

uniform float renderWidth;
uniform float renderHeight;

uniform float pixelWidth = 5.0;
uniform float pixelHeight = 5.0;


void main() {

    float dx = pixelWidth*(1.0/renderWidth);
    float dy = pixelHeight*(1.0/renderHeight);

    vec2 coord = vec2(dx*floor(TexCoords.x/dx), dy*floor(TexCoords.y/dy));

    vec4 t1 = texture(screenTexture, coord);
    vec4 t2 = texture(bloomTexture, coord);

    float sceneDepth = texture(mainDepthTexture,TexCoords).r;
    float bloomDepth = texture(bloomDepthTexture,TexCoords).r;

    float near = 0.1; // Near plane distance
    float far = 100.0; // Far plane distance

    float linearDepth = (2.0 * near * far) / (far + near - sceneDepth * (far - near));
    float linearDepth2 = (2.0 * near * far) / (far + near - bloomDepth * (far - near));

    // Only apply bloom if it's in front of the current scene fragment
    // if (linearDepth2 < linearDepth - 0.1) {
    //     FragColor = clamp(t1 + t2,0.0,1.0);
    // } else {
    //     // FragColor = texture(t1.rgb,TexCoords);
    //     FragColor = texture(bloomTexture,TexCoords); // Use the main scene color
    // }

    // FragColor = vec4(vec3(linearDepth / far), 1.0); // Normalize to [0,1]
    // FragColor = clamp(t2 + t1 * 1.5f,0.0,1.0);
    // FragColor = mix(t1,t2,t1.c);
    FragColor = texture(screenTexture,TexCoords);
}
