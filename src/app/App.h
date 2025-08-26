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

class App
{
public:
    App();
    int run();

private:
    OpenGLWindow window{ 1920, 1080, "Optimization", 3, 3, false };

    ShaderLoader instancedShader;
    ShaderLoader wireShader;

    std::vector<Sphere> spheres;

    Instance instance;
    std::vector<int> visibleIndices;
    int lastVisibleCount = 0;

    Box cage{ glm::vec3(-40.f, -20.f, -45.f), glm::vec3(40.f, 20.f, 45.f) };
    Camera camera{ glm::vec3(0.5f, 6.9f, 85.9f), -90.f, -6.6f };

    int N = INSTANCE_COUNT;

	HUD hud;
    double fps = 0.0;

#if PHYSICS
    glm::vec3 gravity{ 0.0f, -9.81f, 0.0f };
    float restitutionSphere = 0.9f;
    float restitutionWall = 0.8f;
    const float physicsDt = 1.0f / 240.0f;
    double physicsAccumulator = 0.0;
#endif

    glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));

    double lastFrameTime = 0.0;

    static int cachedW, cachedH;

    UniformGrid grid;
};
