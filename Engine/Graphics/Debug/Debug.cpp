/*!*****************************************************************************
\file debug.cpp
\author Vadim Surov (vsurov\@digipen.edu)
\par Course: CSD2151/CSD2150/CS250
\par Assignment: all
\date 12/26/2024 (MM/DD/YYYY)
\brief This file has definitions of helper functions for debugging.
*******************************************************************************/
#include "pch.hpp"
#include "Debug.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <unordered_set>

// Helper function for creating Edges used in debug Drawing
inline uint64_t EdgeKey(uint32_t a, uint32_t b)
{
    if (a > b) std::swap(a, b);
    return (uint64_t(a) << 32) | uint64_t(b);
}

std::vector<uint32_t> BuildUniqueEdgeIndices(const std::vector<GLuint>& triIdx)
{
    std::vector<uint32_t> edges;
    edges.reserve(triIdx.size() * 2);
    std::unordered_set<uint64_t> seen;
    seen.reserve(triIdx.size());

    for (size_t i = 0; i + 2 < triIdx.size(); i += 3) {
        uint32_t i0 = triIdx[i + 0], i1 = triIdx[i + 1], i2 = triIdx[i + 2];
        const uint32_t e[3][2] = { {i0,i1}, {i1,i2}, {i2,i0} };
        for (int k = 0; k < 3; ++k) {
            uint32_t a = e[k][0], b = e[k][1];
            if (seen.insert(EdgeKey(a, b)).second) {
                edges.push_back(a);
                edges.push_back(b);
            }
        }
    }
    return edges;
}

/**
 * \brief Checks for OpenGL errors and reports them.
 *
 * \param file The source file where this function is called (usually passed as __FILE__).
 * \param line The line number where this function is called (usually passed as __LINE__).
 * \return 1 if an OpenGL error is found, 0 otherwise.
 */
int checkForOpenGLError(const char* file, int line)
{
    GLenum glErr;
    int retCode = 0;

    // Loop to check and report all OpenGL errors in the error queue.
    glErr = glGetError();
    while (glErr != GL_NO_ERROR)
    {
        const char* message = "";
        switch (glErr)
        {
        case GL_INVALID_ENUM:
            message = "Invalid enum";
            break;
        case GL_INVALID_VALUE:
            message = "Invalid value";
            break;
        case GL_INVALID_OPERATION:
            message = "Invalid operation";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            message = "Invalid framebuffer operation";
            break;
        case GL_OUT_OF_MEMORY:
            message = "Out of memory";
            break;
        default:
            message = "Unknown error";
        }

        // Log the error with file and line number for easier debugging.
        LOGI("glError in file %s @ line %d: %s", file, line, message);
        retCode = 1;
        glErr = glGetError(); // Check the next error.
    }
    return retCode;
}
/**
 * \brief OpenGL ES 3.1 debug callback function.
 *
 * \param source The origin of the message (e.g., API, Shader Compiler, etc.).
 * \param type The type of error (e.g., error, deprecated behavior, etc.).
 * \param id The ID of the error message.
 * \param severity The severity of the error (e.g., high, medium, etc.).
 * \param length Length of the message string.
 * \param msg The error/debug message.
 * \param param Additional user-defined parameters (not used here).
 */
void debugCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* msg, const void* param)
{
    (void)length;
    (void)param;

    // Determine the source of the debug message.
    std::string sourceStr;
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        sourceStr = "OpenGL";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceStr = "ShaderCompiler";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        sourceStr = "App";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceStr = "3rdParty";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        sourceStr = "Other";
        break;
    default:
        sourceStr = "Unknown";
    }

    // Determine the type of the debug message.
    std::string typeStr;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "Error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "Deprecated";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "Undefined";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "Performance";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeStr = "Other";
        break;
    default:
        typeStr = "Unknown";
    }

    // Determine the severity of the debug message.
    std::string sevStr;
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        sevStr = "HIGH";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        sevStr = "MED";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        sevStr = "LOW";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        sevStr = "NOTIFY";
        break;
    default:
        sevStr = "UNK";
    }

    // Log the formatted debug message.
    std::cerr << sourceStr << ":" << typeStr << "[" << sevStr << "]"
        << "(" << id << "): " << msg << std::endl;
}


/**
 * \brief Dumps OpenGL context information, such as version, vendor, and extensions.
 *
 * \param dumpExtensions If true, also dumps the list of supported extensions.
 */
void dumpGLInfo(bool dumpExtensions /* = false*/)
{
    // Retrieve and print OpenGL context information.
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint major, minor, samples, sampleBuffers;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    glGetIntegerv(GL_SAMPLES, &samples);
    glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);

    // Print general OpenGL information.
    LOGI("-------------------------------------------------------------");
    LOGI("GL Vendor    : %s", vendor);
    LOGI("GL Renderer  : %s", renderer);
    LOGI("GL Version   : %s", version);
    LOGI("GL Version   : %d.%d", major, minor);
    LOGI("GLSL Version : %s", glslVersion);
    LOGI("MSAA samples : %d", samples);
    LOGI("MSAA buffers : %d", sampleBuffers);
    LOGI("-------------------------------------------------------------");

    // Optionally dump the list of supported extensions.
    if (dumpExtensions) {
        GLint nExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
        for (int i = 0; i < nExtensions; i++) {
            LOGI("%s", glGetStringi(GL_EXTENSIONS, i));
        }
    }
}