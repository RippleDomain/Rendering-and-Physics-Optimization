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
    if (m_program) glDeleteProgram(m_program);
}

ShaderLoader::ShaderLoader(ShaderLoader&& other) noexcept 
{
    m_program = other.m_program;
    other.m_program = 0;
}

ShaderLoader& ShaderLoader::operator=(ShaderLoader&& other) noexcept 
{
    if (this == &other) return *this;
    if (m_program) glDeleteProgram(m_program);

    m_program = other.m_program;
    other.m_program = 0;

    return *this;
}

ShaderLoader ShaderLoader::fromFiles(const std::string& vsPath, const std::string& fsPath) 
{
    std::string vsSrc = readTextFile(vsPath);
    std::string fsSrc = readTextFile(fsPath);

    GLuint vs = compile(GL_VERTEX_SHADER, vsSrc, "vertex");
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc, "fragment");
    GLuint prog = link(vs, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return ShaderLoader(prog);
}

GLuint ShaderLoader::compile(GLenum type, const std::string& src, const char* stageName) 
{
    GLuint s = glCreateShader(type);
    const char* csrc = src.c_str();

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

GLuint ShaderLoader::link(GLuint vs, GLuint fs) 
{
    GLuint p = glCreateProgram();

    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);

    if (!ok) 
    {
        GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0'); glGetProgramInfoLog(p, len, nullptr, log.data());

        glDeleteProgram(p);

        throw std::runtime_error(std::string("program link error:\n") + log);
    }

    return p;
}

void ShaderLoader::use() const 
{
    glUseProgram(m_program);
}

void ShaderLoader::setMat4(const char* name, const glm::mat4& m) const 
{
    GLint loc = glGetUniformLocation(m_program, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderLoader::setVec3(const char* name, const glm::vec3& v) const 
{
    GLint loc = glGetUniformLocation(m_program, name);
    glUniform3fv(loc, 1, &v.x);
}