#include "SphereInstance.h"
#include "SceneSphere.h"

#include <gtc/type_ptr.hpp>
#include <gtc/packing.hpp>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>

//--INSTANCE-DATA-PACKED--
namespace
{
    //24 bytes per instance, tightly packed.
    struct InstanceDataPacked
    {
        glm::vec3 pos;          //12
        std::uint16_t scale;    //2 (half)
        std::uint8_t  color[4]; //4 (UNORM8 RGBA, A=255)
        std::uint16_t angle;    //2 (half)
        std::uint16_t pad = 0;  //2
    };
}
//--INSTANCE-DATA-PACKED-END--

SphereInstance::SphereInstance(unsigned XSegments, unsigned YSegments, int maxInstances)
{
    capacity = maxInstances;
    buildMesh(XSegments, YSegments);

    glGenBuffers(1, &instanceVertexBuffer);

    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVertexBuffer);

    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(capacity) * static_cast<GLsizeiptr>(sizeof(InstanceDataPacked)),
        nullptr, GL_STREAM_DRAW);

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

void SphereInstance::buildMesh(unsigned XSegments, unsigned YSegments)
{
    struct VtxPN {
        float px, py, pz;
        std::uint32_t n;
    };

    std::vector<VtxPN> vertices;
    std::vector<std::uint16_t> indices;

    vertices.reserve((XSegments + 1) * (YSegments + 1));
    indices.reserve(XSegments * YSegments * 6);

    for (unsigned y = 0; y <= YSegments; ++y)
    {
        for (unsigned x = 0; x <= XSegments; ++x)
        {
            float xs = float(x) / float(XSegments);
            float ys = float(y) / float(YSegments);
            float phi = xs * 6.2831853071795864769f;
            float theta = ys * 3.14159265358979323846f;

            float px = std::cos(phi) * std::sin(theta);
            float py = std::cos(theta);
            float pz = std::sin(phi) * std::sin(theta);

            glm::vec4 n(px, py, pz, 0.0f);
            std::uint32_t nPacked = glm::packSnorm3x10_1x2(n);

            vertices.push_back({ px, py, pz, nPacked });
        }
    }

    for (unsigned y = 0; y < YSegments; ++y)
    {
        for (unsigned x = 0; x < XSegments; ++x)
        {
            std::uint16_t i0 = static_cast<std::uint16_t>(y * (XSegments + 1) + x);
            std::uint16_t i1 = static_cast<std::uint16_t>((y + 1) * (XSegments + 1) + x);
            std::uint16_t i2 = static_cast<std::uint16_t>((y + 1) * (XSegments + 1) + x + 1);
            std::uint16_t i3 = static_cast<std::uint16_t>(y * (XSegments + 1) + x + 1);

            indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i3);
        }
    }
    indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &elementBuffer);

    glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VtxPN), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(std::uint16_t), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VtxPN), (void*)offsetof(VtxPN, px));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(VtxPN), (void*)offsetof(VtxPN, n));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void SphereInstance::setupInstanceAttribs()
{
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVertexBuffer);

    const GLsizei stride = static_cast<GLsizei>(sizeof(InstanceDataPacked));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(InstanceDataPacked, pos));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_HALF_FLOAT, GL_FALSE, stride, (void*)offsetof(InstanceDataPacked, scale));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(InstanceDataPacked, color));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_HALF_FLOAT, GL_FALSE, stride, (void*)offsetof(InstanceDataPacked, angle));
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(0);
}

