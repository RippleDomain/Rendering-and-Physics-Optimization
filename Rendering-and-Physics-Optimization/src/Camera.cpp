#include "Camera.h"
#include <gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(const glm::vec3& position, float yawDeg, float pitchDeg) : m_pos(position), m_yaw(yawDeg), m_pitch(pitchDeg) 
{
    updateVectors();
}

void Camera::update(GLFWwindow* window, float dt) 
{
    if (!window) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (m_firstMouse) 
    {
        m_lastX = (float)x;
        m_lastY = (float)y;
        m_firstMouse = false;
    }

    float xoff = (float)x - m_lastX;
    float yoff = m_lastY - (float)y;
    m_lastX = (float)x;
    m_lastY = (float)y;

    xoff *= m_sens;
    yoff *= m_sens;

    m_yaw += xoff;
    m_pitch += yoff;
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    updateVectors();

    float speed = m_moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= m_sprintMul;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_pos += m_front * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_pos -= m_front * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_pos -= m_right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_pos += m_right * speed;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)        m_pos += m_worldUp * speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) m_pos -= m_worldUp * speed;
}

glm::mat4 Camera::viewMatrix() const 
{
    return glm::lookAt(m_pos, m_pos + m_front, m_up);
}

void Camera::updateVectors() 
{
    float cy = glm::cos(glm::radians(m_yaw));
    float sy = glm::sin(glm::radians(m_yaw));
    float cp = glm::cos(glm::radians(m_pitch));
    float sp = glm::sin(glm::radians(m_pitch));

    m_front = glm::normalize(glm::vec3(cy * cp, sp, sy * cp));
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}