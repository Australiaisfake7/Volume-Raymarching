#version 460 core
out vec4 fragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    vec3 mapped = pow(color, vec3(1.0/2.2));
    fragColor = vec4(mapped, 1.0);
}