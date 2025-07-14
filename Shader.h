#ifndef _SHADER_H
#define _SHADER_H

#include <glad/glad.h> // 包含glad来获取所有的必须OpenGL头文件

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

class Shader {
public:
	// program id
	GLuint shaderProgram = 0;
	std::vector<GLuint> shaders;

	// read shader from file
	void attachShader(GLenum type, const char* shaderPath);
	// use function
	void link();
	void use();
	void deleteShader();
	// uniform tools
	void setBool(const std::string& name,bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
private:
	bool checkCompileErrors(GLuint shader, std::string type);

};

#endif 