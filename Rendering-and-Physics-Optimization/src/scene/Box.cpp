#include "Box.h"
#include "Sphere.h"

#include <cstdint>

Box::Box(const glm::vec3& boxMin, const glm::vec3& boxMax)
{
    min = boxMin;
    max = boxMax;

    const float x0 = boxMin.x, y0 = boxMin.y, z0 = boxMin.z;
    const float x1 = boxMax.x, y1 = boxMax.y, z1 = boxMax.z;

    const float vertices[] =
    {
        x0,y0,z0,  x1,y0,z0,  x1,y1,z0,  x0,y1,z0,
        x0,y0,z1,  x1,y0,z1,  x1,y1,z1,  x0,y1,z1
    };

    const uint32_t indices[] =
    {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &elementBuffer);

    glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

    glBindVertexArray(0);
}

Box::~Box()
{
    if (elementBuffer) glDeleteBuffers(1, &elementBuffer);
    if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
    if (vertexArray) glDeleteVertexArrays(1, &vertexArray);
}

Box::Box(Box&& o) noexcept
{
    vertexArray = o.vertexArray;
    o.vertexArray = 0;
    vertexBuffer = o.vertexBuffer;
    o.vertexBuffer = 0;
    elementBuffer = o.elementBuffer;
    o.elementBuffer = 0;
    min = o.min; max = o.max;
}

Box& Box::operator=(Box&& o) noexcept
{
    if (this == &o) return *this;
    if (elementBuffer) glDeleteBuffers(1, &elementBuffer);
    if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
    if (vertexArray) glDeleteVertexArrays(1, &vertexArray);

    vertexArray = o.vertexArray;
    o.vertexArray = 0;
    vertexBuffer = o.vertexBuffer;
    o.vertexBuffer = 0;
    elementBuffer = o.elementBuffer;
    o.elementBuffer = 0;
    min = o.min;
    max = o.max;

    return *this;
}

void Box::draw() const
{
    glBindVertexArray(vertexArray);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Box::resolveCollision(Sphere& s, float restitution) const
{
    glm::vec3 p = s.getPosition();
    glm::vec3 v = s.getVelocity();

    float r = s.getScale();

    for (int k = 0; k < 3; ++k)
    {
        float lo = (&min.x)[k] + r;
        float hi = (&max.x)[k] - r;

        float* pp = (&p.x) + k;
        float* vp = (&v.x) + k;

        if (*pp < lo) { *pp = lo; *vp = -*vp * restitution; }
        if (*pp > hi) { *pp = hi; *vp = -*vp * restitution; }
    }

    s.setPosition(p);
    s.setVelocity(v * 1.0004f); //1.0004f multiplier in order to move the spheres around more.
}