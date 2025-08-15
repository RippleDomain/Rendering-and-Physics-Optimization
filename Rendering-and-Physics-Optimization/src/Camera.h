#pragma once

#include <glm.hpp>
#include <GLFW/glfw3.h>

class Camera 
{
public:
    Camera(const glm::vec3& position = glm::vec3(0.f, 0.f, 6.f), float yawDegree = -90.f, float pitchDegree = 0.f);

    void update(GLFWwindow* window, float dt);

    glm::mat4 viewMatrix() const;

    float getFOV() const { return fov; }
    void  setFOV(float f) { fov = f; }

    const glm::vec3& getPosition() const { return position; }
    float getYaw()   const { return yaw; }
    float getPitch() const { return pitch; }

private:
    void updateVectors();

    glm::vec3 position;
    glm::vec3 front{ 0.f, 0.f, -1.f };
    glm::vec3 right{ 1.f, 0.f,  0.f };
    glm::vec3 up{ 0.f, 1.f,  0.f };
    glm::vec3 worldUp{ 0.f, 1.f, 0.f };

    float yaw;  
    float pitch;

    float moveSpeed = 3.0f;
    float sprintMultiplier = 2.5f;
    float sensitivity = 0.12f;
    float fov = 60.f;

    float mouseLastX = 0.f, mouseLastY = 0.f;
    bool  firstMouse = true;
};