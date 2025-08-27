/*
    Instancing header: interface for uploading/drawing many spheres efficiently.
*/

#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <vector>

class Sphere;

//Simple helper that owns a unit-sphere mesh and a per-instance buffer, and draws instanced spheres.
class Instance
{
public:
    Instance(unsigned XSegments, unsigned YSegments, int maxInstances);
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    void updateInstances(const std::vector<Sphere>& spheres, int count, float timeSeconds); //Upload all in order.
    void updateInstancesFiltered(const std::vector<Sphere>& spheres, const std::vector<int>& visible, int count, float timeSeconds); //Upload visible subset.
    void draw(GLsizei count) const; //Instanced draw call.

private:
    void buildMesh(unsigned XSegments, unsigned YSegments); //Build UV-sphere vertex/index buffers.
    void setupInstanceAttribs(); //Enable per-instance attributes.

    GLuint vertexArray{ 0 };
    GLuint vertexBuffer{ 0 };
    GLuint elementBuffer{ 0 };
    GLuint instanceVertexBuffer{ 0 };

    GLsizei indexCount{ 0 };
    int capacity{ 0 };
};