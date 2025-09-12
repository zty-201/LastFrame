#ifndef SHADER_H
#define SHADER_H

#include <unordered_map>

class ShaderProgram 
{
public:
    ShaderProgram();
    ~ShaderProgram();
    
    // Load from files
    void AddShader(const char* fileName, GLenum type);
    void LinkProgram();
    void UseShader();
    void UnuseShader();
    
    // Uniform setters
    void SetBool(const std::string& name, bool value);
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec2(const std::string& name, const glm::vec2& value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetVec4(const std::string& name, const glm::vec4& value);
    void SetMat4(const std::string& name, const glm::mat4& value);
    
    GLuint GetProgramID() const { return programId; }

private:
    GLuint programId;
    std::unordered_map<std::string, GLint> uniformLocationCache;
    
    // Helper to get uniform location with caching
    GLint GetUniformLocation(const std::string& name);
};

#endif // SHADER_H
