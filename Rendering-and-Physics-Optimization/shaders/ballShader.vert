#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aNormalPacked;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vNormal;
out vec3 vWorldPos;

void main()
{
    vWorldPos = vec3(uModel * vec4(aPos, 1.0));

    vec3 N = normalize(aNormalPacked.xyz);
    vNormal = mat3(transpose(inverse(uModel))) * N;
    gl_Position = uMVP * vec4(aPos, 1.0);
}