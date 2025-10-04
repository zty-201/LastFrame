/******************************************************************/
/*!
\file   ShaderProgram.cpp
\author Zhang TongYan
\par    tongyan.zhang\@digipen.edu
\date   October 2025
\brief
This file contains the implementation of the ShaderProgram class, which
manages the compilation, linking, and usage of OpenGL shader programs.
It also provides utilities for handling shader uniforms, attributes, and
error logging.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************/

#include "pch.hpp"
#include "ShaderProgram.h"
#include "AssetManager.hpp"
// Function to convert OpenGL shader data type enums to strings.
const char* getTypeString(GLenum type)
{
    // There are many more types than are covered here, but
    // these are the most common in these examples.
    switch (type) {
    case GL_FLOAT:
        return "float";
    case GL_FLOAT_VEC2:
        return "vec2";
    case GL_FLOAT_VEC3:
        return "vec3";
    case GL_FLOAT_VEC4:
        return "vec4";
    case GL_INT:
        return "int";
    case GL_UNSIGNED_INT:
        return "unsigned int";
    case GL_BOOL:
        return "bool";
    case GL_FLOAT_MAT2:
        return "mat2";
    case GL_FLOAT_MAT3:
        return "mat3";
    case GL_FLOAT_MAT4:
        return "mat4";
    default:
        return "?";
    }
}

// Constructor initializing a shader program from a pre-existing OpenGL program handle.
ShaderProgram::ShaderProgram(GLuint handle)
    : handle(handle), linked(false)
{
}

// Constructor that compiles vertex and fragment shaders from file paths and links them.
ShaderProgram::ShaderProgram(const char* sv, const char* sf)
    : handle(0), linked(false)
{
    if (sv && sf && strlen(sv) && strlen(sf)) // Check that shader file paths are valid.
    {
        LOGD("Compiling Vertex Shader: %s", sv);
        compileShaderFile(sv, GL_VERTEX_SHADER);  // Compile vertex shader.
        LOGD("Compiling Fragment Shader: %s", sf);
        compileShaderFile(sf, GL_FRAGMENT_SHADER);  // Compile fragment shader.
        link();  // Link the shaders into a program.
    }
}

// Destructor: cleans up by detaching and deleting shader objects, and deleting the program.
ShaderProgram::~ShaderProgram()
{
    if (!handle || !linked)
        return;
    detachAndDeleteShaderObjects();  // Detach and delete shaders.
    glDeleteProgram(handle);  // Delete the shader program.
}

// Detach and delete shader objects from the program.
void ShaderProgram::detachAndDeleteShaderObjects()
{
    // Detach and delete the shader objects (if they are not already removed)
    GLint numShaders = 0;
    glGetProgramiv(handle, GL_ATTACHED_SHADERS, &numShaders);  // Get the number of attached shaders.
    std::vector<GLuint> shaderNames(numShaders);
    glGetAttachedShaders(handle, numShaders, NULL, shaderNames.data());  // Get shader handles.
    for (GLuint shader : shaderNames)  // Iterate through shaders to detach and delete them.
    {
        glDetachShader(handle, shader);  // Detach shader.
        glDeleteShader(shader);  // Delete shader.
    }
}

// Compile a shader from a file.
void ShaderProgram::compileShaderFile(const char* fileName, GLenum type)
{
    //Load using cross-platform compatible function.
    std::vector<unsigned char> file_data = EngineFramework::AssetManagerSystem::GetDataFromFile(std::filesystem::path(fileName));
    if (file_data.empty())
    {
        throw std::exception((std::string("Unable to read filepath: ") + fileName + ", when compiling shader.").c_str());
    }
    //A null-terminated string is expected.
    file_data.push_back('\0');
    try
    {
        compileShader(reinterpret_cast<char*>(file_data.data()), type);  // Compile the shader using the code read from the file.
    }
    //Rethrow with additional debug info.
    catch (std::runtime_error& e)
    {
        throw std::exception((std::string("Error in shader file compilation for: ") + fileName + "\n" + e.what()).c_str());
    }
}

