#include "App.h"

#include <random>
#include <iostream>

int App::cachedW = 0;
int App::cachedH = 0;

App::App()
    : instancedShader(ShaderLoader::fromFiles("shaders/instanced.vert", "shaders/instanced.frag"))
    , wireShader(ShaderLoader::fromFiles("shaders/box.vert", "shaders/box.frag"))
    , instance(SPHERE_XSEGS, SPHERE_YSEGS, INSTANCE_COUNT)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    const float sphereRadius = 0.25f;
    const glm::vec3 BOX_MIN(-40.f, -20.f, -25.f);
    const glm::vec3 BOX_MAX(40.f, 20.f, 25.f);

    grid.resize(BOX_MIN, BOX_MAX, sphereRadius * 2.0f);

    glLineWidth(1.5f);

    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dx(BOX_MIN.x + sphereRadius, BOX_MAX.x - sphereRadius);
    std::uniform_real_distribution<float> dy(BOX_MIN.y + sphereRadius, BOX_MAX.y - sphereRadius);
    std::uniform_real_distribution<float> dz(BOX_MIN.z + sphereRadius, BOX_MAX.z - sphereRadius);

    spheres.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        glm::vec3 pos(dx(rng), dy(rng), dz(rng));
        spheres.emplace_back(SPHERE_XSEGS, SPHERE_YSEGS, pos, sphereRadius);
    }

    instance.updateInstances(spheres, N, 0.0f);
    instancedShader.use();
    instancedShader.setVec3("uLightDir", lightDir);
    visibleIndices.reserve(N);

    wireShader.use();
    wireShader.setVec3("uColor", glm::vec3(0.95f, 0.95f, 0.95f));

    lastFrameTime = glfwGetTime();
    titleWindowStart = lastFrameTime;
    frameCounter = 0;
}

int App::run()
{
    try
    {
        while (!window.shouldClose())
        {
            //--TITLE-INFO-UPDATE-STAGE--
            ++frameCounter;
            const double nowTitle = glfwGetTime();
            const double elapsed = nowTitle - titleWindowStart;

            if (elapsed >= 0.5)
            {
                const double fps = static_cast<double>(frameCounter) / elapsed;
                const double msPerFrame = (elapsed / static_cast<double>(frameCounter)) * 1000.0;

                std::string title = std::string("Optimization - Ball Count: ")
                    + std::to_string(N)
                    + " | Visible: " + std::to_string(lastVisibleCount)
                    + " | FPS: " + std::to_string(static_cast<int>(fps))
                    + " | Frame: " + std::to_string(msPerFrame) + " ms";

                glfwSetWindowTitle(window.handle(), title.c_str());

                titleWindowStart = nowTitle;
                frameCounter = 0;
            }
            //--TITLE-INFO-UPDATE-STAGE-END--

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

                grid.clear(N);
                for (int i = 0; i < N; ++i)
                {
                    grid.insert(i, spheres[i].getPosition(), spheres[i].getScale());
                }

                for (int iter = 0; iter < 2; ++iter)
                {
                    grid.forEachPotentialPair([&](int a, int b)
                        {
                            spheres[a].collide(spheres[b], restitutionSphere);
                        });
                }

                physicsAccumulator -= physicsDt;
                didPhysicsStep = true;
            }
            //--PHYSICS-UPDATE-STAGE-END--
#endif
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

            //--FRUSTUM-BUILD-STAGE--
            FrustumPlane frustum[6];
            extractFrustumPlanes(vp, frustum);
            //--FRUSTUM-BUILD-STAGE-END--

            //--INSTANCED-SPHERE-DRAWING-STAGE--
            instancedShader.use();
            instancedShader.setMat4("uVP", vp);
            instancedShader.setVec3("uCamPos", camera.getPosition());
            instancedShader.setFloat("uTime", static_cast<float>(now));

            visibleIndices.clear();

            for (int i = 0; i < N; ++i)
            {
                const glm::vec3 c = spheres[i].getPosition();
                const float     r = spheres[i].getScale();

                if (sphereIntersectsFrustum(frustum, c, r))
                {
                    visibleIndices.push_back(i);
                }
            }

            lastVisibleCount = (int)visibleIndices.size();

            instance.updateInstancesFiltered(spheres, visibleIndices, lastVisibleCount, static_cast<float>(now));
            instance.draw(lastVisibleCount);
            //--INSTANCED-SPHERE-DRAWING-STAGE-END--

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