#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <vector>

//A simple UV-sphere that owns its GL buffers and can draw itself.
class Sphere
{
public:
    //XSegments = longitude, YSegments = latitude
    Sphere(unsigned XSegments = 64, unsigned YSegments = 64, const glm::vec3& getPosition = glm::vec3(0.0f), float getScale = 0.25f);
    ~Sphere();

    Sphere(const Sphere&) = delete;
    Sphere& operator=(const Sphere&) = delete;
    Sphere(Sphere&&) noexcept;
    Sphere& operator=(Sphere&&) noexcept;

    void draw() const;

    const glm::vec3& getPosition() const { return position; }
    float getScale() const { return scale; }
    const glm::vec3& getColor() const { return color; }
    const glm::vec3& getVelocity() const { return velocity; }
    float getMass() const { return mass; }

    void setPosition(const glm::vec3& p) { position = p; }
    void setScale(float s) { scale = s; mass = scale * scale * scale; }
    void setVelocity(const glm::vec3& v) { velocity = v; }

    void applyGravity(const glm::vec3& acceleration, float dt);
    void collide(Sphere& other, float restitution);

    glm::mat4 modelMatrix(float dt) const;

private:
    void build(unsigned XSegments, unsigned YSegments);

    GLuint vertexArray{ 0 };
    GLuint vertexBuffer{ 0 };
    GLuint elementBuffer{ 0 };
    GLsizei indexCount{ 0 };

    glm::vec3  position{ 0.0f, 0.0f, 0.0f };
    float      scale{ 0.25f };
    glm::vec3  color{ 1.0f, 1.0f, 1.0f }; //Randomized in the constructor.
    glm::vec3  velocity{ 0.0f, 0.0f, 0.0f };
    float      mass{ 1.0f };
};