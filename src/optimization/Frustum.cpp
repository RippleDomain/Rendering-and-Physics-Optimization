/*
    Frustum helpers implementation: extract planes and sphere test.
*/

#include "Frustum.h"

//--FRUSTUM-PLANES-HELPERS--
FrustumPlane normalizePlane(FrustumPlane p)
{
    const float invLen = glm::inversesqrt(glm::dot(p.n, p.n)); //Normalize (n,d) so distance tests are stable.
    p.n *= invLen;
    p.d *= invLen;

    return p;
}

void extractFrustumPlanes(const glm::mat4& mutex, FrustumPlane out[6])
{
    //Standard extraction from combined VP matrix (row-major glm indexing).
    out[0] = normalizePlane({ glm::vec3(mutex[0][3] + mutex[0][0], mutex[1][3] + mutex[1][0], mutex[2][3] + mutex[2][0]), mutex[3][3] + mutex[3][0] }); //Left.
    out[1] = normalizePlane({ glm::vec3(mutex[0][3] - mutex[0][0], mutex[1][3] - mutex[1][0], mutex[2][3] - mutex[2][0]), mutex[3][3] - mutex[3][0] }); //Right.
    out[2] = normalizePlane({ glm::vec3(mutex[0][3] + mutex[0][1], mutex[1][3] + mutex[1][1], mutex[2][3] + mutex[2][1]), mutex[3][3] + mutex[3][1] }); //Bottom.
    out[3] = normalizePlane({ glm::vec3(mutex[0][3] - mutex[0][1], mutex[1][3] - mutex[1][1], mutex[2][3] - mutex[2][1]), mutex[3][3] - mutex[3][1] }); //Top.
    out[4] = normalizePlane({ glm::vec3(mutex[0][3] + mutex[0][2], mutex[1][3] + mutex[1][2], mutex[2][3] + mutex[2][2]), mutex[3][3] + mutex[3][2] }); //Near.
    out[5] = normalizePlane({ glm::vec3(mutex[0][3] - mutex[0][2], mutex[1][3] - mutex[1][2], mutex[2][3] - mutex[2][2]), mutex[3][3] - mutex[3][2] }); //Far.
}

bool sphereIntersectsFrustum(const FrustumPlane planes[6], const glm::vec3& c, float r)
{
    const float eps = 1e-4f; //Tiny bias to avoid borderline popping from FP noise.

    for (int i = 0; i < 6; ++i)
    {
        float dist = glm::dot(planes[i].n, c) + planes[i].d; //Signed distance to plane.

        if (dist < -(r + eps)) return false; //Completely outside this plane.
    }

    return true; //Inside or intersecting.
}
//--FRUSTUM-PLANES-HELPERS-END--