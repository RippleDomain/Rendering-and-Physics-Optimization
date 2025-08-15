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
    GLuint getID() const { return programID; }

    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec3(const char* name, const glm::vec3& v) const;
	void setFloat(const char* name, float value) const;

private:
    explicit ShaderLoader(GLuint program) : programID(program) {}
    static GLuint compile(GLenum type, const std::string& source, const char* stageName);
    static GLuint link(GLuint vertexShader, GLuint fragmentShader);

    GLuint programID{ 0 };
};
