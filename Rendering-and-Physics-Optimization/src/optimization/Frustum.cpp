#include "Frustum.h"

//--FRUSTUM-PLANES-HELPERS--
FrustumPlane normalizePlane(FrustumPlane p)
{
    float invLen = 1.0f / glm::length(p.n);
    p.n *= invLen; p.d *= invLen;

    return p;
}

void extractFrustumPlanes(const glm::mat4& m, FrustumPlane out[6])
{
    out[0] = normalizePlane({ glm::vec3(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0]), m[3][3] + m[3][0] });
    out[1] = normalizePlane({ glm::vec3(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0]), m[3][3] - m[3][0] });
    out[2] = normalizePlane({ glm::vec3(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1]), m[3][3] + m[3][1] });
    out[3] = normalizePlane({ glm::vec3(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1]), m[3][3] - m[3][1] });
    out[4] = normalizePlane({ glm::vec3(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2]), m[3][3] + m[3][2] });
    out[5] = normalizePlane({ glm::vec3(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2]), m[3][3] - m[3][2] });
}

bool sphereIntersectsFrustum(const FrustumPlane planes[6], const glm::vec3& c, float r)
{
    const float eps = 1e-4f;

    for (int i = 0; i < 6; ++i)
    {
        float dist = glm::dot(planes[i].n, c) + planes[i].d;

        if (dist < -(r + eps)) return false;
    }

    return true;
}
//--FRUSTUM-PLANES-HELPERS-END--