#include "SphereInstance.h"
#include "Sphere.h"

#include <gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

SphereInstance::SphereInstance(unsigned XSegments, unsigned YSegments, int maxInstances)
{
    capacity = maxInstances;
    buildMesh(XSegments, YSegments);

    glGenBuffers(1, &instanceVertexBuffer);

    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVertexBuffer);

    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(capacity) * sizeof(float) * 20, nullptr, GL_STREAM_DRAW);
    setupInstanceAttribs();
    glBindVertexArray(0);
}

SphereInstance::~SphereInstance()
{
    if (instanceVertexBuffer) glDeleteBuffers(1, &instanceVertexBuffer);
    if (elementBuffer)  glDeleteBuffers(1, &elementBuffer);
    if (vertexBuffer)  glDeleteBuffers(1, &vertexBuffer);
    if (vertexArray)  glDeleteVertexArrays(1, &vertexArray);
}

SphereInstance::SphereInstance(SphereInstance&& o) noexcept
{
    vertexArray = o.vertexArray; o.vertexArray = 0;
    vertexBuffer = o.vertexBuffer; o.vertexBuffer = 0;
    elementBuffer = o.elementBuffer; o.elementBuffer = 0;
    instanceVertexBuffer = o.instanceVertexBuffer; o.instanceVertexBuffer = 0;
    indexCount = o.indexCount; o.indexCount = 0;
    capacity = o.capacity; o.capacity = 0;
}

SphereInstance& SphereInstance::operator=(SphereInstance&& o) noexcept
{
    if (this == &o) return *this;

    if (instanceVertexBuffer) glDeleteBuffers(1, &instanceVertexBuffer);
    if (elementBuffer)  glDeleteBuffers(1, &elementBuffer);
    if (vertexBuffer)  glDeleteBuffers(1, &vertexBuffer);
    if (vertexArray)  glDeleteVertexArrays(1, &vertexArray);

    vertexArray = o.vertexArray; o.vertexArray = 0;
    vertexBuffer = o.vertexBuffer; o.vertexBuffer = 0;
    elementBuffer = o.elementBuffer; o.elementBuffer = 0;
    instanceVertexBuffer = o.instanceVertexBuffer; o.instanceVertexBuffer = 0;
    indexCount = o.indexCount; o.indexCount = 0;
    capacity = o.capacity; o.capacity = 0;

    return *this;
}

void SphereInstance::buildMesh(unsigned XSegments, unsigned YSegments)
{
    std::vector<float> vertices;
    std::vector<unsigned> indices;

    vertices.reserve((XSegments + 1) * (YSegments + 1) * 6);
    indices.reserve(XSegments * YSegments * 6);

    for (unsigned y = 0; y <= YSegments; ++y)
    {
        for (unsigned x = 0; x <= XSegments; ++x)
        {
            float xs = static_cast<float>(x) / static_cast<float>(XSegments);
            float ys = static_cast<float>(y) / static_cast<float>(YSegments);
            float phi = xs * 6.2831853071795864769f;
            float theta = ys * 3.14159265358979323846f;

            float px = std::cos(phi) * std::sin(theta);
            float py = std::cos(theta);
            float pz = std::sin(phi) * std::sin(theta);

            //Position.
            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);
            //Normal.
            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);
        }
    }

    for (unsigned y = 0; y < YSegments; ++y)
    {
        for (unsigned x = 0; x < XSegments; ++x)
        {
            unsigned i0 = y * (XSegments + 1) + x;
            unsigned i1 = (y + 1) * (XSegments + 1) + x;
            unsigned i2 = (y + 1) * (XSegments + 1) + x + 1;
            unsigned i3 = y * (XSegments + 1) + x + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &elementBuffer);

    glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

    GLsizei stride = (3 + 3) * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void SphereInstance::setupInstanceAttribs()
{
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVertexBuffer);

    GLsizei stride = sizeof(float) * 8;

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 0));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 4));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
}

void SphereInstance::updateInstances(const std::vector<Sphere>& spheres, int count, float timeSeconds)
{
    std::vector<float> data;
    data.reserve(static_cast<size_t>(count) * 8);

    for (int i = 0; i < count; ++i)
    {
        const glm::vec3 p = spheres[i].getPosition();
        const float     s = spheres[i].getScale();
        const glm::vec3 c = spheres[i].getColor();

        float angle = timeSeconds * 0.5f;

        data.push_back(p.x);
        data.push_back(p.y);
        data.push_back(p.z);
        data.push_back(s);

        data.push_back(c.x);
        data.push_back(c.y);
        data.push_back(c.z);
        data.push_back(angle);
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVertexBuffer);

    GLsizeiptr byteSize = static_cast<GLsizeiptr>(data.size() * sizeof(float));

    // Orphan the old storage to avoid stalls
    glBufferData(GL_ARRAY_BUFFER, byteSize, nullptr, GL_STREAM_DRAW);

    // Map and write
    void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, byteSize,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    if (ptr)
    {
        std::memcpy(ptr, data.data(), static_cast<size_t>(byteSize));
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    else
    {
        // Fallback in case mapping fails (rare)
        glBufferSubData(GL_ARRAY_BUFFER, 0, byteSize, data.data());
    }
}

void SphereInstance::draw(GLsizei count) const
{
    glBindVertexArray(vertexArray);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, count);
    glBindVertexArray(0);
}