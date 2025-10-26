/**
 * @file FBO.h
 * @brief Defines the FBO struct for managing OpenGL Framebuffer Objects.
 *
 * This file contains the `FBO` struct, which provides functionality for setting up
 * and managing OpenGL Framebuffer Objects (FBOs). It supports various rendering
 * techniques such as texture rendering, depth-only rendering, HDR, and deferred rendering.
 *
 * The `FBO` struct includes methods for resizing framebuffers, creating texture and
 * depth buffers, and configuring framebuffers for specific rendering types.
 *
 * @author Zhang TongYan
 * @date October, 2025
 */
#ifndef FBO_H
#define FBO_H

#include <string>
#include <glm/glm.hpp>

//
// This struct that sets up and manages OpenGL Framebuffer Objects (FBOs), allowing 
// different rendering techniques(e.g., texture rendering, depth-only
// rendering, HDR, deferred rendering) to be used in a graphics application.
//
struct FBO
{
    // Enum to define different types of framebuffers
    enum Type {
        Default = 0,       // Default framebuffer
        Texture,           // Standard texture framebuffer
        TextureHDR,        // High Dynamic Range (HDR) texture framebuffer
        TextureDepth,      // Depth-only texture framebuffer
        Deferred           // Framebuffer for deferred rendering
    } type; // Specifies the type of framebuffer

    GLuint handle;         // OpenGL handle for the framebuffer object
    GLuint* pTexture;     // Pointer to the associated texture
    int framebufferWidth;             // Width of the framebuffer
    int framebufferHeight;            // Height of the framebuffer
    // Texture handles for deferred rendering
    GLuint gPosition = 0;
    GLuint gNormal = 0;
    GLuint gAlbedoSpec = 0;

    // Constructor to initialize the FBO with specific attributes
    FBO(Type type = Default, int width = 0, int height = 0, GLuint* pTexture = nullptr)
        :
        type{ type },
        handle{ 0 },
        pTexture{ pTexture },
        framebufferWidth{ width },
        framebufferHeight{ height }
    {
        // Resizes the framebuffer to match the specified dimensions
        resize(width, height);
    }

    // Getters for deferred textures
    GLuint getPositionTexture() const { return gPosition; }
    GLuint getNormalTexture() const { return gNormal; }
    GLuint getAlbedoSpecTexture() const { return gAlbedoSpec; }

    // Resizes the framebuffer object based on its type
    void resize(int width, int height)
    {
        switch (type)
        {
        case Default:
            break; // No additional setup for default type
        case Texture:
            resetTextureAsTarget(width, height, GL_RGBA8); // Setup for color texture
            break;
        case TextureHDR:
            resetTextureAsTarget(width, height, GL_RGB32F); // Setup for HDR texture
            break;
        case TextureDepth:
            resetTextureDepthAsTarget(width, height); // Setup for depth texture
            break;
        case Deferred:
            resetDeferredAsTarget(width, height); // Setup for deferred rendering
            break;
        }
    }

    // Creates a texture buffer and attaches it to the FBO
    void createTextureBuffer(int width, int height, GLenum texUnit,
        GLenum format, GLenum attachment, GLuint* pTexid = nullptr)
    {
        GLuint texid;

        // Activate the specified texture unit and generate a texture
        glActiveTexture(texUnit);

        glGenTextures(1, &texid);
        glBindTexture(GL_TEXTURE_2D, texid);
        glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        // Attach the texture to the framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texid, 0);

        if (pTexid)
            *pTexid = texid; // Return texture ID if a pointer is provided
    }

    // Creates a renderbuffer for depth storage and attaches it to the FBO
    void createDepthBuffer(int width, int height)
    {
        GLuint depthBuf;
        glGenRenderbuffers(1, &depthBuf);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

        // Attach the depth buffer to the framebuffer
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);
    }

    // Configures the FBO for rendering to a color texture with a depth buffer
    void resetTextureAsTarget(int width, int height, GLenum format)
    {
        //Framebuffer must have a valid size.
        if (width == 0 || height == 0) return;
        if (handle)
            glDeleteFramebuffers(1, &handle);

        if (pTexture && *pTexture) // Add check if pointer is valid before deleting
            glDeleteTextures(1, pTexture);

        // Create and bind the framebuffer
        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);

        // Create a color buffer texture and attach it
        createTextureBuffer(width, height, GL_TEXTURE0, format, GL_COLOR_ATTACHMENT0, pTexture);

        // Create and attach a depth buffer
        createDepthBuffer(width, height);

        // Specify the draw buffers
        GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBuffers);

        // Check the status of OUR FBO (handle) before unbinding it
        GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE)
        {
            // Unbind before throwing to avoid leaving our broken FBO bound.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            throw std::runtime_error(std::string("Error ") +
                std::to_string(result) +
                ": Framebuffer is not complete.\n");
        }
        // Revert to the default framebuffer.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Also unbind the texture to avoid state conflicts
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void resetTextureDepthAsTarget(int width, int height)
    {
        GLuint texture = 0;

        if (handle)
            glDeleteFramebuffers(1, &handle);

        if (pTexture && *pTexture) // Add check if pointer is valid before deleting
            glDeleteTextures(1, pTexture);

        GLfloat border[] = { 1.0f, 0.0f,0.0f,0.0f };

        // The depth buffer texture
        glGenTextures(1, pTexture ? pTexture : &texture);
        glBindTexture(GL_TEXTURE_2D, pTexture ? *pTexture : texture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

        // Assign the depth buffer texture to texture channel 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pTexture ? *pTexture : texture);

        // Create and set up the FBO
        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pTexture ? *pTexture : texture, 0);

        GLenum drawBuffers[] = { GL_NONE };
        glDrawBuffers(1, drawBuffers);

        // Check status before unbinding
        GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            throw std::runtime_error(std::string("Error ") +
                std::to_string(result) +
                ": Framebuffer is not complete.\n");
        }

        // Unbind after successful check 
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Configures the FBO for deferred rendering
    void resetDeferredAsTarget(int width, int height)
    {
        if (handle)
            glDeleteFramebuffers(1, &handle);

        // Delete previous textures if they exist
        if (gPosition) glDeleteTextures(1, &gPosition);
        if (gNormal) glDeleteTextures(1, &gNormal);
        if (gAlbedoSpec) glDeleteTextures(1, &gAlbedoSpec);

        // Create and bind the framebuffer
        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);

        // Create textures for different deferred rendering passes
        createTextureBuffer(width, height, GL_TEXTURE0, GL_RGB16F, GL_COLOR_ATTACHMENT0, &gPosition);
        createTextureBuffer(width, height, GL_TEXTURE1, GL_RGB16F, GL_COLOR_ATTACHMENT1, &gNormal);
        createTextureBuffer(width, height, GL_TEXTURE2, GL_RGBA8, GL_COLOR_ATTACHMENT2, &gAlbedoSpec);

        // Create and attach a depth buffer
        createDepthBuffer(width, height);

        // Define an array of buffers into which outputs from 
        // the fragment shader data will be written
        GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, drawBuffers);

        // Check status before unbinding 
        GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            std::string errorMsg;
            switch (result) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                errorMsg = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                errorMsg = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                errorMsg = "GL_FRAMEBUFFER_UNSUPPORTED"; break;
                // Add more cases as needed
            default:
                errorMsg = "Unknown error"; break;
            }
            throw std::runtime_error("Framebuffer incomplete: " + errorMsg + " (" +
                std::to_string(result) + ")");
        }

        // Unbind after successful check 
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

#endif // FBO_H