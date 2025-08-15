#include "Box.h"
#include "Sphere.h"

#include <cstdint>

Box::Box(const glm::vec3& bmin, const glm::vec3& bmax)
{
    m_min = bmin;
    m_max = bmax;

    const float x0 = bmin.x, y0 = bmin.y, z0 = bmin.z;
    const float x1 = bmax.x, y1 = bmax.y, z1 = bmax.z;

    const float verts[] =
    {
        x0,y0,z0,  x1,y0,z0,  x1,y1,z0,  x0,y1,z0,
        x0,y0,z1,  x1,y0,z1,  x1,y1,z1,  x0,y1,z1
    };

    const uint32_t idx[] =
    {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

    glBindVertexArray(0);
}

Box::~Box()
{
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

Box::Box(Box&& o) noexcept
{
    m_vao = o.m_vao; o.m_vao = 0;
    m_vbo = o.m_vbo; o.m_vbo = 0;
    m_ebo = o.m_ebo; o.m_ebo = 0;
    m_min = o.m_min; m_max = o.m_max;
}
Box& Box::operator=(Box&& o) noexcept
{
    if (this == &o) return *this;
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);

    m_vao = o.m_vao; o.m_vao = 0;
    m_vbo = o.m_vbo; o.m_vbo = 0;
    m_ebo = o.m_ebo; o.m_ebo = 0;
    m_min = o.m_min; m_max = o.m_max;

    return *this;
}

void Box::draw() const
{
    glBindVertexArray(m_vao);
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
        float lo = (&m_min.x)[k] + r;
        float hi = (&m_max.x)[k] - r;

        float* pp = (&p.x) + k;
        float* vp = (&v.x) + k;

        if (*pp < lo) { *pp = lo; *vp = -*vp * restitution; }
        if (*pp > hi) { *pp = hi; *vp = -*vp * restitution; }
    }

    s.setPosition(p);
    s.setVelocity(v);
}