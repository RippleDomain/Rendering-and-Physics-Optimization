#include "Sphere.h"

#include <gtc/matrix_transform.hpp>
#include <cmath>
#include <random>
#include <algorithm>

namespace
{
    //RNG method for random colors.
    std::mt19937& rng()
    {
        static thread_local std::mt19937 gen{ std::random_device{}() };
        return gen;
    }

    //Vibrant-ish color range.
    std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
}

Sphere::Sphere(unsigned XSegments, unsigned YSegments, const glm::vec3& getPosition, float getScale) : m_position(getPosition), m_scale(getScale)
{
    //Each ball gets a random color.
    m_color = glm::vec3(colorDist(rng()), colorDist(rng()), colorDist(rng()));
    m_mass = m_scale * m_scale * m_scale;
    build(XSegments, YSegments);
}

Sphere::~Sphere()
{
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

Sphere::Sphere(Sphere&& other) noexcept
{
    m_vao = other.m_vao; other.m_vao = 0;
    m_vbo = other.m_vbo; other.m_vbo = 0;
    m_ebo = other.m_ebo; other.m_ebo = 0;
    m_indexCount = other.m_indexCount; other.m_indexCount = 0;

    m_position = other.m_position;
    m_scale = other.m_scale;
    m_color = other.m_color;

    m_velocity = other.m_velocity;
    m_mass = other.m_mass;
}

Sphere& Sphere::operator=(Sphere&& other) noexcept
{
    if (this == &other) return *this;

    //Clean up existing resources.
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);

    m_vao = other.m_vao; other.m_vao = 0;
    m_vbo = other.m_vbo; other.m_vbo = 0;
    m_ebo = other.m_ebo; other.m_ebo = 0;
    m_indexCount = other.m_indexCount; other.m_indexCount = 0;

    m_position = other.m_position;
    m_scale = other.m_scale;
    m_color = other.m_color;

    m_velocity = other.m_velocity;
    m_mass = other.m_mass;

    return *this;
}

void Sphere::build(unsigned XSegments, unsigned YSegments)
{
    std::vector<float> vertices;
    std::vector<unsigned> indices;

    vertices.reserve((XSegments + 1) * (YSegments + 1) * 6);
    indices.reserve(XSegments * YSegments * 6);

    //Vertices.
    for (unsigned y = 0; y <= YSegments; ++y)
    {
        for (unsigned x = 0; x <= XSegments; ++x)
        {
            float xs = static_cast<float>(x) / static_cast<float>(XSegments);
            float ys = static_cast<float>(y) / static_cast<float>(YSegments);
            float phi = xs * 6.2831853071795864769f;
            float theta = ys * 3.14159265358979323846f;

            float px = std::cos(phi) * std::sin(theta);
            float py = std::cos(theta);
            float pz = std::sin(phi) * std::sin(theta);

            //Position.
            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);

            //Normal.
            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);
        }
    }

    //Indices.
    for (unsigned y = 0; y < YSegments; ++y)
    {
        for (unsigned x = 0; x < XSegments; ++x)
        {
            unsigned i0 = y * (XSegments + 1) + x;
            unsigned i1 = (y + 1) * (XSegments + 1) + x;
            unsigned i2 = (y + 1) * (XSegments + 1) + x + 1;
            unsigned i3 = y * (XSegments + 1) + x + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    m_indexCount = static_cast<GLsizei>(indices.size());

    // GL buffers
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

    GLsizei stride = (3 + 3) * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

glm::mat4 Sphere::modelMatrix(float timeSeconds) const
{
    glm::mat4 M(1.0f);

    M = glm::translate(M, m_position);
    M = glm::rotate(M, timeSeconds * 0.5f, glm::vec3(0, 1, 0));
    M = glm::scale(M, glm::vec3(m_scale));

    return M;
}

void Sphere::draw() const
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Sphere::applyGravity(const glm::vec3& acceleration, float dt)
{
    m_velocity += acceleration * dt;
    m_position += m_velocity * dt;
}

void Sphere::collide(Sphere& other, float restitution)
{
    glm::vec3 d = m_position - other.m_position;
    float dist2 = glm::dot(d, d);
    float r = m_scale + other.m_scale;
    float r2 = r * r;

    if (dist2 >= r2) return;

    float dist = std::sqrt(std::max(dist2, 1e-8f));
    glm::vec3 n = (dist > 1e-6f) ? (d / dist) : glm::vec3(1.0f, 0.0f, 0.0f);

    float invA = 1.0f / m_mass;
    float invB = 1.0f / other.m_mass;

    float penetration = r - dist;
    const float percent = 0.8f;
    const float slop = 0.001f;
    glm::vec3 correction = ((std::max(penetration - slop, 0.0f) / (invA + invB)) * percent) * n;
    m_position += correction * invA;
    other.m_position -= correction * invB;

    glm::vec3 rv = m_velocity - other.m_velocity;
    float velAlongNormal = glm::dot(rv, n);
    if (velAlongNormal > 0.0f) return;

    float j = -(1.0f + restitution) * velAlongNormal;
    j /= (invA + invB);

    glm::vec3 impulse = j * n;
    m_velocity += impulse * invA;
    other.m_velocity -= impulse * invB;
}