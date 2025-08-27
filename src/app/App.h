/*
    Application header: wiring for window, shaders, scene objects, and per-frame settings.
*/

#pragma once

#include "AppConfig.h"
#include "../utils/OpenGLWindow.h"
#include "../utils/ShaderLoader.h"
#include "../utils/HUD.h"
#include "../scene/Box.h"
#include "../scene/Sphere.h"
#include "../scene/Camera.h"
#include "../optimization/Instance.h"
#include "../optimization/Frustum.h"
#include "../optimization/UniformGrid.h"
#include "../optimization/ThreadSystem.h"

#include <vector>
#include <string>
#include <gtc/matrix_transform.hpp>

//--SPAWN-RNG--
namespace
{
    struct spawnRNG
    {
        uint32_t s;
        explicit spawnRNG(uint32_t seed) : s(seed ? seed : 1u) {}
        inline uint32_t u32() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
        inline float f01() { return (float)((u32() >> 8) * (1.0 / 16777215.0)); }
        inline glm::vec3 f3() { return glm::vec3(f01(), f01(), f01()); }
    };
}
//--SPAWN-RNG-END--

//--SPHERE-LOCKS--
struct SphereLock
{
    std::atomic_flag f = ATOMIC_FLAG_INIT;
    SphereLock() noexcept = default;
    SphereLock(const SphereLock&) = delete;
    SphereLock& operator=(const SphereLock&) = delete;

    inline void lock() { while (f.test_and_set(std::memory_order_acquire)) {} }
    inline void unlock() { f.clear(std::memory_order_release); }
};
//--SPHERE-LOCKS-END--

//--THREADS--
namespace
{
    ThreadSystem threads(0);                        //Thread pool used for CPU-parallel stages.
    std::unique_ptr<SphereLock[]> sphereLocks;      //One spinlock per sphere for ordered narrow-phase locking.
}
//--THREADS-END--

class App
{
public:
    App();
    int run(); //Main loop.

private:
    OpenGLWindow window{ 1920, 1080, "Optimization", 3, 3, false }; //GL context + swap control.

    ShaderLoader instancedShader;       //Shader for instanced spheres.
    ShaderLoader wireShader;            //Shader for the wireframe box.

    std::vector<Sphere> spheres;        //All simulated spheres.

    Instance instance;                  //GPU-side instancing helper.
    std::vector<int> visibleIndices;    //Compact list of visible sphere indices.
    int lastVisibleCount = 0;           //Visible count from last cull.

    Box cage{ glm::vec3(-40.f, -20.f, -45.f), glm::vec3(40.f, 20.f, 45.f) }; //World bounds.
    Camera camera{ glm::vec3(0.5f, 6.9f, 85.9f), -90.f, -6.6f };             //Free-fly camera.

    int N = INSTANCE_COUNT;       //Target instance count.

    HUD hud;                      //Tiny HUD for FPS/visible count.
    double fps = 0.0;             //Averaged FPS (1s window).

#if PHYSICS
    glm::vec3 gravity{ 0.0f, -9.81f, 0.0f }; //Constant gravity.
    float restitutionSphere = 0.9f;          //Bounciness for sphere-sphere collisions.
    float restitutionWall = 0.8f;            //Bounciness for wall-sphere collisions.
    const float physicsDt = 1.0f / 240.0f;   //Fixed step time.
    double physicsAccumulator = 0.0;         //Accumulator for fixed stepping.
#endif

    glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)); //Directional light.

    double lastFrameTime = 0.0;   //For dt computation.

    static int cachedW, cachedH;  //Cached viewport to avoid redundant glViewport.

    UniformGrid grid;             //Broadphase (bucket grid) for potential pairs.
};