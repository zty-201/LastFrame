/*!*****************************************************************************
\file debug.h
\author Vadim Surov (vsurov\@digipen.edu)
\par Course: CSD2151/CSD2150/CS250
\par Assignment: all
\date 12/26/2024 (MM/DD/YYYY)
\brief This file has declarations of helper functions for debugging.
*******************************************************************************/
#pragma once

// Helper functions to build edge cases
uint64_t EdgeKey(uint32_t a, uint32_t b);
std::vector<uint32_t>BuildUniqueEdgeIndices(const std::vector<GLuint>& triIdx);

/**
 * \brief Checks for OpenGL errors and logs them.
 *
 * \param file The source file where this function is called.
 * \param line The line number where this function is called.
 * \return 1 if an OpenGL error is detected, 0 otherwise.
 */
int checkForOpenGLError(const char* file, int line);

/**
 * \brief OpenGL debug callback function to log detailed debug information.
 *
 * \param source The origin of the debug message (e.g., API, Shader Compiler).
 * \param type The type of the message (e.g., error, performance).
 * \param id The ID of the debug message.
 * \param severity The severity of the message (e.g., high, medium).
 * \param length Length of the debug message string.
 * \param msg The debug message string.
 * \param param Additional user-defined parameter (not used).
 */
void debugCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* msg, const void* param);

/**
 * \brief Dumps OpenGL context information such as vendor, renderer, and version.
 *
 * \param dumpExtensions If true, also outputs the list of supported OpenGL extensions.
 */
void dumpGLInfo(bool dumpExtensions = false);
