#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <vector>

class Sphere;

class Instance
{
public:
    Instance(unsigned XSegments, unsigned YSegments, int maxInstances);
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    void updateInstances(const std::vector<Sphere>& spheres, int count, float timeSeconds);
    void updateInstancesFiltered(const std::vector<Sphere>& spheres, const std::vector<int>& visible, int count, float timeSeconds);
    void draw(GLsizei count) const;

private:
    void buildMesh(unsigned XSegments, unsigned YSegments);
    void setupInstanceAttribs();

    GLuint vertexArray{ 0 };
    GLuint vertexBuffer{ 0 };
    GLuint elementBuffer{ 0 };
    GLuint instanceVertexBuffer{ 0 };

    GLsizei indexCount{ 0 };
    int capacity{ 0 };
};