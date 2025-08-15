#pragma once
#include <glad/glad.h>
#include <glm.hpp>

class Sphere;

class Box
{
public:
    Box(const glm::vec3& bmin, const glm::vec3& bmax);
    ~Box();

    Box(const Box&) = delete;
    Box& operator=(const Box&) = delete;
    Box(Box&&) noexcept;
    Box& operator=(Box&&) noexcept;

    void draw() const;
    void resolveCollision(Sphere& s, float restitution) const;

private:
    GLuint m_vao{ 0 }, m_vbo{ 0 }, m_ebo{ 0 };
    glm::vec3 m_min{ 0.0f, 0.0f, 0.0f };
    glm::vec3 m_max{ 0.0f, 0.0f, 0.0f };
};