/*
    Main application setup and frame loop: spawns spheres, runs physics, culls, and draws.
*/

#include "App.h"

#include <random>
#include <iostream>
#include <thread>
#include <atomic>
#include <memory>

int App::cachedW = 0;
int App::cachedH = 0;

App::App()
    : instancedShader(ShaderLoader::fromFiles("shaders/instanced.vert", "shaders/instanced.frag"))
    , wireShader(ShaderLoader::fromFiles("shaders/box.vert", "shaders/box.frag"))
    , instance(SPHERE_XSEGS, SPHERE_YSEGS, INSTANCE_COUNT)
{
    glEnable(GL_DEPTH_TEST);            //Depth test on for proper 3D visibility.
    glEnable(GL_CULL_FACE);             //Back-face culling to save fillrate.
    glCullFace(GL_BACK);

    glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED); //Lock cursor for camera look.

    const float sphereRadius = 0.25f;
    const glm::vec3 BOX_MIN(-40.f, -20.f, -45.f);
    const glm::vec3 BOX_MAX(40.f, 20.f, 45.f);

    grid.resize(BOX_MIN, BOX_MAX, sphereRadius * 2.0f); //Cell size is around the same as diameter for good neighborhood locality.

    glLineWidth(1.5f);

    //--STRATIFIED-SPAWN--
    spheres.reserve(N); //Reserve upfront to avoid reallocation churn.

    const int side = static_cast<int>(std::ceil(std::cbrt(static_cast<double>(N))));
    const glm::vec3 boxSize = BOX_MAX - BOX_MIN;
    const glm::vec3 cell = boxSize / static_cast<float>(side);
    const glm::vec3 clampMin = BOX_MIN + glm::vec3(sphereRadius);
    const glm::vec3 clampMax = BOX_MAX - glm::vec3(sphereRadius);

    spawnRNG frng(0xC001CAFEu);
    int placed = 0;

    for (int z = 0; z < side && placed < N; ++z)
    {
        for (int y = 0; y < side && placed < N; ++y)
        {
            for (int x = 0; x < side && placed < N; ++x)
            {
                glm::vec3 base = BOX_MIN + (glm::vec3(x, y, z) + glm::vec3(0.5f)) * cell; //Center of grid cell.
                glm::vec3 jitter = (frng.f3() - glm::vec3(0.5f)) * (cell - glm::vec3(sphereRadius * 2.0f)); //Small random offset inside cell.
                glm::vec3 pos = glm::clamp(base + jitter, clampMin, clampMax); //Clamp to avoid spawning intersecting the walls.

                spheres.emplace_back(SPHERE_XSEGS, SPHERE_YSEGS, pos, sphereRadius);
                ++placed;
            }
        }
    }
    //--STRATIFIED-SPAWN-END--

    //--LOCKS-INIT--
    sphereLocks.reset(new SphereLock[N]); //Allocate per-sphere locks once.
    //--LOCKS-INIT-END--

    //--GRID-WARMUP--
    grid.clear(N);

    for (int i = 0; i < N; ++i)
    {
        grid.insert(i, spheres[i].getPosition(), spheres[i].getScale()); //Prime broadphase grid for first frame.
    }
    //--GRID-WARMUP-END--

    instance.updateInstances(spheres, N, 0.0f); //Upload initial instance data to the GPU.

    instancedShader.use();
    instancedShader.setVec3("uLightDir", lightDir); //Static lighting direction for simple shading.
    visibleIndices.resize(N); //Pre-size visibility buffer to worst case.

    wireShader.use();
    wireShader.setVec3("uColor", glm::vec3(0.95f, 0.95f, 0.95f)); //Wireframe box color.

    lastFrameTime = glfwGetTime();

    //--GPU-JIT-WARMUP--
    {
        int w, h;
        window.getFramebufferSize(w, h);
        glViewport(0, 0, w, h);

        instancedShader.use();
        instancedShader.setMat4("uVP", glm::mat4(1.0f));
        instancedShader.setVec3("uCamPos", glm::vec3(0.0f));
        instancedShader.setFloat("uTime", 0.0f);

        instance.draw(1);   //One small draw to kick pipelines.
        glFinish();         //Ensure driver compiles/allocs before the real frame.
    }
    //--GPU-JIT-WARMUP-END--
}

