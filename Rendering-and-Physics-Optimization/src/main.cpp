#define INSTANCING 0
#define PHYSICS 0

#include <gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <random>

#include "OpenGLWindow.h"
#include "Sphere.h"

#if INSTANCING
#include "SphereInstance.h"
#endif

#include "Box.h"
#include "ShaderLoader.h"
#include "Camera.h"

//--TUNABLES--
static constexpr int SPHERE_XSEGS = 24;
static constexpr int SPHERE_YSEGS = 24;
static constexpr int INSTANCE_COUNT = 10000;
static constexpr bool SHOW_FPS = true;
//--TUNABLES-END--

int main()
{
    try
    {
        //Create window + GL context (3.3 core) with vsync on.
        OpenGLWindow window(1920, 1080, "Optimization", 3, 3, false);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        //--SHADERS-SETUP--
#if INSTANCING
        ShaderLoader instancedShader = ShaderLoader::fromFiles("shaders/instanced.vert", "shaders/instanced.frag");
#else
        ShaderLoader shader = ShaderLoader::fromFiles("shaders/ballShader.vert", "shaders/ballShader.frag");
#endif
        ShaderLoader wireShader = ShaderLoader::fromFiles("shaders/box.vert", "shaders/box.frag");
        //--SHADERS-SETUP-END--

        //Spawn spheres of count N.
        const int N = INSTANCE_COUNT;
        std::vector<Sphere> spheres;
        spheres.reserve(N);

#if INSTANCING
        SphereInstance instance(SPHERE_XSEGS, SPHERE_YSEGS, N);
#endif

        //--BOX-SETUP--
        const glm::vec3 BOX_MIN(-40.f, -10.f, -10.f);
        const glm::vec3 BOX_MAX(40.f, 10.f, 10.f);
        Box cage(BOX_MIN, BOX_MAX);
        glLineWidth(1.5f);
        //--BOX-SETUP-END--

        //Random spawn fully inside the fixed size box.
        const float sphereRadius = 0.25f;
        std::mt19937 rng{ std::random_device{}() };
        std::uniform_real_distribution<float> dx(BOX_MIN.x + sphereRadius, BOX_MAX.x - sphereRadius);
        std::uniform_real_distribution<float> dy(BOX_MIN.y + sphereRadius, BOX_MAX.y - sphereRadius);
        std::uniform_real_distribution<float> dz(BOX_MIN.z + sphereRadius, BOX_MAX.z - sphereRadius);

        for (int i = 0; i < N; ++i)
        {
            glm::vec3 pos(dx(rng), dy(rng), dz(rng));
            spheres.emplace_back(SPHERE_XSEGS, SPHERE_YSEGS, pos, sphereRadius);
        }

#if INSTANCING
        //Static instance upload.
        instance.updateInstances(spheres, N, 0.0f);
#endif

#if PHYSICS
        //--PHYSICS-SETUP--
        glm::vec3 gravity(0.0f, -9.81f, 0.0f);
        float restitutionSphere = 0.9f;
        float restitutionWall = 0.8f;
        const float physicsDt = 1.0f / 240.0f;
        double physicsAccumulator = 0.0;
        //--PHYSICS-SETUP-END--
#endif

        //--CAMERA-LIGHT-SETUP--
        Camera camera(glm::vec3(0.873736382f, 5.155540886f, 50.8167648f));
        double lastFrameTime = glfwGetTime();
        glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));

#if INSTANCING
        instancedShader.use();
        instancedShader.setVec3("uLightDir", lightDir);
#endif

        wireShader.use();
        wireShader.setVec3("uColor", glm::vec3(0.95f, 0.95f, 0.95f));
        //--CAMERA-LIGHT-SETUP-END--

        //--FPS-COUNTER-SETUP--
        double titleWindowStart = glfwGetTime(); 
        unsigned int frameCounter = 0;
        //--FPS-COUNTER-SETUP-END--

        //Cache viewport, only update on size change.
        static int cachedW = 0, cachedH = 0;

        while (!window.shouldClose())
        {
            //--FPS-COUNTER-STAGE--
            if (SHOW_FPS)
            {
                ++frameCounter;
                const double nowTitle = glfwGetTime();
                const double elapsed = nowTitle - titleWindowStart;

                if (elapsed >= 0.5)
                {
                    const double fps = static_cast<double>(frameCounter) / elapsed;
                    const double msPerFrame = (elapsed / static_cast<double>(frameCounter)) * 1000.0;

                    std::string title = std::string("Optimization - Ball Count: ")
                        + std::to_string(N)
                        + " | FPS: " + std::to_string(static_cast<int>(fps))
                        + " | Frame: " + std::to_string(msPerFrame) + " ms";

                    glfwSetWindowTitle(window.handle(), title.c_str());

                    titleWindowStart = nowTitle;
                    frameCounter = 0;
                }
            }
            //--FPS-COUNTER-STAGE-END--

            //Window input handling.
            if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                window.requestClose();
            }

            //--CAMERA-UPDATE-STAGE--
            const double now = glfwGetTime();
            const float dt = static_cast<float>(now - lastFrameTime);
            lastFrameTime = now;
            camera.update(window.handle(), dt);
            //--CAMERA-UPDATE-STAGE-END--

#if PHYSICS
            //--PHYSICS-UPDATE-STAGE--
            bool didPhysicsStep = false;

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
                didPhysicsStep = true;
            }
            //--PHYSICS-UPDATE-STAGE-END--

#if INSTANCING
            //If transforms changed this frame, refresh GPU instance data.
            if (didPhysicsStep)
            {
                instance.updateInstances(spheres, N, static_cast<float>(now));
            }
#endif //INSTANCING
#endif //PHYSICS

            int w, h;
            window.getFramebufferSize(w, h);
            if (w != cachedW || h != cachedH)
            {
                glViewport(0, 0, w, h);
                cachedW = w; cachedH = h;
            }

            glClearColor(0.08f, 0.10f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const glm::mat4 proj = glm::perspective(glm::radians(camera.getFOV()), (float)w / (float)h, 0.1f, 100.0f);
            const glm::mat4 view = camera.viewMatrix();
            const glm::mat4 vp = proj * view;

#if !INSTANCING
            //--SPHERE-DRAWING-STAGE-(non-instanced)--
            shader.use();
            shader.setVec3("uLightDir", lightDir);
            shader.setVec3("uCamPos", camera.getPosition());

            for (const auto& b : spheres)
            {
                glm::mat4 model = b.modelMatrix(static_cast<float>(now));
                glm::mat4 mvp = vp * model;

                shader.setMat4("uMVP", mvp);
                shader.setMat4("uModel", model);
                shader.setVec3("uBaseColor", b.getColor());

                b.draw();
            }
            //--SPHERE-DRAWING-STAGE-END--
#else
            //--INSTANCED-SPHERE-DRAWING-STAGE--
            instancedShader.use();
            instancedShader.setMat4("uVP", vp);
            instancedShader.setVec3("uCamPos", camera.getPosition());

            instancedShader.setFloat("uTime", static_cast<float>(now));

            //Instance data uploaded once at initialization. No per-frame update needed when static.
            //If physics turned on, call instance.updateInstances() only when transforms change.
            instance.draw(N);
            //--INSTANCED-SPHERE-DRAWING-STAGE-END--
#endif

            //--BOX-DRAWING-STAGE--
            wireShader.use();
            wireShader.setMat4("uMVP", vp);
            cage.draw();
            //--BOX-DRAWING-STAGE-END--

            window.swapBuffers();
            window.pollEvents();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return -1;
    }

    return 0;
}