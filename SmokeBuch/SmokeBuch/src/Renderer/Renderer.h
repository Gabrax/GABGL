#pragma once
#include <string>
namespace Renderer
{
	void Render();
    
	void Load(std::string& vertexPath);
	int checkCompileErrors(unsigned int shader, std::string type);
	std::string ReadTextFromFile(std::string path);
	
}