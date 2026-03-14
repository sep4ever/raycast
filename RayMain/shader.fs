#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform vec2 resolution;   // размер экрана
uniform vec2 mouse;        // позиция мыши
uniform float time;        // время для анимации

void main()
{
    vec2 uv = fragTexCoord * resolution;
    float dist = length(uv - mouse);
    float brightness = 1.0 / (dist*0.01 + 1.0);
    finalColor = vec4(brightness, brightness*0.5, 1.0 - brightness, 1.0);
}