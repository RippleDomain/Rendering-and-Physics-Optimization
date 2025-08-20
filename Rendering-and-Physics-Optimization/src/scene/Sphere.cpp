#include "Sphere.h"

#include <gtc/matrix_transform.hpp>
#include <gtc/packing.hpp>
#include <cmath>
#include <random>
#include <algorithm>

namespace
{
    std::mt19937& colorRNG()
    {
        static thread_local std::mt19937 gen{ std::random_device{}() };

        return gen;
    }

    std::uniform_real_distribution<float> colorDist(0.1f, 1.0f);
}

Sphere::Sphere(unsigned XSegments, unsigned YSegments, const glm::vec3& getPosition, float getScale) : position(getPosition), scale(getScale)
{
    color = glm::vec3(colorDist(colorRNG()), colorDist(colorRNG()), colorDist(colorRNG()));
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
    M = glm::rotate(M, dt * 0.5f, glm::vec3(0, 1, 0));
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
    velocity += impulse * invA * 1.0f;
    other.velocity -= impulse * invB * 1.0f;
}