// Compile a shader from source code.
void ShaderProgram::compileShader(const char* code, GLenum type)
{
    if (!handle)  // If no valid program handle, create a new program.
    {
        handle = glCreateProgram();
        if (!handle)
            throw std::runtime_error("Unable to create shader program.");
    }

    GLuint shaderHandle = glCreateShader(type);  // Create a shader object of the specified type.
    glShaderSource(shaderHandle, 1, &code, NULL);  // Set the shader source code.
    glCompileShader(shaderHandle);  // Compile the shader.

    int result;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &result);  // Check compilation status.
    if (GL_FALSE == result)  // If compilation fails, retrieve and throw error message.
    {
        std::string msg = "Shader compilation failed.\n";

        int length = 0;
        glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length);  // Get error log length.
        if (length > 0)
        {
            std::string log(length, ' ');
            int written = 0;
            glGetShaderInfoLog(shaderHandle, length, &written, &log[0]);  // Get compilation error log.
            msg += log;
        }
        throw std::runtime_error(msg);  // Throw the error with the message.
    }
    else
    {
        glAttachShader(handle, shaderHandle);  // Attach the shader to the program.
    }
}

// Link the program after shaders have been attached.
void ShaderProgram::link()
{
    if (!handle)
        throw std::runtime_error("ShaderProgram has not been compiled.");

    glLinkProgram(handle);  // Link the program.
    int status = 0;
    std::string errString;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);  // Check link status.
    if (GL_FALSE == status)  // If linking fails, retrieve and throw error message.
    {
        // Store log and return false
        int length = 0;
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);  // Get error log length.
        errString += "ShaderProgram link failed:\n";
        if (length > 0)
        {
            std::string log(length, ' ');
            int written = 0;
            glGetProgramInfoLog(handle, length, &written, &log[0]);  // Get linking error log.
            errString += log;
        }
    }
    else
    {
        findUniformLocations();  // Find locations of uniforms in the linked program.
        linked = true;  // Set the program as successfully linked.
    }

    detachAndDeleteShaderObjects();  // Detach and delete shader objects.

    if (GL_FALSE == status)
        throw std::runtime_error(errString);  // Throw an error if linking fails.
}

// Find and store the locations of all active uniforms in the program.
void ShaderProgram::findUniformLocations()
{
    uniformLocations.clear();  // Clear previous uniform locations.

    GLint numUniforms = 0;

    // For OpenGL 4.3 and above, use glGetProgramResource
    glGetProgramInterfaceiv(handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms); // Get number of active uniforms.

    GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX };  // Properties to query for each uniform.

    // Loop through all uniforms and retrieve their properties.
    for (GLint i = 0; i < numUniforms; ++i)
    {
        GLint results[4];
        glGetProgramResourceiv(handle, GL_UNIFORM, i, 4, properties, 4, NULL, results);  // Get uniform properties.

        if (results[3] != -1) continue;  // Skip uniforms inside blocks.

        GLint nameBufSize = results[0] + 1;
        char* name = new char[nameBufSize];
        glGetProgramResourceName(handle, GL_UNIFORM, i, nameBufSize, NULL, name);  // Get uniform name.
        uniformLocations[name] = results[2];  // Store uniform location.
        delete[] name;
    }
}

// Set the shader program as the current active program.
void ShaderProgram::use() const
{
    glUseProgram(handle);  // Set the shader program to be used.
}

// Disable the use of the current shader program (use the default).
void ShaderProgram::disuse() const
{
    glUseProgram(0);  // Disable the use of the current program.
}

// Print all active uniforms in the program to the console.
void ShaderProgram::printActiveUniforms()
{
    // For OpenGL 4.3 and above, use glGetProgramResource
    GLint numUniforms = 0;
    glGetProgramInterfaceiv(handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms); // Get number of active uniforms.

    GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX };  // Properties to query for each uniform.

    LOGI("Active uniforms:");
    for (int i = 0; i < numUniforms; ++i) {
        GLint results[4];
        glGetProgramResourceiv(handle, GL_UNIFORM, i, 4, properties, 4, NULL, results);  // Get uniform properties.

        if (results[3] != -1) continue;  // Skip uniforms inside blocks.

        GLint nameBufSize = results[0] + 1;
        char* name = new char[nameBufSize];
        glGetProgramResourceName(handle, GL_UNIFORM, i, nameBufSize, NULL, name);  // Get uniform name.
        LOGI("%-5d %s (%s)", results[2], name, getTypeString(results[1]));  // Print uniform location and type.
        delete[] name;
    }
}

