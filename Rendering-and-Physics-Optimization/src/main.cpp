#include <gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <random>

#include "OpenGLWindow.h"
#include "Sphere.h"
#include "Box.h"
#include "ShaderLoader.h"
#include "Camera.h"

int main()
{
    try
    {
        //Create window + GL context (3.3 core) with vsync on.
        OpenGLWindow window(800, 600, "Optimization", 3, 3, true);
        glEnable(GL_DEPTH_TEST);

        glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        //Load shaders from files.
        ShaderLoader shader = ShaderLoader::fromFiles("shaders/ballShader.vert", "shaders/ballShader.frag");

        //Spawn spheres of count N.
        const int N = 5;
        std::vector<Sphere> spheres;
        spheres.reserve(N);

        //Container sizing inputs.
        int   grid = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(N))));
        float spacing = 1.2f;
        float halfOfSphere = (grid - 1) * spacing * 0.5f;

        float radius = 0.75f;
        float margin = 5.f;
        float halfOfContainer = (grid - 1) * spacing * 0.5f;
        (void)halfOfContainer;
        float B = halfOfSphere + radius + margin;

        //Random, non-overlapping spawns inside.
        std::mt19937 rng{ std::random_device{}() };
        const float sphereRadius = 0.25f;
        const float inner = B - sphereRadius;
        std::uniform_real_distribution<float> dist(-inner, inner);

        for (int i = 0; i < N; ++i)
        {
            glm::vec3 pos;
            bool ok = false;
            int attempts = 0;

            while (!ok && attempts < 512)
            {
                pos = glm::vec3(dist(rng), dist(rng), dist(rng));
                ok = true;

                for (int j = 0; j < i; ++j)
                {
                    glm::vec3 d = pos - spheres[j].getPosition();
                    float minDist = sphereRadius + spheres[j].getScale();

                    if (glm::dot(d, d) < (minDist * minDist))
                    {
                        ok = false;

                        break;
                    }
                }

                ++attempts;
            }

            spheres.emplace_back(64, 64, pos, sphereRadius);
        }

        //Create the container.
        ShaderLoader wireShader = ShaderLoader::fromFiles("shaders/box.vert", "shaders/box.frag");
        Box cage(glm::vec3(-B, -B, -B), glm::vec3(B, B, B));

        //Physics.
        glm::vec3 gravity(0.0f, -9.81f, 0.0f);
        float restitutionSphere = 0.9f;
        float restitutionWall = 0.8f;
        const float physicsDt = 1.0f / 240.0f;
        double physicsAccumulator = 0.0;

        //Camera.
        Camera camera(glm::vec3(0.0f, 0.0f, 6.0f));
        double lastFrameTime = window.time();

        //Light direction.
        glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));

        //FPS counter variables.
        double previousTime = 0.0;
        double currentTime = 0.0;
        double deltaTime = 0.0;
        unsigned int counter = 0;

        while (!window.shouldClose())
        {
            //Update FPS counter.
            currentTime = glfwGetTime();
            deltaTime = currentTime - previousTime;
            counter++;

            if (deltaTime >= 1.0 / 20)
            {
                std::string fps = std::to_string((int)(1.0 / deltaTime) * counter);
                std::string ms = std::to_string((deltaTime / counter) * 1000.0);
                std::string title = std::string("Optimization - Ball Count: ")
                    + std::to_string(N)
                    + " FPS: " + fps
                    + " - Frame time : " + ms + " ms";

                glfwSetWindowTitle(window.handle(), title.c_str());

                previousTime = currentTime;
                counter = 0;
            }

            if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                window.requestClose();
            }

            double now = window.time();
            float dt = static_cast<float>(now - lastFrameTime);
            lastFrameTime = now;

            camera.update(window.handle(), dt);

            //Physics.
            physicsAccumulator += dt;

            while (physicsAccumulator >= physicsDt)
            {
                for (int i = 0; i < N; ++i)
                {
                    spheres[i].applyGravity(gravity, physicsDt);
                }

                for (int i = 0; i < N; ++i)
                {
                    cage.resolveCollision(spheres[i], restitutionWall);
                }

                for (int iter = 0; iter < 2; ++iter)
                {
                    for (int i = 0; i < N; ++i)
                    {
                        for (int j = i + 1; j < N; ++j)
                        {
                            spheres[i].collide(spheres[j], restitutionSphere);
                        }
                    }
                }

                physicsAccumulator -= physicsDt;
            }

            int w, h;
            window.getFramebufferSize(w, h);
            glViewport(0, 0, w, h);

            glClearColor(0.08f, 0.10f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 proj = glm::perspective(glm::radians(camera.fov()), (float)w / (float)h, 0.1f, 100.0f);
            glm::mat4 view = camera.viewMatrix();

            shader.use();
            shader.setVec3("uLightDir", lightDir);
            shader.setVec3("uCamPos", camera.getPosition());

            //Draw all spheres.
            for (const auto& b : spheres)
            {
                glm::mat4 model = b.modelMatrix(static_cast<float>(now));
                glm::mat4 mvp = proj * view * model;

                shader.setMat4("uMVP", mvp);
                shader.setMat4("uModel", model);
                shader.setVec3("uBaseColor", b.getColor());

                b.draw();
            }

            //Draw the container.
            wireShader.use();
            glm::mat4 mvpBox = proj * view * glm::mat4(1.0f);
            wireShader.setMat4("uMVP", mvpBox);
            wireShader.setVec3("uColor", glm::vec3(0.95f, 0.95f, 0.95f));
            glLineWidth(1.5f);
            cage.draw();

            window.swapBuffers();
            window.pollEvents();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Fatal] " << e.what() << '\n';
        return -1;
    }

    return 0;
}