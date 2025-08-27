/*
    Sphere header: state accessors and simple physics methods.
*/

#pragma once

#include <glm.hpp>
#include <vector>

//--COLOR-RNG--
namespace colorRNG
{
    inline uint32_t xs32(uint32_t& s) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float u01(uint32_t v) { return (float)((v >> 8) * (1.0 / 16777215.0)); } //24-bit to [0,1)
}
//--COLOR-RNG-END--

//A simple UV-sphere that owns its GL buffers and can draw itself.
class Sphere
{
public:
    //XSegments = longitude, YSegments = latitude.
    Sphere(unsigned XSegments = 24, unsigned YSegments = 24, const glm::vec3& getPosition = glm::vec3(0.0f), float getScale = 0.25f);

    Sphere(const Sphere&) = delete;
    Sphere& operator=(const Sphere&) = delete;
    Sphere(Sphere&&) noexcept;
    Sphere& operator=(Sphere&&) noexcept;

    const glm::vec3& getPosition() const { return position; }
    float getScale() const { return scale; }
    const glm::vec3& getColor() const { return color; }
    const glm::vec3& getVelocity() const { return velocity; }
    float getMass() const { return mass; }

    void setPosition(const glm::vec3& p) { position = p; }
    void setScale(float s) { scale = s; mass = scale * scale * scale; }
    void setVelocity(const glm::vec3& v) { velocity = v; }

    void applyGravity(const glm::vec3& acceleration, float dt); //Integrate velocity/position.
    void collide(Sphere& other, float restitution); //Elastic-ish collision.

    glm::mat4 modelMatrix(float dt) const;

private:
    void build(unsigned XSegments, unsigned YSegments);

    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    float scale{ 0.25f };
    glm::vec3 color{ 1.0f, 1.0f, 1.0f }; //Assigned fast in the constructor.
    glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
    float mass{ 1.0f };
};