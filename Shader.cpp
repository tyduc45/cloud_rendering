#include "Shader.h"

void Shader::attachShader(GLenum type, const char* shaderPath)
{
    std::ifstream shaderStream;
    std::stringstream shaderFile;
    std::string shaderString;
    shaderStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try 
    {
        shaderStream.open(shaderPath);
        shaderFile << shaderStream.rdbuf();
        shaderStream.close();
        shaderString = shaderFile.str();
    }
    catch(const std::ifstream::failure& e)
    {
        std::cout << "SHADER::STREAM::CANNOT::READ::FILE" << shaderPath << "\n";
    }

    const char* shaderCode = shaderString.c_str();

    GLuint src;
    src = glCreateShader(type);
    glShaderSource(src, 1, &shaderCode, NULL);
    glCompileShader(src);
    if (!checkCompileErrors(src, "SHADER"))
    {
        return;
    }
    shaders.push_back(src);
}
void Shader::link()
{
    shaderProgram = glCreateProgram();
    for (int i = 0; i < shaders.size(); i++) {
        glAttachShader(shaderProgram, shaders[i]);
    }
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");
}

void Shader::deleteShader()
{
    for (int i = 0; i < shaders.size(); i++) {
        glDeleteShader(shaders[i]);
    }
    shaders.clear();
}

void Shader::use()
{
	glUseProgram(shaderProgram);
}

void Shader::setBool(const std::string& name, bool value) const
{
	glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
}
void Shader::setInt(const std::string& name, int value) const
{
	glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
}
void Shader::setFloat(const std::string& name, float value) const
{
	glUniform1f(glGetUniformLocation(shaderProgram, name.c_str()), value);
}


bool Shader::checkCompileErrors(GLuint shader, std::string type)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            return false;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            return false;
        }
    }
    return true;
}