#include "ShaderLoader.h"

#include <gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>

static std::string readTextFile(const std::string& path) 
{
    std::ifstream ifs(path, std::ios::in);

    if (!ifs) throw std::runtime_error("Failed to open file: " + path);

    std::ostringstream oss; oss << ifs.rdbuf();

    return oss.str();
}

ShaderLoader::~ShaderLoader() 
{
    if (programID) glDeleteProgram(programID);
}

ShaderLoader::ShaderLoader(ShaderLoader&& other) noexcept 
{
    programID = other.programID;
    other.programID = 0;
}

ShaderLoader& ShaderLoader::operator=(ShaderLoader&& other) noexcept 
{
    if (this == &other) return *this;
    if (programID) glDeleteProgram(programID);

    programID = other.programID;
    other.programID = 0;

    return *this;
}

ShaderLoader ShaderLoader::fromFiles(const std::string& vsPath, const std::string& fsPath) 
{
    std::string vsSrc = readTextFile(vsPath);
    std::string fsSrc = readTextFile(fsPath);

    GLuint vertexShader = compile(GL_VERTEX_SHADER, vsSrc, "vertex");
    GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fsSrc, "fragment");
    GLuint prog = link(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return ShaderLoader(prog);
}

GLuint ShaderLoader::compile(GLenum type, const std::string& source, const char* stageName) 
{
    GLuint s = glCreateShader(type);
    const char* csrc = source.c_str();

    glShaderSource(s, 1, &csrc, nullptr);
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

GLuint ShaderLoader::link(GLuint vertexShader, GLuint fragmentShader) 
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

void ShaderLoader::use() const 
{
    glUseProgram(programID);
}

void ShaderLoader::setMat4(const char* name, const glm::mat4& m) const 
{
    GLint loc = glGetUniformLocation(programID, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderLoader::setVec3(const char* name, const glm::vec3& v) const 
{
    GLint loc = glGetUniformLocation(programID, name);
    glUniform3fv(loc, 1, &v.x);
}

void ShaderLoader::setFloat(const char* name, float v) const 
{
    GLint loc = glGetUniformLocation(programID, name);
    glUniform1f(loc, v);
}