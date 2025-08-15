#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

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

    //Current framebuffer size in pixels (accounts for HiDPI).
    void getFramebufferSize(int& w, int& h) const;

    double time() const;

    //Raw GLFW window handle.
    GLFWwindow* handle() const { return m_window; }

private:
    static void errorCallback(int code, const char* desc);
    static void framebufferSizeCallback(GLFWwindow* win, int w, int h);

    void createWindow(int width, int height, const char* title, int glMajor, int glMinor, bool vsync);

    GLFWwindow* m_window{ nullptr };
    bool m_initializedGLFW{ false };
    bool m_vsync{ true };
};
