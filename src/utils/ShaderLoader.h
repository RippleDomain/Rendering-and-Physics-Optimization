/*
    Shader utility: load GLSL from files, compile/link, and set common uniforms.
*/

#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

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

    void setMat4(const char* name, const glm::mat4& mutex) const;
    void setVec2(const char* name, const glm::vec2& v) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setVec4(const char* name, const glm::vec4& v) const;
    void setFloat(const char* name, float value) const;

private:
    explicit ShaderLoader(GLuint program) : programID(program) {}
    static GLuint compile(GLenum type, const std::string& source, const char* stageName);
    static GLuint link(GLuint vertexShader, GLuint fragmentShader);

    GLuint programID{ 0 };
};

//--FILE-READ-HELPER--
static inline std::string readTextFile(const std::string& path)
{
    std::ifstream stream(path, std::ios::in);
    if (!stream) throw std::runtime_error("Failed to open file: " + path);
    std::ostringstream oss; oss << stream.rdbuf();

    return oss.str();
}
//--FILE-READ-HELPER-END--

inline ShaderLoader::~ShaderLoader()
{
    if (programID) glDeleteProgram(programID);
}

inline ShaderLoader::ShaderLoader(ShaderLoader&& other) noexcept
{
    programID = other.programID;
    other.programID = 0;
}

inline ShaderLoader& ShaderLoader::operator=(ShaderLoader&& other) noexcept
{
    if (this == &other) return *this;
    if (programID) glDeleteProgram(programID);

    programID = other.programID;
    other.programID = 0;

    return *this;
}

inline ShaderLoader ShaderLoader::fromFiles(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
{
    std::string vertexShaderSource = readTextFile(vertexShaderPath);
    std::string fragmentShaderSource = readTextFile(fragmentShaderPath);

    GLuint vertexShader = compile(GL_VERTEX_SHADER, vertexShaderSource, "vertex");
    GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentShaderSource, "fragment");
    GLuint program = link(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return ShaderLoader(program);
}

//--SHADER-COMPILE--
inline GLuint ShaderLoader::compile(GLenum type, const std::string& source, const char* stageName)
{
    GLuint s = glCreateShader(type);
    const char* cSrc = source.c_str();

    glShaderSource(s, 1, &cSrc, nullptr);
    glCompileShader(s);

    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);

    if (!ok)
    {
        GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0'); glGetShaderInfoLog(s, len, nullptr, log.data());

        glDeleteShader(s);

        throw std::runtime_error(std::string(stageName) + " compile error:\n" + log);
    }

    return s;
}
//--SHADER-COMPILE-END--

//--PROGRAM-LINK--
inline GLuint ShaderLoader::link(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint p = glCreateProgram();

    glAttachShader(p, vertexShader);
    glAttachShader(p, fragmentShader);
    glLinkProgram(p);

    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);

    if (!ok)
    {
        GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0'); glGetProgramInfoLog(p, len, nullptr, log.data());

        glDeleteProgram(p);

        throw std::runtime_error(std::string("programID link error:\n") + log);
    }

    return p;
}
//--PROGRAM-LINK-END--

inline void ShaderLoader::use() const
{
    glUseProgram(programID);
}

inline void ShaderLoader::setMat4(const char* name, const glm::mat4& mutex) const
{
    GLint uniformLocation = glGetUniformLocation(programID, name);
    glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(mutex));
}

inline void ShaderLoader::setVec2(const char* name, const glm::vec2& v) const
{
    GLint uniformLocation = glGetUniformLocation(programID, name);
    glUniform2fv(uniformLocation, 1, &v.x);
}

inline void ShaderLoader::setVec3(const char* name, const glm::vec3& v) const
{
    GLint uniformLocation = glGetUniformLocation(programID, name);
    glUniform3fv(uniformLocation, 1, &v.x);
}

inline void ShaderLoader::setVec4(const char* name, const glm::vec4& v) const
{
    GLint uniformLocation = glGetUniformLocation(programID, name);
    glUniform4fv(uniformLocation, 1, &v.x);
}

inline void ShaderLoader::setFloat(const char* name, float v) const
{
    GLint uniformLocation = glGetUniformLocation(programID, name);
    glUniform1f(uniformLocation, v);
}