#pragma once
#include <glad/glad.h> 

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>




namespace Shader
{
	inline unsigned int ID;
	void CreateShader(const char* vertexPath, const char* fragmentPath);
	void Use();
	void checkCompileErrors(unsigned int shader, std::string type);
	void setBool(const std::string& name, bool value);
	void setInt(const std::string& name, int value);
	void setFloat(const std::string& name, float value);
}

