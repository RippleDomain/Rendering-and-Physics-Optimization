#pragma once

#include <glm.hpp>

struct FrustumPlane { glm::vec3 n; float d; };

FrustumPlane normalizePlane(FrustumPlane p);
void extractFrustumPlanes(const glm::mat4& m, FrustumPlane out[6]);
bool sphereIntersectsFrustum(const FrustumPlane planes[6], const glm::vec3& c, float r);