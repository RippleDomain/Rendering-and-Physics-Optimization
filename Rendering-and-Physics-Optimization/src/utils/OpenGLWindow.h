#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>
#include <cstdio>

/*Wrapper for a GLFW window + OpenGL context.
Creates GLFW, the window, makes the context current, loads GL (via GLAD).
Sets vsync, and installs a framebuffer resize callback (glViewport).*/
class OpenGLWindow
{
public:
    //glMajor/glMinor choose the context version. Set vsync to true to enable it.
    OpenGLWindow(int width, int height, const char* title, int glMajor = 3, int glMinor = 3, bool vsync = true);
    ~OpenGLWindow();

    OpenGLWindow(const OpenGLWindow&) = delete;
    OpenGLWindow& operator=(const OpenGLWindow&) = delete;
    OpenGLWindow(OpenGLWindow&& other) noexcept;
    OpenGLWindow& operator=(OpenGLWindow&& other) noexcept;

    bool shouldClose() const;
    void requestClose() const;
    void pollEvents() const;
    void swapBuffers() const;

    //Current framebuffer size in pixels.
    void getFramebufferSize(int& w, int& h) const;

    double time() const;

    //Raw GLFW window handle.
    GLFWwindow* handle() const { return window; }

private:
    static void errorCallback(int code, const char* desc);
    static void framebufferSizeCallback(GLFWwindow* win, int w, int h);

    void createWindow(int width, int height, const char* title, int glMajor, int glMinor, bool vsync);

    GLFWwindow* window{ nullptr };
    bool initializedGLFW{ false };
    bool vSync{ true };
};

static inline void setGLContextHints(int major, int minor)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

inline void OpenGLWindow::errorCallback(int code, const char* desc)
{
    std::fprintf(stderr, "[GLFW %d] %s\n", code, desc ? desc : "");
}

inline void OpenGLWindow::framebufferSizeCallback(GLFWwindow*, int w, int h)
{
    glViewport(0, 0, w, h);
}

inline OpenGLWindow::OpenGLWindow(int width, int height, const char* title, int glMajor, int glMinor, bool vsync)
{
    glfwSetErrorCallback(&OpenGLWindow::errorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    initializedGLFW = true;

    createWindow(width, height, title, glMajor, glMinor, vsync);
}

inline void OpenGLWindow::createWindow(int width, int height, const char* title, int glMajor, int glMinor, bool vsync)
{
    setGLContextHints(glMajor, glMinor);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!window)
    {
        throw std::runtime_error("Failed to create GLFW window.");
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD.");
    }

    vSync = vsync;
    glfwSwapInterval(vSync ? 1 : 0);

    glfwSetFramebufferSizeCallback(window, &OpenGLWindow::framebufferSizeCallback);

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);
}

inline OpenGLWindow::~OpenGLWindow()
{
    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    if (initializedGLFW)
    {
        glfwTerminate();
        initializedGLFW = false;
    }
}

inline OpenGLWindow::OpenGLWindow(OpenGLWindow&& other) noexcept
{
    window = other.window;         
    other.window = nullptr;
    initializedGLFW = other.initializedGLFW; 
    other.initializedGLFW = false;
    vSync = other.vSync;
}

inline OpenGLWindow& OpenGLWindow::operator=(OpenGLWindow&& other) noexcept
{
    if (this == &other) return *this;

    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    if (initializedGLFW)
    {
        glfwTerminate();
        initializedGLFW = false;
    }

    window = other.window;
    other.window = nullptr;
    initializedGLFW = other.initializedGLFW;
    other.initializedGLFW = false;
    vSync = other.vSync;

    return *this;
}

inline bool OpenGLWindow::shouldClose() const
{
    return glfwWindowShouldClose(window);
}

inline void OpenGLWindow::requestClose() const
{
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

inline void OpenGLWindow::pollEvents() const
{
    glfwPollEvents();
}

inline void OpenGLWindow::swapBuffers() const
{
    glfwSwapBuffers(window);
}

inline void OpenGLWindow::getFramebufferSize(int& w, int& h) const
{
    glfwGetFramebufferSize(window, &w, &h);
}

inline double OpenGLWindow::time() const
{
    return glfwGetTime();
}