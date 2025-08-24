#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 iRect;
layout(location = 2) in vec4 iColor;

uniform vec2 uScreen;

out vec4 vColor;

void main()
{
    vec2 p   = aPos * iRect.zw + iRect.xy;
    vec2 ndc = vec2((p.x + 0.5) / uScreen.x * 2.0 - 1.0, 1.0 - (p.y + 0.5) / uScreen.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor = iColor;
}
