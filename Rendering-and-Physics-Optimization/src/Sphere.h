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

    const glm::vec3& getPosition() const { return m_position; }
    float getScale() const { return m_scale; }
    const glm::vec3& getColor() const { return m_color; }

    void setPosition(const glm::vec3& p) { m_position = p; }
    void setScale(float s) { m_scale = s; m_mass = m_scale * m_scale * m_scale; }

    const glm::vec3& getVelocity() const { return m_velocity; }
    void setVelocity(const glm::vec3& v) { m_velocity = v; }
    float getMass() const { return m_mass; }

    void applyGravity(const glm::vec3& acceleration, float dt);
    void collide(Sphere& other, float restitution);

    glm::mat4 modelMatrix(float timeSeconds) const;

private:
    void build(unsigned XSegments, unsigned YSegments);

    GLuint m_vao{ 0 };
    GLuint m_vbo{ 0 };
    GLuint m_ebo{ 0 };
    GLsizei m_indexCount{ 0 };

    glm::vec3  m_position{ 0.0f, 0.0f, 0.0f };
    float      m_scale{ 0.25f };
    glm::vec3  m_color{ 1.0f, 1.0f, 1.0f }; //Randomized in the constructor.

    glm::vec3  m_velocity{ 0.0f, 0.0f, 0.0f };
    float      m_mass{ 1.0f };
};