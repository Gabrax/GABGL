#pragma once
#include "glad/glad.h"
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

struct Shader {
    
    void Load(const char* fullshader)
    {
       // Read the entire shader file
      std::ifstream shaderFile(fullshader);
      shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      std::string fileContent;
      try {
          std::stringstream shaderStream;
          shaderStream << shaderFile.rdbuf();
          fileContent = shaderStream.str();
      } catch (std::ifstream::failure& e) {
          std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
          return;
      }

      // Parse the shader file into sections based on #type
      std::unordered_map<std::string, std::string> shaderSources;
      const std::string typeToken = "#type";
      size_t pos = 0;
      while ((pos = fileContent.find(typeToken, pos)) != std::string::npos) {
          size_t endOfLine = fileContent.find('\n', pos);
          std::string type = fileContent.substr(pos + typeToken.length(), endOfLine - pos - typeToken.length());
          type = type.substr(type.find_first_not_of(" \t\r\n")); // Trim leading whitespace
          type = type.substr(0, type.find_last_not_of(" \t\r\n") + 1); // Trim trailing whitespace

          size_t nextTypePos = fileContent.find(typeToken, endOfLine + 1);
          if (nextTypePos == std::string::npos)
              nextTypePos = fileContent.size();

          std::string source = fileContent.substr(endOfLine + 1, nextTypePos - endOfLine - 1);
          shaderSources[type] = source;

          pos = nextTypePos;
      }

      // Compile each shader
      GLuint vertex = 0, fragment = 0, geometry = 0, tessControl = 0, tessEvaluation = 0, compute = 0;

      if (shaderSources.find("VERTEX") != shaderSources.end()) {
          const char* vertexCode = shaderSources["VERTEX"].c_str();
          vertex = glCreateShader(GL_VERTEX_SHADER);
          glShaderSource(vertex, 1, &vertexCode, NULL);
          glCompileShader(vertex);
          checkCompileErrors(vertex, "VERTEX");
      }

      if (shaderSources.find("FRAGMENT") != shaderSources.end()) {
          const char* fragmentCode = shaderSources["FRAGMENT"].c_str();
          fragment = glCreateShader(GL_FRAGMENT_SHADER);
          glShaderSource(fragment, 1, &fragmentCode, NULL);
          glCompileShader(fragment);
          checkCompileErrors(fragment, "FRAGMENT");
      }

      if (shaderSources.find("GEOMETRY") != shaderSources.end()) {
          const char* geometryCode = shaderSources["GEOMETRY"].c_str();
          geometry = glCreateShader(GL_GEOMETRY_SHADER);
          glShaderSource(geometry, 1, &geometryCode, NULL);
          glCompileShader(geometry);
          checkCompileErrors(geometry, "GEOMETRY");
      }

      if (shaderSources.find("TESS_CONTROL") != shaderSources.end()) {
          const char* tessControlCode = shaderSources["TESS_CONTROL"].c_str();
          tessControl = glCreateShader(GL_TESS_CONTROL_SHADER);
          glShaderSource(tessControl, 1, &tessControlCode, NULL);
          glCompileShader(tessControl);
          checkCompileErrors(tessControl, "TESS_CONTROL");
      }

      if (shaderSources.find("TESS_EVALUATION") != shaderSources.end()) {
          const char* tessEvaluationCode = shaderSources["TESS_EVALUATION"].c_str();
          tessEvaluation = glCreateShader(GL_TESS_EVALUATION_SHADER);
          glShaderSource(tessEvaluation, 1, &tessEvaluationCode, NULL);
          glCompileShader(tessEvaluation);
          checkCompileErrors(tessEvaluation, "TESS_EVALUATION");
      }

      if (shaderSources.find("COMPUTE") != shaderSources.end()) {
          const char* computeCode = shaderSources["COMPUTE"].c_str();
          compute = glCreateShader(GL_COMPUTE_SHADER);
          glShaderSource(compute, 1, &computeCode, NULL);
          glCompileShader(compute);
          checkCompileErrors(compute, "COMPUTE");
      }

      // Link shaders into a program
      this->ID = glCreateProgram();
      if (vertex != 0) glAttachShader(this->ID, vertex);
      if (fragment != 0) glAttachShader(this->ID, fragment);
      if (geometry != 0) glAttachShader(this->ID, geometry);
      if (tessControl != 0) glAttachShader(this->ID, tessControl);
      if (tessEvaluation != 0) glAttachShader(this->ID, tessEvaluation);
      if (compute != 0) glAttachShader(this->ID, compute);
      glLinkProgram(this->ID);
      checkCompileErrors(this->ID, "PROGRAM");

      // Validate the program
      glValidateProgram(this->ID);
      GLint isValid;
      glGetProgramiv(this->ID, GL_VALIDATE_STATUS, &isValid);
      if (!isValid) {
          char infoLog[1024];
          glGetProgramInfoLog(this->ID, 1024, NULL, infoLog);
          std::cout << "ERROR::PROGRAM_VALIDATION_ERROR\n" << infoLog << std::endl;
      }

      // Delete shaders as they are now linked
      if (vertex != 0) glDeleteShader(vertex);
      if (fragment != 0) glDeleteShader(fragment);
      if (geometry != 0) glDeleteShader(geometry);
      if (tessControl != 0) glDeleteShader(tessControl);
      if (tessEvaluation != 0) glDeleteShader(tessEvaluation);
      if (compute != 0) glDeleteShader(compute);
    }

    void Load(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            // if geometry shader path is present, also load a geometry shader
            if (geometryPath != nullptr)
            {
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        GLuint vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // if geometry shader is given, compile geometry shader
        unsigned int geometry;
        if (geometryPath != nullptr)
        {
            const char* gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }
        // shader Program
        this->ID = glCreateProgram();
        glAttachShader(this->ID, vertex);
        glAttachShader(this->ID, fragment);
        if (geometryPath != nullptr)
            glAttachShader(this->ID, geometry);
        glLinkProgram(this->ID);
        checkCompileErrors(this->ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometryPath != nullptr)
            glDeleteShader(geometry);
    }

    void Use() const
    {
        glUseProgram(this->ID);
    }

    GLuint getID() const 
    {
      return this->ID;
    }

    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(this->ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(this->ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(this->ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(this->ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(this->ID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(this->ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(this->ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(this->ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(this->ID, name.c_str()), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(this->ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:

    GLuint ID;

    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
