/*
    Box header: wireframe cage draw + resolveCollision signature.
*/

#pragma once

#include <glad/glad.h>
#include <glm.hpp>

class Sphere;

//Axis-aligned bounding box used both for rendering and wall collisions.
class Box
{
public:
    Box(const glm::vec3& bMin, const glm::vec3& bMax);
    ~Box();

    Box(const Box&) = delete;
    Box& operator=(const Box&) = delete;
    Box(Box&&) noexcept;
    Box& operator=(Box&&) noexcept;

    void draw() const; //Draw wireframe box.
    void resolveCollision(Sphere& s, float restitution) const; //Clamp position and invert velocity.

private:
    GLuint vertexArray{ 0 }, vertexBuffer{ 0 }, elementBuffer{ 0 };
    glm::vec3 min{ 0.0f, 0.0f, 0.0f };
    glm::vec3 max{ 0.0f, 0.0f, 0.0f };
};