int App::run()
{
    try
    {
        while (!window.shouldClose())
        {
            if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                window.requestClose();
            }

            //--CAMERA-UPDATE-STAGE--
            const double now = glfwGetTime();                                   //Frame time in seconds.
            const float dt = static_cast<float>(now - lastFrameTime);           //Delta time for this frame.
            lastFrameTime = now;
            camera.update(window.handle(), dt);                                 //Mouse + keyboard camera control.
            //--CAMERA-UPDATE-STAGE-END--

#if PHYSICS
            //--PHYSICS-UPDATE-STAGE--
            physicsAccumulator += dt;                                           //Fixed-step accumulator.
            int steps = 0;
            const int MAX_STEPS = 4;                                            //Clamp to avoid spiral-of-death under load.

            while (physicsAccumulator >= physicsDt && steps < MAX_STEPS)
            {
                //--APPLY-GRAVITY-- (parallel)
                threads.parallelFor(0, N, 2048, [&](int i0, int i1, int /*k*/)
                {
                    for (int i = i0; i < i1; ++i)
                        spheres[i].applyGravity(gravity, physicsDt);            //Simple Euler integration.
                });
                //--APPLY-GRAVITY-END--

                //--WALL-COLLISIONS-- (parallel)
                threads.parallelFor(0, N, 2048, [&](int i0, int i1, int /*k*/)
                {
                    for (int i = i0; i < i1; ++i)
                        cage.resolveCollision(spheres[i], restitutionWall);     //Cheap AABB boundary bounce.
                });
                //--WALL-COLLISIONS-END--

                //--GRID-REBUILD-- (sequential; internal datastructures not thread-safe)
                grid.clear(N);                                                          //Sparse reset (touch list) to keep O(active).
                for (int i = 0; i < N; ++i)
                {
                    grid.insert(i, spheres[i].getPosition(), spheres[i].getScale());    //Broadphase buckets.
                }
                //--GRID-REBUILD-END--

                //--SPHERE-SPHERE-COLLISIONS-- (broadphase parallel, ordered spinlocks in narrowphase)
                for (int iter = 0; iter < 2; ++iter)                                    //Two solver passes to reduce jitter.
                {
                    grid.forEachPotentialPairPrunedParallel
                    (
                        threads,
                        [&](int id) -> const glm::vec3& { return spheres[id].getPosition(); },
                        [&](int id) -> float { return spheres[id].getScale();     },
                        [&](int a, int b)
                        {
                            int i = a, j = b;
                            if (i > j) std::swap(i, j); //Order locks to avoid deadlock.

                            sphereLocks[i].lock();
                            sphereLocks[j].lock();
                            spheres[a].collide(spheres[b], restitutionSphere); //Narrow-phase resolve.
                            sphereLocks[j].unlock();
                            sphereLocks[i].unlock();
                        }
                    );
                }
                //--SPHERE-SPHERE-COLLISIONS-END--

                physicsAccumulator -= physicsDt;
                ++steps;
            }

            const double maxCarry = physicsDt * MAX_STEPS; //Cap the leftover time so we don’t accumulate too much lag.
            if (physicsAccumulator > maxCarry) physicsAccumulator = maxCarry;
            //--PHYSICS-UPDATE-STAGE-END--
#endif
            int w, h;
            window.getFramebufferSize(w, h);

            if (w != cachedW || h != cachedH)
            {
                glViewport(0, 0, w, h); //Only touch viewport when it changes.
                cachedW = w; cachedH = h;
            }

            glClearColor(0.08f, 0.10f, 0.12f, 1.0f); //Dark slate background.
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //--PROJECTION-CACHE--
            static int lastW = -1, lastH = -1;
            static float lastFov = -1.0f;
            static glm::mat4 cachedProj(1.0f);

            const float fovNow = camera.getFOV();
            if (w != lastW || h != lastH || fovNow != lastFov)
            {
                cachedProj = glm::perspective(glm::radians(fovNow), (float)w / (float)h, 0.5f, 200.0f); //Only recompute when inputs change.
                lastW = w; lastH = h; lastFov = fovNow;
            }
            const glm::mat4 proj = cachedProj; //Cheap copy from static cache.
            //--PROJECTION-CACHE-END--

            const glm::mat4 view = camera.viewMatrix();
            const glm::mat4 vp = proj * view; //Combined clip transform.

            //--FRUSTUM-BUILD-STAGE--
            FrustumPlane frustum[6];
            extractFrustumPlanes(vp, frustum); //Build 6 planes for culling.
            //--FRUSTUM-BUILD-STAGE-END--

            //--INSTANCED-SPHERE-DRAWING-STAGE--
            instancedShader.use();
            instancedShader.setMat4("uVP", vp);
            instancedShader.setVec3("uCamPos", camera.getPosition());
            instancedShader.setFloat("uTime", static_cast<float>(now));

            //--VISIBILITY-CULL--
            if ((int)visibleIndices.size() < N) visibleIndices.resize(N); //Ensure space for worst case.

            const int total = N;
            const int minGrain = 4096; //Chunk size tuned for cache and scheduling overhead.
            const int tcount = threads.getThreadCount();
            const int chunks = std::max(1, std::min(tcount, total / std::max(1, minGrain)));

            static std::vector<int> counts;
            counts.assign(chunks, 0); //Per-chunk visible counts.

            threads.parallelFor(0, total, minGrain, [&](int i0, int i1, int k)
            {
                int c = 0;

                for (int i = i0; i < i1; ++i)
                {
                    const glm::vec3 cpos = spheres[i].getPosition();
                    const float rad = spheres[i].getScale();
                    c += sphereIntersectsFrustum(frustum, cpos, rad) ? 1 : 0; //Only test sphere vs frustum (cheap).
                }

                counts[k] = c;
            });

            static std::vector<int> offsets;
            offsets.assign(chunks + 1, 0); //Exclusive prefix sum for scatter.

            for (int k = 0; k < chunks; ++k)
            {
                offsets[k + 1] = offsets[k] + counts[k];
            }

            threads.parallelFor(0, total, minGrain, [&](int i0, int i1, int k)
            {
                int out = offsets[k];

                for (int i = i0; i < i1; ++i)
                {
                    const glm::vec3 cpos = spheres[i].getPosition();
                    const float     rad = spheres[i].getScale();

                    if (sphereIntersectsFrustum(frustum, cpos, rad))
                    {
                        visibleIndices[out++] = i; //Scatter visible indices compactly.
                    }
                }
            });

            lastVisibleCount = offsets.back();      //Total visible after prefix sum.
            //--VISIBILITY-CULL-END--

            instance.updateInstancesFiltered(spheres, visibleIndices, lastVisibleCount, static_cast<float>(now)); //Upload only visible instances.
            instance.draw(lastVisibleCount);        //Instanced draw, amortizes vertex work on GPU.
            //--INSTANCED-SPHERE-DRAWING-STAGE-END--

            //--BOX-DRAWING-STAGE--
            wireShader.use();
            wireShader.setMat4("uMVP", vp);
            cage.draw(); //Outline the simulation bounds.
            //--BOX-DRAWING-STAGE-END--

            //--FPS-UPDATE-STAGE--
            {
                static double accTime = 0.0;
                static unsigned int accFrames = 0;
                static const double UPDATE_SECS = 1.0; //Update FPS once per second for stability.

                accTime += (double)dt;
                accFrames += 1;

                if (accTime >= UPDATE_SECS)
                {
                    fps = (accFrames > 0) ? (double(accFrames) / accTime) : 0.0;
                    accTime -= UPDATE_SECS;
                    accFrames = 0;
                }
            }

            {
                char line1[64], line3[64];
                std::snprintf(line1, sizeof(line1), "FPS %d", (int)std::round(fps));
                std::snprintf(line3, sizeof(line3), "VIS %d", lastVisibleCount);

                hud.draw(w, h, line1, nullptr, line3); //Minimal HUD: FPS and visible count.
            }
            //--FPS-UPDATE-STAGE-END--

            window.swapBuffers();
            window.pollEvents();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n'; //Simple error print.
        return -1;
    }

    return 0;
}