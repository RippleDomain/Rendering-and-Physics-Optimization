#include "Sphere.h"

#include <gtc/matrix_transform.hpp>
#include <gtc/packing.hpp>
#include <cmath>
#include <algorithm>

//--FAST-RNG-FOR-COLORS-- (tiny, no <random> overhead)
namespace
{
    inline uint32_t xs32(uint32_t& s) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float u01(uint32_t v) { return (float)((v >> 8) * (1.0 / 16777215.0)); } //24-bit to [0,1)
}
//--FAST-RNG-FOR-COLORS-END--

Sphere::Sphere(unsigned XSegments, unsigned YSegments, const glm::vec3& getPosition, float getScale) : position(getPosition), scale(getScale)
{
    //--FAST-COLOR--
    uint32_t seed = 0x9E3779B9u ^ (uint32_t)(uintptr_t)this;
    float r = 0.1f + 0.9f * u01(xs32(seed));
    float g = 0.1f + 0.9f * u01(xs32(seed));
    float b = 0.1f + 0.9f * u01(xs32(seed));
    color = glm::vec3(r, g, b);
    //--FAST-COLOR-END--

    mass = scale * scale * scale;

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
    (void)XSegments; (void)YSegments;
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
    velocity += acceleration * dt;
    position += velocity * dt;
}

void Sphere::collide(Sphere& other, float restitution)
{
    glm::vec3 d = position - other.position;
    float dist2 = glm::dot(d, d);
    float r = scale + other.scale;
    float r2 = r * r;

    if (dist2 >= r2) return;

    float dist = std::sqrt(std::max(dist2, 1e-8f));
    glm::vec3 n = (dist > 1e-6f) ? (d / dist) : glm::vec3(1.0f, 0.0f, 0.0f);

    float invA = 1.0f / mass;
    float invB = 1.0f / other.mass;

    float penetration = r - dist;
    const float percent = 0.8f;
    const float slop = 0.001f;
    glm::vec3 correction = ((std::max(penetration - slop, 0.0f) / (invA + invB)) * percent) * n;
    position += correction * invA;
    other.position -= correction * invB;

    glm::vec3 rv = velocity - other.velocity;
    float velAlongNormal = glm::dot(rv, n);
    if (velAlongNormal > 0.0f) return;

    float j = -(1.0f + restitution) * velAlongNormal;
    j /= (invA + invB);

    glm::vec3 impulse = j * n;
    velocity += impulse * invA * 1.0001f; //1.0001f multiplier in order to move the spheres around more.
    other.velocity -= impulse * invB * 1.0001f; //1.0001f multiplier in order to move the spheres around more.
}