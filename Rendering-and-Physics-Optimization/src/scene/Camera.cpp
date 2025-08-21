#include "Camera.h"

#include <gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(const glm::vec3& position, float yawDegree, float pitchDegree) : position(position), yaw(yawDegree), pitch(pitchDegree)
{
    updateVectors();
}

void Camera::update(GLFWwindow* window, float dt)
{
    if (!window) return;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
    {
        firstMouse = true;

        return;
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (firstMouse)
    {
        mouseLastX = static_cast<float>(x);
        mouseLastY = static_cast<float>(y);
        firstMouse = false;
    }

    float xOffset = static_cast<float>(x) - mouseLastX;
    float yOffset = mouseLastY - static_cast<float>(y);
    mouseLastX = static_cast<float>(x);
    mouseLastY = static_cast<float>(y);

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw += xOffset;
    pitch = std::clamp(pitch + yOffset, -89.0f, 89.0f);
    updateVectors();

    float speed = moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= sprintMultiplier;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += front * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= front * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * speed;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) position += worldUp * speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) position -= worldUp * speed;
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

void Camera::updateVectors()
{
    float cosYaw = glm::cos(glm::radians(yaw));
    float sinYaw = glm::sin(glm::radians(yaw));
    float cosPitch = glm::cos(glm::radians(pitch));
    float sinPitch = glm::sin(glm::radians(pitch));

    front = glm::normalize(glm::vec3(cosYaw * cosPitch, sinPitch, sinYaw * cosPitch));
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}