#pragma once

#include <glad/glad.h>
#include <glm.hpp>

class SceneSphere;

class SceneBox
{
public:
    SceneBox(const glm::vec3& bmin, const glm::vec3& bmax);
    ~SceneBox();

    SceneBox(const SceneBox&) = delete;
    SceneBox& operator=(const SceneBox&) = delete;
    SceneBox(SceneBox&&) noexcept;
    SceneBox& operator=(SceneBox&&) noexcept;

    void draw() const;
    void resolveCollision(SceneSphere& s, float restitution) const;

private:
    GLuint vertexArray{ 0 }, vertexBuffer{ 0 }, elementBuffer{ 0 };
    glm::vec3 min{ 0.0f, 0.0f, 0.0f };
    glm::vec3 max{ 0.0f, 0.0f, 0.0f };
};