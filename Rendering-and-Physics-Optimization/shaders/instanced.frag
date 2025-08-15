#version 330 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vBaseColor;

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uCamPos;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);

    float NdotL = max(dot(N, L), 0.0);
    vec3 ambient = 0.15 * vBaseColor;
    vec3 diffuse = NdotL * vBaseColor;

    FragColor = vec4(ambient + diffuse, 1.0);
}