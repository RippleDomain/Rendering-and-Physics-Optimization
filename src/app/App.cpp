#include "App.h"

#include <random>
#include <iostream>
#include <thread>
#include <atomic>
#include <memory>

//--TINY-FAST-RNG--
namespace
{
    struct FastRNG
    {
        uint32_t s;
        explicit FastRNG(uint32_t seed) : s(seed ? seed : 1u) {}
        inline uint32_t u32() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
        inline float f01() { return (float)((u32() >> 8) * (1.0 / 16777215.0)); }
        inline glm::vec3 f3() { return glm::vec3(f01(), f01(), f01()); }
    };
}
//--TINY-FAST-RNG-END--

//--SPINLOCK-PER-SPHERE--
struct Spin
{
    std::atomic_flag f = ATOMIC_FLAG_INIT;
    Spin() noexcept = default;
    Spin(const Spin&) = delete;
    Spin& operator=(const Spin&) = delete;

    inline void lock() { while (f.test_and_set(std::memory_order_acquire)) {} }
    inline void unlock() { f.clear(std::memory_order_release); }
};
//--SPINLOCK-PER-SPHERE-END--

//--GLOBAL-TASKS+LOCKS--
namespace
{
    ThreadSystem threads(0);
    std::unique_ptr<Spin[]> sphereLocks;
}
//--GLOBAL-TASKS+LOCKS-END--

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
    const glm::vec3 BOX_MIN(-40.f, -20.f, -45.f);
    const glm::vec3 BOX_MAX(40.f, 20.f, 45.f);

    grid.resize(BOX_MIN, BOX_MAX, sphereRadius * 2.0f);

    glLineWidth(1.5f);

    //--STRATIFIED-SPAWN--
    spheres.reserve(N);

    const int side = static_cast<int>(std::ceil(std::cbrt(static_cast<double>(N))));
    const glm::vec3 boxSize = BOX_MAX - BOX_MIN;
    const glm::vec3 cell = boxSize / static_cast<float>(side);
    const glm::vec3 clampMin = BOX_MIN + glm::vec3(sphereRadius);
    const glm::vec3 clampMax = BOX_MAX - glm::vec3(sphereRadius);

    FastRNG frng(0xC001CAFEu);
    int placed = 0;

    for (int z = 0; z < side && placed < N; ++z)
    {
        for (int y = 0; y < side && placed < N; ++y)
        {
            for (int x = 0; x < side && placed < N; ++x)
            {
                glm::vec3 base = BOX_MIN + (glm::vec3(x, y, z) + glm::vec3(0.5f)) * cell;
                glm::vec3 jitter = (frng.f3() - glm::vec3(0.5f)) * (cell - glm::vec3(sphereRadius * 2.0f));
                glm::vec3 pos = glm::clamp(base + jitter, clampMin, clampMax);

                spheres.emplace_back(SPHERE_XSEGS, SPHERE_YSEGS, pos, sphereRadius);
                ++placed;
            }
        }
    }
    //--STRATIFIED-SPAWN-END--

    //--LOCKS-INIT--
    sphereLocks.reset(new Spin[N]);
    //--LOCKS-INIT-END--

    //--GRID-WARMUP--
    grid.clear(N);
    for (int i = 0; i < N; ++i)
    {
        grid.insert(i, spheres[i].getPosition(), spheres[i].getScale());
    }
    //--GRID-WARMUP-END--

    instance.updateInstances(spheres, N, 0.0f);
    instancedShader.use();
    instancedShader.setVec3("uLightDir", lightDir);
    visibleIndices.reserve(N);

    wireShader.use();
    wireShader.setVec3("uColor", glm::vec3(0.95f, 0.95f, 0.95f));

    lastFrameTime = glfwGetTime();

    //--GPU-JIT-WARMUP-- (tiny draw so shader JIT/buffer paths settle)
    {
        int w, h;
        window.getFramebufferSize(w, h);
        glViewport(0, 0, w, h);

        instancedShader.use();
        instancedShader.setMat4("uVP", glm::mat4(1.0f));
        instancedShader.setVec3("uCamPos", glm::vec3(0.0f));
        instancedShader.setFloat("uTime", 0.0f);

        instance.draw(1);
        glFinish();
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
            const double now = glfwGetTime();
            const float dt = static_cast<float>(now - lastFrameTime);
            lastFrameTime = now;
            camera.update(window.handle(), dt);
            //--CAMERA-UPDATE-STAGE-END--

#if PHYSICS
            //--PHYSICS-UPDATE-STAGE--
            physicsAccumulator += dt;
            while (physicsAccumulator >= physicsDt)
            {
                //--APPLY-GRAVITY-- (parallel)
                threads.parallel_for(0, N, 2048, [&](int i0, int i1, int /*k*/)
                    {
                        for (int i = i0; i < i1; ++i)
                            spheres[i].applyGravity(gravity, physicsDt);
                    });
                //--APPLY-GRAVITY-END--

                //--WALL-COLLISIONS-- (parallel)
                threads.parallel_for(0, N, 2048, [&](int i0, int i1, int /*k*/)
                    {
                        for (int i = i0; i < i1; ++i)
                            cage.resolveCollision(spheres[i], restitutionWall);
                    });
                //--WALL-COLLISIONS-END--

                //--GRID-REBUILD-- (sequential; internal datastructures not thread-safe)
                grid.clear(N);
                for (int i = 0; i < N; ++i)
                    grid.insert(i, spheres[i].getPosition(), spheres[i].getScale());
                //--GRID-REBUILD-END--

                //--SPHERE-SPHERE-COLLISIONS-- (broadphase parallel, ordered spinlocks in narrowphase)
                for (int iter = 0; iter < 2; ++iter)
                {
                    grid.forEachPotentialPairParallel(threads, [&](int a, int b)
                        {
                            int i = a, j = b;
                            if (i > j) std::swap(i, j);

                            sphereLocks[i].lock();
                            sphereLocks[j].lock();
                            spheres[a].collide(spheres[b], restitutionSphere);
                            sphereLocks[j].unlock();
                            sphereLocks[i].unlock();
                        });
                }
                //--SPHERE-SPHERE-COLLISIONS-END--

                physicsAccumulator -= physicsDt;
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

            const glm::mat4 proj = glm::perspective(glm::radians(camera.getFOV()), (float)w / (float)h, 0.5f, 200.0f);
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

            //--VISIBILITY-CULL-- (parallel fill into chunked buffers, then merge)
            visibleIndices.clear();

            const int total = N;
            const int minGrain = 4096;
            const int maxChunks = std::max(1, std::min(threads.threadCount(), total / std::max(1, minGrain)));
            const int chunks = std::max(1, maxChunks);

            std::vector<std::vector<int>> visChunks;
            visChunks.resize(chunks);

            threads.parallel_for(0, total, minGrain, [&](int i0, int i1, int k)
                {
                    auto& out = visChunks[k];
                    out.reserve(i1 - i0);
                    for (int i = i0; i < i1; ++i)
                    {
                        const glm::vec3 c = spheres[i].getPosition();
                        const float     r = spheres[i].getScale();

                        if (sphereIntersectsFrustum(frustum, c, r))
                            out.push_back(i);
                    }
                });

            size_t merged = 0;
            for (const auto& v : visChunks) merged += v.size();
            visibleIndices.reserve((int)merged);
            for (const auto& v : visChunks) visibleIndices.insert(visibleIndices.end(), v.begin(), v.end());

            lastVisibleCount = (int)visibleIndices.size();

            instance.updateInstancesFiltered(spheres, visibleIndices, lastVisibleCount, static_cast<float>(now));
            instance.draw(lastVisibleCount);
            //--INSTANCED-SPHERE-DRAWING-STAGE-END--

            //--BOX-DRAWING-STAGE--
            wireShader.use();
            wireShader.setMat4("uMVP", vp);
            cage.draw();
            //--BOX-DRAWING-STAGE-END--

            //--FPS-UPDATE-STAGE--
            {
                static double accTime = 0.0;
                static unsigned int accFrames = 0;
                static const double UPDATE_SECS = 1.0;

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

                hud.draw(w, h, line1, nullptr, line3);
            }
            //--FPS-UPDATE-STAGE-END--

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