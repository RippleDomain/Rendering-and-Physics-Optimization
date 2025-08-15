#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 iPosScale;
layout (location = 3) in vec4 iColorAngle;

uniform mat4 uVP;

out vec3 vNormal;
out vec3 vWorldPos;
out vec3 vBaseColor;

mat3 rotY(float a){ float c=cos(a), s=sin(a); return mat3(c,0,s, 0,1,0, -s,0,c); }

void main(){
    vec3 pos   = iPosScale.xyz;
    float sca  = iPosScale.w;
    vec3 color = iColorAngle.rgb;
    float ang  = iColorAngle.a;

    mat3 R = rotY(ang);
    vec3 local = aPos * sca;
    vec3 world = pos + R * local;

    vWorldPos  = world;
    vNormal    = normalize(R * aNormal);
    vBaseColor = color;

    gl_Position = uVP * vec4(world, 1.0);
}