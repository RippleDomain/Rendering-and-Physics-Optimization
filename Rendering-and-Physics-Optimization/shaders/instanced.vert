#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aNormalPacked;

layout (location = 2) in vec3  iPos;          //float3
layout (location = 3) in float iScale;        //half -> promoted to float
layout (location = 4) in vec4  iColorUNorm;   //UNORM8x4 normalized
layout (location = 5) in float iAngle;        //half -> float

uniform mat4 uVP;

out vec3 vNormal;
out vec3 vWorldPos;
out vec3 vBaseColor;

mat3 rotY(float a){ float c=cos(a), s=sin(a); return mat3(c,0,s, 0,1,0, -s,0,c); }

void main()
{
    vec3  N0    = normalize(aNormalPacked.xyz);
    mat3  R     = rotY(iAngle);
    vec3  local = aPos * iScale;
    vec3  world = iPos + R * local;

    vWorldPos  = world;
    vNormal    = normalize(R * N0);
    vBaseColor = iColorUNorm.rgb;

    gl_Position = uVP * vec4(world, 1.0);
}