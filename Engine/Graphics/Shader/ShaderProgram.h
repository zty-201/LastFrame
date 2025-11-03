/**
 * @file ShaderProgram.h
 * @brief Declaration of the ShaderProgram class for managing OpenGL shader programs.
 *
 * This file defines the `ShaderProgram` class, which provides functionality for compiling,
 * linking, and managing OpenGL shader programs. It includes methods for handling shader
 * source files, managing uniforms, and interacting with OpenGL for rendering operations.
 *
 * Key functionalities:
 * - Compilation and linking of vertex and fragment shaders.
 * - Management of uniform variables and their locations.
 * - Utility functions for debugging, such as printing active uniforms and attributes.
 * - Integration with material components for setting material-related uniforms.
 *
 * @author Zhang TongYan
 * @date Sept 2025
 */

#ifndef SHADER_H
#define SHADER_H

#include "CameraComponent.h"
#include "LightComponent.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"
//
    // Shader ShaderProgram class for managing shader program handle and operations.
    //
class ShaderProgram
{
public:
    GLuint handle; // OpenGL shader program handle
private:
    bool linked; // Indicates if the shader program is successfully linked
    std::map<std::string, int> uniformLocations; // Cache for uniform variable locations

    // Helper function to retrieve the location of a uniform variable by name
    GLint getUniformLocation(const std::string& name)
    {
        // Search for the uniform location in the cache
        auto pos = uniformLocations.find(name);
        if (pos == uniformLocations.end())
            // If not found, get the location from OpenGL and store it in the cache
            return uniformLocations[name] = glGetUniformLocation(handle, name.c_str());
        return pos->second; // Return cached location if found
    }

public:
    // Constructor that takes a pre-existing OpenGL program handle
    ShaderProgram(GLuint handle);
    // Constructor that takes file paths for a vertex shader and a fragment shader
    ShaderProgram(const char* sv, const char* sf);
    // Takes source code strings directly.
    ShaderProgram(const char* vertexSource, const char* fragmentSource, bool isSourceCode);
	// Takes source code strings directly for a compute shader.
    ShaderProgram(const char* cs);

    ~ShaderProgram(); // Destructor to clean up resources

    // Delete copy constructor and assignment operator to prevent copying
    ShaderProgram(const ShaderProgram& rhs) = delete;
    ShaderProgram& operator=(const ShaderProgram& rhs) = delete;

    // Convert the ShaderProgram object to its OpenGL handle for easier use in OpenGL functions
    operator GLuint()
    {
        return handle;
    }

    // Compile a shader from a file
    void compileShaderFile(const char* fileName, GLenum type);
    // Compile a shader from a code string
    void compileShaderString(const char* code, GLenum type);

    void validate(); // Validate the program (ensure it works correctly)
    void use() const; // Activate the shader program for use
    void disuse() const; // Deactivate the shader program
    void detachAndDeleteShaderObjects(); // Detach and delete shader objects (not implemented here)
    void link(); // Link the shader program (not implemented here)

    // Bind an attribute location (used to associate vertex data with shader inputs)
    void bindAttribLocation(GLuint location, const char* name) {
        glBindAttribLocation(handle, location, name);
    }

    // Set a uniform variable of type vec3 (float x, y, z)
    void setUniform(const std::string& name, float x, float y, float z) {
        glUniform3f(getUniformLocation(name), x, y, z);
    }

    // Set a uniform variable of type vec2 (glm::vec2)
    void setUniform(const std::string& name, const glm::vec2& v) {
        glUniform2f(getUniformLocation(name), v.x, v.y);
    }

    // Set a uniform variable of type vec3 (glm::vec3)
    void setUniform(const std::string& name, const glm::vec3& v) {
        this->setUniform(name, v.x, v.y, v.z);
    }

    // Set a uniform variable of type vec4 (glm::vec4)
    void setUniform(const std::string& name, const glm::vec4& v) {
        glUniform4f(getUniformLocation(name), v.x, v.y, v.z, v.w);
    }

    // Set a uniform variable for vector types (std::vector<float>)
    void setUniform(const std::string& name, const std::vector<float>& v) {
        if (v.size() == 1)
            glUniform1f(getUniformLocation(name), v[0]);
        else if (v.size() == 2)
            glUniform2f(getUniformLocation(name), v[0], v[1]);
        else if (v.size() == 3)
            glUniform3f(getUniformLocation(name), v[0], v[1], v[2]);
        else if (v.size() == 4)
            glUniform4f(getUniformLocation(name), v[0], v[1], v[2], v[3]);
    }

    // Set a uniform matrix4fv (4x4 matrix, typically used for transformations)
    void setUniform(const std::string& name, const glm::mat4& m) {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &m[0][0]);
    }

    // Set a uniform matrix3fv (3x3 matrix)
    void setUniform(const std::string& name, const glm::mat3& m) {
        glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &m[0][0]);
    }

    // Set a uniform float variable
    void setUniform(const std::string& name, float val) {
        glUniform1f(getUniformLocation(name), val);
    }

    // Set a uniform integer variable
    void setUniform(const std::string& name, int val) {
        glUniform1i(getUniformLocation(name), val);
    }

    // Set a uniform boolean variable (converted to int)
    void setUniform(const std::string& name, bool val) {
        glUniform1i(getUniformLocation(name), val);
    }

    // Set a uniform unsigned integer variable
    void setUniform(const std::string& name, GLuint val) {
        glUniform1ui(getUniformLocation(name), val);
    }

    void setUniform(const std::string& name, GLsizei count, const GLuint* value) {
        glUniform1uiv(getUniformLocation(name), count, value);
    }

    void setMaterialUniforms(const MaterialComponent& material)
    {
        setUniform("material.rough", material.rough);
        setUniform("material.metal", material.metal);
        setUniform("material.color", material.color);
        setUniform("material.Ka", material.Ka);
        setUniform("material.Kd", material.Kd);
        setUniform("material.Ks", material.Ks);
        setUniform("material.shininess", material.shininess);
        setUniform("material.effect", material.effect); // *** ADD THIS LINE ***
    }

    // Implemented in .cpp file
    void findUniformLocations();

    void printActiveUniforms();
    void printActiveUniformBlocks();
    void printActiveAttribs();
};

#endif // SHADER_H
