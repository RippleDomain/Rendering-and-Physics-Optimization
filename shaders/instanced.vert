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

void main()
{
    vec3 N0    = normalize(aNormalPacked.xyz);
    vec3 local = aPos * iScale;
    vec3 world = iPos + local;

    vWorldPos  = world;
    vNormal    = N0;
    vBaseColor = iColorUNorm.rgb;

    gl_Position = uVP * vec4(world, 1.0);
}