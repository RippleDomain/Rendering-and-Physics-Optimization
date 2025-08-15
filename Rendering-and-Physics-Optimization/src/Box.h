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
    GLuint vertexArray{ 0 }, vertexBuffer{ 0 }, elementBuffer{ 0 };
    glm::vec3 min{ 0.0f, 0.0f, 0.0f };
    glm::vec3 max{ 0.0f, 0.0f, 0.0f };
};