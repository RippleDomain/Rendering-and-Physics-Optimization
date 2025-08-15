#pragma once

#include <glm.hpp>
#include <GLFW/glfw3.h>

class Camera 
{
public:
    Camera(const glm::vec3& position = glm::vec3(0.f, 0.f, 6.f), float yawDeg = -90.f, float pitchDeg = 0.f);

    void update(GLFWwindow* window, float dtSeconds);

    glm::mat4 viewMatrix() const;

    float fov() const { return m_fov; }
    void  setFov(float f) { m_fov = f; }

    const glm::vec3& getPosition() const { return m_pos; }
    float getYaw()   const { return m_yaw; }
    float getPitch() const { return m_pitch; }

private:
    void updateVectors();

    glm::vec3 m_pos;
    glm::vec3 m_front{ 0.f, 0.f, -1.f };
    glm::vec3 m_right{ 1.f, 0.f,  0.f };
    glm::vec3 m_up{ 0.f, 1.f,  0.f };
    glm::vec3 m_worldUp{ 0.f, 1.f, 0.f };

    float m_yaw;  
    float m_pitch;

    float m_moveSpeed = 3.0f;
    float m_sprintMul = 2.5f;
    float m_sens = 0.12f;
    float m_fov = 60.f;

    float m_lastX = 0.f, m_lastY = 0.f;
    bool  m_firstMouse = true;
};