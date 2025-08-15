#include <glad/glad.h>  
#include "OpenGLWindow.h"
#include <stdexcept>
#include <string>

static void setGLContextHints(int major, int minor) 
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void OpenGLWindow::errorCallback(int code, const char* desc) 
{
    fprintf(stderr, "[GLFW %d] %s\n", code, desc ? desc : "");
}

void OpenGLWindow::framebufferSizeCallback(GLFWwindow*, int w, int h) 
{
    glViewport(0, 0, w, h);
}

OpenGLWindow::OpenGLWindow(int width, int height, const char* title, int glMajor, int glMinor, bool vsync) 
{
    glfwSetErrorCallback(&OpenGLWindow::errorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    m_initializedGLFW = true;

    createWindow(width, height, title, glMajor, glMinor, vsync);
}

void OpenGLWindow::createWindow(int width, int height, const char* title, int glMajor, int glMinor, bool vsync) 
{
    setGLContextHints(glMajor, glMinor);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!m_window) 
    {
        throw std::runtime_error("Failed to create GLFW window.");
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        throw std::runtime_error("Failed to initialize GLAD.");
    }

    m_vsync = vsync;
    glfwSwapInterval(m_vsync ? 1 : 0);

    glfwSetFramebufferSizeCallback(m_window, &OpenGLWindow::framebufferSizeCallback);

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(m_window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);
}

OpenGLWindow::~OpenGLWindow() 
{
    if (m_window) 
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_initializedGLFW) 
    {
        glfwTerminate();
        m_initializedGLFW = false;
    }
}

OpenGLWindow::OpenGLWindow(OpenGLWindow&& other) noexcept 
{
    m_window = other.m_window;         other.m_window = nullptr;
    m_initializedGLFW = other.m_initializedGLFW; other.m_initializedGLFW = false;
    m_vsync = other.m_vsync;
}

OpenGLWindow& OpenGLWindow::operator=(OpenGLWindow&& other) noexcept 
{
    if (this == &other) return *this;

    if (m_window) 
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_initializedGLFW) 
    {
        glfwTerminate();
        m_initializedGLFW = false;
    }

    m_window = other.m_window;
    other.m_window = nullptr;
    m_initializedGLFW = other.m_initializedGLFW;
    other.m_initializedGLFW = false;
    m_vsync = other.m_vsync;

    return *this;
}

bool OpenGLWindow::shouldClose() const 
{
    return glfwWindowShouldClose(m_window);
}

void OpenGLWindow::requestClose() const 
{
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void OpenGLWindow::pollEvents() const 
{
    glfwPollEvents();
}

void OpenGLWindow::swapBuffers() const 
{
    glfwSwapBuffers(m_window);
}

void OpenGLWindow::getFramebufferSize(int& w, int& h) const 
{
    glfwGetFramebufferSize(m_window, &w, &h);
}

double OpenGLWindow::time() const 
{
    return glfwGetTime();
}