void SphereInstance::updateInstances(const std::vector<SceneSphere>& spheres, int count, float timeSeconds)
{
    const GLsizeiptr byteSize = static_cast<GLsizeiptr>(count) * static_cast<GLsizeiptr>(sizeof(InstanceDataPacked));
    glBindBuffer(GL_ARRAY_BUFFER, instanceVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, byteSize, nullptr, GL_STREAM_DRAW);

    auto* dst = static_cast<InstanceDataPacked*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, byteSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    if (dst)
    {
        for (int i = 0; i < count; ++i)
        {
            const glm::vec3 p = spheres[i].getPosition();
            const float     s = spheres[i].getScale();
            const glm::vec3 c = spheres[i].getColor();

            dst[i].pos = p;
            dst[i].scale = glm::packHalf1x16(s);

            dst[i].color[0] = static_cast<std::uint8_t>(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f + 0.5f);
            dst[i].color[1] = static_cast<std::uint8_t>(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f + 0.5f);
            dst[i].color[2] = static_cast<std::uint8_t>(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f + 0.5f);
            dst[i].color[3] = 255;

            const float angle = timeSeconds * 0.5f;

            dst[i].angle = glm::packHalf1x16(angle);
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    else
    {
        std::vector<InstanceDataPacked> scratch(static_cast<size_t>(count));

        for (int i = 0; i < count; ++i)
        {
            scratch[i].pos = spheres[i].getPosition();
            scratch[i].scale = glm::packHalf1x16(spheres[i].getScale());
            const glm::vec3 c = spheres[i].getColor();
            scratch[i].color[0] = static_cast<std::uint8_t>(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f + 0.5f);
            scratch[i].color[1] = static_cast<std::uint8_t>(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f + 0.5f);
            scratch[i].color[2] = static_cast<std::uint8_t>(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f + 0.5f);
            scratch[i].color[3] = 255;
            scratch[i].angle = glm::packHalf1x16(timeSeconds * 0.5f);
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, byteSize, scratch.data());
    }
}

void SphereInstance::updateInstancesFiltered(const std::vector<SceneSphere>& spheres, const std::vector<int>& visible, int count, float timeSeconds)
{
    const int c = std::min<int>(count, (int)visible.size());
    const GLsizeiptr byteSize = static_cast<GLsizeiptr>(c) * static_cast<GLsizeiptr>(sizeof(InstanceDataPacked));

    glBindBuffer(GL_ARRAY_BUFFER, instanceVertexBuffer);

    auto* dst = static_cast<InstanceDataPacked*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, byteSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    if (dst)
    {
        for (int k = 0; k < c; ++k)
        {
            const SceneSphere& s = spheres[visible[k]];
            const glm::vec3 p = s.getPosition();
            const float     sc = s.getScale();
            const glm::vec3 col = s.getColor();

            dst[k].pos = p;
            dst[k].scale = glm::packHalf1x16(sc);
            dst[k].color[0] = static_cast<std::uint8_t>(glm::clamp(col.r, 0.0f, 1.0f) * 255.0f + 0.5f);
            dst[k].color[1] = static_cast<std::uint8_t>(glm::clamp(col.g, 0.0f, 1.0f) * 255.0f + 0.5f);
            dst[k].color[2] = static_cast<std::uint8_t>(glm::clamp(col.b, 0.0f, 1.0f) * 255.0f + 0.5f);
            dst[k].color[3] = 255;

            const float angle = timeSeconds * 0.5f;
            dst[k].angle = glm::packHalf1x16(angle);
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    else
    {
        std::vector<InstanceDataPacked> scratch(static_cast<size_t>(c));

        for (int k = 0; k < c; ++k)
        {
            const SceneSphere& s = spheres[visible[k]];
            scratch[k].pos = s.getPosition();
            scratch[k].scale = glm::packHalf1x16(s.getScale());
            const glm::vec3 col = s.getColor();
            scratch[k].color[0] = static_cast<std::uint8_t>(glm::clamp(col.r, 0.0f, 1.0f) * 255.0f + 0.5f);
            scratch[k].color[1] = static_cast<std::uint8_t>(glm::clamp(col.g, 0.0f, 1.0f) * 255.0f + 0.5f);
            scratch[k].color[2] = static_cast<std::uint8_t>(glm::clamp(col.b, 0.0f, 1.0f) * 255.0f + 0.5f);
            scratch[k].color[3] = 255;
            scratch[k].angle = glm::packHalf1x16(timeSeconds * 0.5f);
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, byteSize, scratch.data());
    }
}

void SphereInstance::draw(GLsizei count) const
{
    glBindVertexArray(vertexArray);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0, count);
    glBindVertexArray(0);
}