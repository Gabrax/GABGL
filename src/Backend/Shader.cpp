#include "Shader.h"
#include "BackendLogger.h"

static inline void checkCompileErrors(GLuint shader, std::string type)
{
  GLint success;
  GLchar infoLog[1024];
  if (type != "PROGRAM")
  {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
          glGetShaderInfoLog(shader, 1024, NULL, infoLog);
          GABGL_ERROR("ERROR::SHADER_COMPILATION_ERROR of type: {0}",type);
          GABGL_ERROR("ERROR::INFO: {0}",infoLog);
      }
  }
  else
  {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success)
      {
          glGetProgramInfoLog(shader, 1024, NULL, infoLog);
          GABGL_ERROR("ERROR::PROGRAM_LINKING_ERROR of type: {0}",type);
          GABGL_ERROR("ERROR::INFO: {0}",infoLog);
      }
  }
}

Shader::Shader(const char* fullshader)
{
    Timer timer;
    Load(fullshader);
    GABGL_WARN("Shader creation took {0} ms", timer.ElapsedMillis());
}

Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    Timer timer;
    Load(vertexPath, fragmentPath, geometryPath);
    GABGL_WARN("Shader creation took {0} ms", timer.ElapsedMillis());
}

void Shader::Load(const char* fullshader)
{
  std::ifstream shaderFile(fullshader);
  shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  std::string fileContent;
  try {
      std::stringstream shaderStream;
      shaderStream << shaderFile.rdbuf();
      fileContent = shaderStream.str();
  }
  catch (std::ifstream::failure& e) {
      GABGL_WARN("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: ",e.what());
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
  this->m_ID = glCreateProgram();
  if (vertex != 0) glAttachShader(this->m_ID, vertex);
  if (fragment != 0) glAttachShader(this->m_ID, fragment);
  if (geometry != 0) glAttachShader(this->m_ID, geometry);
  if (tessControl != 0) glAttachShader(this->m_ID, tessControl);
  if (tessEvaluation != 0) glAttachShader(this->m_ID, tessEvaluation);
  if (compute != 0) glAttachShader(this->m_ID, compute);
  glLinkProgram(this->m_ID);
  checkCompileErrors(this->m_ID, "PROGRAM");

  // Validate the program
  glValidateProgram(this->m_ID);
  GLint isValid;
  glGetProgramiv(this->m_ID, GL_VALIDATE_STATUS, &isValid);
  if (!isValid) {
      char infoLog[1024];
      glGetProgramInfoLog(this->m_ID, 1024, NULL, infoLog);
      GABGL_ERROR("ERROR::PROGRAM_VALIDATION_ERROR: ", infoLog);
  }

  if (vertex != 0) glDeleteShader(vertex);
  if (fragment != 0) glDeleteShader(fragment);
  if (geometry != 0) glDeleteShader(geometry);
  if (tessControl != 0) glDeleteShader(tessControl);
  if (tessEvaluation != 0) glDeleteShader(tessEvaluation);
  if (compute != 0) glDeleteShader(compute);
}

void Shader::Load(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
  std::string vertexCode;
  std::string fragmentCode;
  std::string geometryCode;
  std::ifstream vShaderFile;
  std::ifstream fShaderFile;
  std::ifstream gShaderFile;

  vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try
  {
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    std::stringstream vShaderStream, fShaderStream;

    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    vShaderFile.close();
    fShaderFile.close();

    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();

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
    GABGL_WARN("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: ", e.what());
  }
  const char* vShaderCode = vertexCode.c_str();
  const char* fShaderCode = fragmentCode.c_str();

  GLuint vertex, fragment;

  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vShaderCode, NULL);
  glCompileShader(vertex);
  checkCompileErrors(vertex, "VERTEX");

  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCode, NULL);
  glCompileShader(fragment);
  checkCompileErrors(fragment, "FRAGMENT");

  unsigned int geometry;
  if (geometryPath != nullptr)
  {
      const char* gShaderCode = geometryCode.c_str();
      geometry = glCreateShader(GL_GEOMETRY_SHADER);
      glShaderSource(geometry, 1, &gShaderCode, NULL);
      glCompileShader(geometry);
      checkCompileErrors(geometry, "GEOMETRY");
  }

  this->m_ID = glCreateProgram();
  glAttachShader(this->m_ID, vertex);
  glAttachShader(this->m_ID, fragment);
  if (geometryPath != nullptr) glAttachShader(this->m_ID, geometry);
  glLinkProgram(this->m_ID);
  checkCompileErrors(this->m_ID, "PROGRAM");

  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (geometryPath != nullptr) glDeleteShader(geometry);
}

void Shader::Bind() const
{
  glUseProgram(this->m_ID);
}
void Shader::UnBind() const
{
  glUseProgram(0);
}
GLuint Shader::GetID() const
{
  return this->m_ID;
}
void Shader::SetBool(const std::string& name, bool value) const
{
  glUniform1i(glGetUniformLocation(this->m_ID, name.c_str()), (int)value);
}
void Shader::SetInt(const std::string& name, int value) const
{
  glUniform1i(glGetUniformLocation(this->m_ID, name.c_str()), value);
}
void Shader::SetFloat(const std::string& name, float value) const
{
  glUniform1f(glGetUniformLocation(this->m_ID, name.c_str()), value);
}
void Shader::SetVec2(const std::string& name, const glm::vec2& value) const
{
  glUniform2fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, &value[0]);
}
void Shader::SetVec2(const std::string& name, float x, float y) const
{
  glUniform2f(glGetUniformLocation(this->m_ID, name.c_str()), x, y);
}
void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
  glUniform3fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, &value[0]);
}
void Shader::SetVec3(const std::string& name, float x, float y, float z) const
{
  glUniform3f(glGetUniformLocation(this->m_ID, name.c_str()), x, y, z);
}
void Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
  glUniform4fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, &value[0]);
}
void Shader::SetVec4(const std::string& name, float x, float y, float z, float w) const
{
  glUniform4f(glGetUniformLocation(this->m_ID, name.c_str()), x, y, z, w);
}
void Shader::SetMat2(const std::string& name, const glm::mat2& mat) const
{
  glUniformMatrix2fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::SetMat3(const std::string& name, const glm::mat3& mat) const
{
  glUniformMatrix3fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
  glUniformMatrix4fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

std::shared_ptr<Shader> Shader::Create(const char* fullshader)
{ 
  return std::make_shared<Shader>(fullshader);
}

std::shared_ptr<Shader> Shader::Create(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{ 
  return std::make_shared<Shader>(vertexPath,fragmentPath,geometryPath);
}

