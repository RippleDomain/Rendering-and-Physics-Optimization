#version 330 core

in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uCamPos;
uniform vec3 uBaseColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);

    float NdotL = max(dot(N, L), 0.0);
    vec3 ambient = 0.15 * uBaseColor;
    vec3 diffuse = NdotL * uBaseColor;

    FragColor = vec4(ambient + diffuse, 1.0);
}