///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a shader program. This contains methods
// to build a shader program from multiples files containing vertex
// and pixel shader code, and a method to link the result.  When
// loaded (method "Use"), its vertex shader and pixel shader will be
// invoked for all geometry passing through the graphics pipeline.
// When done, unload it with method "Unuse".
////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "shader.h"

// Reads a specified file into a string and returns the string.
char* ReadFile(const char* name)
{
    // First try to open the file
    std::ifstream f(name, std::ios_base::binary);
    if (!f.is_open()) {
        std::cerr << "Error: Could not open file " << name << std::endl;
        return nullptr;
    }
    
    // Get file size
    f.seekg(0, std::ios_base::end);
    int length = static_cast<int>(f.tellg());
    f.seekg(0, std::ios_base::beg);
    
    // Create buffer with extra space for null terminator
    char* content = new char[length + 1];
    
    // Read the file content
    f.read(content, length);
    
    // Check if read was successful
    if (!f) {
        int actualLength = static_cast<int>(f.gcount());
        std::cerr << "Warning: Only read " << actualLength << " of " << length << " bytes from " << name << std::endl;
        // Ensure we don't write past what we actually read
        content[actualLength] = '\0';
    } else {
        // Normal termination
        content[length] = '\0';
    }
    
    f.close();
    return content;
}

// Creates an empty shader program.
ShaderProgram::ShaderProgram()
{ 
    programId = glCreateProgram();
}

// Use a shader program
void ShaderProgram::UseShader()
{
    glUseProgram(programId);
}

// Done using a shader program
void ShaderProgram::UnuseShader()
{
    glUseProgram(0);
}

// Read, send to OpenGL, and compile a single file into a shader
// program.  In case of an error, retrieve and print the error log
// string.
void ShaderProgram::AddShader(const char* fileName, GLenum type)
{
    // Read the source from the named file
    char* src = ReadFile(fileName);
    if (!src) {
        std::cerr << "Failed to read shader file: " << fileName << std::endl;
        return;
    }
    
    const char* psrc[1] = {src};

    // Create a shader and attach, hand it the source, and compile it.
    int shader = glCreateShader(type);
    glAttachShader(programId, shader);
    glShaderSource(shader, 1, psrc, NULL);
    glCompileShader(shader);
    delete[] src; // Use delete[] for arrays, not delete

    // Get the compilation status
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    
    // If compilation status is not OK, get and print the log message.
    if (status != 1) {
        int length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* buffer = new char[length];
        glGetShaderInfoLog(shader, length, NULL, buffer);
        printf("Compile log for %s:\n%s\n", fileName, buffer);
        delete[] buffer; // Use delete[] for arrays
    }
}

// Link a shader program after all the shader files have been added
// with the AddShader method.  In case of an error, retrieve and print
// the error log string.
void ShaderProgram::LinkProgram()
{
    // Link program and check the status
    glLinkProgram(programId);
    int status;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    
    // If link failed, get and print log
    if (status != 1) {
        int length;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &length);
        char* buffer = new char[length];
        glGetProgramInfoLog(programId, length, NULL, buffer);
        printf("Link log:\n%s\n", buffer);
        delete[] buffer; // Use delete[] for arrays
    }
}

// Clean up the shader program resources
ShaderProgram::~ShaderProgram()
{
    if (programId) {
        glDeleteProgram(programId);
        programId = 0;
    }
}

// Helper to get uniform location with caching
GLint ShaderProgram::GetUniformLocation(const std::string& name)
{
    if (uniformLocationCache.find(name) != uniformLocationCache.end())
        return uniformLocationCache[name];
    
    GLint location = glGetUniformLocation(programId, name.c_str());
    uniformLocationCache[name] = location;
    return location;
}

// Uniform setter implementations
void ShaderProgram::SetBool(const std::string& name, bool value)
{
    glUniform1i(GetUniformLocation(name), static_cast<int>(value));
}

void ShaderProgram::SetInt(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void ShaderProgram::SetFloat(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void ShaderProgram::SetVec2(const std::string& name, const glm::vec2& value)
{
    glUniform2fv(GetUniformLocation(name), 1, &value[0]);
}

void ShaderProgram::SetVec3(const std::string& name, const glm::vec3& value)
{
    glUniform3fv(GetUniformLocation(name), 1, &value[0]);
}

void ShaderProgram::SetVec4(const std::string& name, const glm::vec4& value)
{
    glUniform4fv(GetUniformLocation(name), 1, &value[0]);
}

void ShaderProgram::SetMat4(const std::string& name, const glm::mat4& value)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}