// Print all active uniform blocks in the program to the console.
void ShaderProgram::printActiveUniformBlocks()
{
    GLint numBlocks = 0;

    glGetProgramInterfaceiv(handle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numBlocks);  // Get number of uniform blocks.
    GLenum blockProps[] = { GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH };
    GLenum blockIndex[] = { GL_ACTIVE_VARIABLES };
    GLenum props[] = { GL_NAME_LENGTH, GL_TYPE, GL_BLOCK_INDEX };

    // Loop through each block to print details.
    for (int block = 0; block < numBlocks; ++block) {
        GLint blockInfo[2];
        glGetProgramResourceiv(handle, GL_UNIFORM_BLOCK, block, 2, blockProps, 2, NULL, blockInfo);
        GLint numUnis = blockInfo[0];

        char* blockName = new char[blockInfo[1] + 1];
        glGetProgramResourceName(handle, GL_UNIFORM_BLOCK, block, blockInfo[1] + 1, NULL, blockName);
        LOGI("Uniform block \"%s\":", blockName);  // Print block name.
        delete[] blockName;

        GLint* unifIndexes = new GLint[numUnis];
        glGetProgramResourceiv(handle, GL_UNIFORM_BLOCK, block, 1, blockIndex, numUnis, NULL, unifIndexes);

        // Loop through each uniform in the block.
        for (int unif = 0; unif < numUnis; ++unif) {
            GLint uniIndex = unifIndexes[unif];
            GLint results[3];
            glGetProgramResourceiv(handle, GL_UNIFORM, uniIndex, 3, props, 3, NULL, results);

            GLint nameBufSize = results[0] + 1;
            char* name = new char[nameBufSize];
            glGetProgramResourceName(handle, GL_UNIFORM, uniIndex, nameBufSize, NULL, name);
            LOGI("    %s (%s)", name, getTypeString(results[1]));  // Print uniform name and type.
            delete[] name;
        }

        delete[] unifIndexes;
    }
}

// Print all active attributes in the program to the console.
void ShaderProgram::printActiveAttribs()
{
    // >= OpenGL 4.3, use glGetProgramResource
    GLint numAttribs;
    glGetProgramInterfaceiv(handle, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numAttribs);  // Get number of active attributes.

    GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };  // Properties to query for each attribute.

    LOGI("Active attributes:");
    for (int i = 0; i < numAttribs; ++i) {
        GLint results[3];
        glGetProgramResourceiv(handle, GL_PROGRAM_INPUT, i, 3, properties, 3, NULL, results);  // Get attribute properties.

        GLint nameBufSize = results[0] + 1;
        char* name = new char[nameBufSize];
        glGetProgramResourceName(handle, GL_PROGRAM_INPUT, i, nameBufSize, NULL, name);  // Get attribute name.
        LOGI("%-5d %s (%s)", results[2], name, getTypeString(results[1]));  // Print attribute location and type.
        delete[] name;
    }
}

// Validate the shader program by checking its correctness.
void ShaderProgram::validate()
{
    if (linked)  // If the program is already linked, skip validation.
        throw std::runtime_error("ShaderProgram is not linked");

    GLint status;
    glValidateProgram(handle);  // Validate the shader program.
    glGetProgramiv(handle, GL_VALIDATE_STATUS, &status);  // Get validation status.

    if (GL_FALSE == status) {  // If validation fails, retrieve and throw error message.
        int length = 0;
        std::string logString;

        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);  // Get error log length.

        if (length > 0) {
            char* c_log = new char[length];
            int written = 0;
            glGetProgramInfoLog(handle, length, &written, c_log);
            logString = c_log;
            delete[] c_log;
        }

        throw std::runtime_error(std::string("ShaderProgram failed to validate\n") + logString);
    }
}