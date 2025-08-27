/*
    Sphere implementation: fast color seed, integration, and collision resolve.
*/

#include "Sphere.h"

#include <gtc/matrix_transform.hpp>
#include <gtc/packing.hpp>
#include <cmath>
#include <algorithm>

Sphere::Sphere(unsigned XSegments, unsigned YSegments, const glm::vec3& getPosition, float getScale) : position(getPosition), scale(getScale)
{
    //--COLORING--
    uint32_t seed = 0x9E3779B9u ^ (uint32_t)(uintptr_t)this; //Per-instance seed using pointer bits.
    float r = 0.1f + 0.9f * colorRNG::u01(colorRNG::xs32(seed));
    float g = 0.1f + 0.9f * colorRNG::u01(colorRNG::xs32(seed));
    float b = 0.1f + 0.9f * colorRNG::u01(colorRNG::xs32(seed));
    color = glm::vec3(r, g, b);
    //--COLORING-END--

    mass = scale * scale * scale; //Rough mass from volume for same density.

    (void)XSegments;
    (void)YSegments;
}

Sphere::Sphere(Sphere&& other) noexcept
{
    position = other.position;
    scale = other.scale;
    color = other.color;
    velocity = other.velocity;
    mass = other.mass;
}

Sphere& Sphere::operator=(Sphere&& other) noexcept
{
    if (this == &other) return *this;

    position = other.position;
    scale = other.scale;
    color = other.color;
    velocity = other.velocity;
    mass = other.mass;

    return *this;
}

void Sphere::build(unsigned XSegments, unsigned YSegments)
{
    (void)XSegments; (void)YSegments; //Geometry lives in Instance, this is a placeholder.
}

glm::mat4 Sphere::modelMatrix(float dt) const
{
    glm::mat4 M(1.0f);

    M = glm::translate(M, position);
    M = glm::scale(M, glm::vec3(scale));

    return M;
}

void Sphere::applyGravity(const glm::vec3& acceleration, float dt)
{
    velocity += acceleration * dt; //Euler.
    position += velocity * dt;
}

void Sphere::collide(Sphere& other, float restitution)
{
    glm::vec3 d = position - other.position;
    float d2 = glm::dot(d, d);
    float r = scale + other.scale;
    float r2 = r * r;

    if (d2 >= r2) return; //No overlap.

    float distance = std::sqrt(std::max(d2, 1e-8f));
    glm::vec3 n = (distance > 1e-6f) ? (d / distance) : glm::vec3(1.0f, 0.0f, 0.0f); //Fallback axis if overlapping too much.

    float invA = 1.0f / mass;
    float invB = 1.0f / other.mass;

    float penetration = r - distance;

    //Keeps the method from overcorrecting and reduces jitter for tiny overlaps.
    const float percent = 0.8f; //Positional correction to resolve interpenetration.
    const float slop = 0.001f;

    glm::vec3 correction = ((std::max(penetration - slop, 0.0f) / (invA + invB)) * percent) * n;
    position += correction * invA;
    other.position -= correction * invB;

    glm::vec3 relativeVelocity = velocity - other.velocity;
    float velocityAlongNormal = glm::dot(relativeVelocity, n);
    if (velocityAlongNormal > 0.0f) return; //Currently separating, nothing to do.

    float j = -(1.0f + restitution) * velocityAlongNormal;
    j /= (invA + invB);

    glm::vec3 impulse = j * n;
    velocity += impulse * invA;
    other.velocity -= impulse * invB;

    //--TINY-TANGENTIAL-FRICTION--
    const float mu = 0.02f;
    glm::vec3 t = relativeVelocity - n * velocityAlongNormal; //Tangential relative velocity.
    float t2 = glm::dot(t, t);

    if (t2 > 1e-12f)
    {
        glm::vec3 tdir = t * (1.0f / std::sqrt(t2));
        glm::vec3 ft = -mu * j * tdir; //Very small friction to damp sliding.
        velocity += ft * invA;
        other.velocity -= ft * invB;
    }
    //--TINY-TANGENTIAL-FRICTION-END--
}