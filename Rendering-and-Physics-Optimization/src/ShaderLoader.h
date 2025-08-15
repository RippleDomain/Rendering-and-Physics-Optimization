#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <string>

class ShaderLoader 
{
public:
    ShaderLoader() = default;
    ~ShaderLoader();

    ShaderLoader(const ShaderLoader&) = delete;
    ShaderLoader& operator=(const ShaderLoader&) = delete;
    ShaderLoader(ShaderLoader&& other) noexcept;
    ShaderLoader& operator=(ShaderLoader&& other) noexcept;

    static ShaderLoader fromFiles(const std::string& vsPath, const std::string& fsPath);

    void use() const;
    GLuint id() const { return m_program; }

    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec3(const char* name, const glm::vec3& v) const;

private:
    explicit ShaderLoader(GLuint program) : m_program(program) {}
    static GLuint compile(GLenum type, const std::string& src, const char* stageName);
    static GLuint link(GLuint vs, GLuint fs);

    GLuint m_program{ 0 };
};
