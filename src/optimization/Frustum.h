/*
	Frustum helpers header: plane struct and culling declarations.
*/

#pragma once

#include <glm.hpp>

//--FRUSTUM-PLANES-HELPERS--
struct FrustumPlane { glm::vec3 n; float d; }; //Plane: n.x * x + n.y * y + n.z * z + d = 0
FrustumPlane normalizePlane(FrustumPlane p);
void extractFrustumPlanes(const glm::mat4& mutex, FrustumPlane out[6]);
bool sphereIntersectsFrustum(const FrustumPlane planes[6], const glm::vec3& c, float r); //Sphere vs frustum test.
//--FRUSTUM-PLANES-HELPERS-END--