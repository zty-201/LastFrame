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
    int width;             // Width of the framebuffer
    int height;            // Height of the framebuffer

    // Constructor to initialize the FBO with specific attributes
    FBO(Type type = Default, int width = 0, int height = 0, GLuint* pTexture = nullptr)
        :
        type{ type },
        handle{ 0 },
        pTexture{ pTexture },
        width{ width },
        height{ height }
    {
        // Resizes the framebuffer to match the specified dimensions
        resize(width, height);
    }

    // Resizes the framebuffer object based on its type
    void resize(int w, int h)
    {
        switch (type)
        {
        case Default:
            break; // No additional setup for default type
        case Texture:
            resetTextureAsTarget(w, h, GL_RGBA8); // Setup for color texture
            break;
        case TextureHDR:
            resetTextureAsTarget(w, h, GL_RGB32F); // Setup for HDR texture
            break;
        case TextureDepth:
            resetTextureDepthAsTarget(w, h); // Setup for depth texture
            break;
        case Deferred:
            resetDeferredAsTarget(w, h); // Setup for deferred rendering
            break;
        }
    }

    // Creates a texture buffer and attaches it to the FBO
    void createTextureBuffer(int w, int h, GLenum texUnit,
        GLenum format, GLenum attachment, GLuint* pTexid = nullptr)
    {
        GLuint texid;

        // Activate the specified texture unit and generate a texture
        glActiveTexture(texUnit);
        glGenTextures(1, &texid);
        glBindTexture(GL_TEXTURE_2D, texid);
        glTexStorage2D(GL_TEXTURE_2D, 1, format, w, h);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        // Attach the texture to the framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texid, 0);

        if (pTexid)
            *pTexid = texid; // Return texture ID if a pointer is provided
    }

    // Creates a renderbuffer for depth storage and attaches it to the FBO
    void createDepthBuffer(int w, int h)
    {
        GLuint depthBuf;
        glGenRenderbuffers(1, &depthBuf);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);

        // Attach the depth buffer to the framebuffer
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);
    }

    // Configures the FBO for rendering to a color texture with a depth buffer
    void resetTextureAsTarget(int w, int h, GLenum format)
    {
        if (handle)
            glDeleteFramebuffers(1, &handle);

        if (pTexture)
            glDeleteTextures(1, pTexture);

        // Create and bind the framebuffer
        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);

        // Create a color buffer texture and attach it
        createTextureBuffer(w, h, GL_TEXTURE0, format, GL_COLOR_ATTACHMENT0, pTexture);

        // Create and attach a depth buffer
        createDepthBuffer(w, h);

        // Specify the draw buffers
        GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBuffers);

        // Unbind the framebuffer, and revert to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Check for errors
        GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error(std::string("Error ") +
                std::to_string(result) +
                ": Framebuffer is not complete.\n");
    }

    void resetTextureDepthAsTarget(int w, int h)
    {
        GLuint texture = 0;

        if (handle)
            glDeleteFramebuffers(1, &handle);

        if (pTexture)
            glDeleteTextures(1, pTexture);

        GLfloat border[] = { 1.0f, 0.0f,0.0f,0.0f };

        // The depth buffer texture
        glGenTextures(1, pTexture ? pTexture : &texture);
        glBindTexture(GL_TEXTURE_2D, pTexture ? *pTexture : texture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, w, h);
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

        // Unbind the framebuffer, and revert to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Check for errors
        GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error(std::string("Error ") +
                std::to_string(result) +
                ": Framebuffer is not complete.\n");
    }

    // Configures the FBO for deferred rendering
    void resetDeferredAsTarget(int w, int h)
    {
        if (handle)
            glDeleteFramebuffers(1, &handle);

        // Create and bind the framebuffer
        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);

        // Create textures for different deferred rendering passes
        createTextureBuffer(w, h, GL_TEXTURE0, GL_RGB32F, GL_COLOR_ATTACHMENT0);
        createTextureBuffer(w, h, GL_TEXTURE1, GL_RGB32F, GL_COLOR_ATTACHMENT1);
        createTextureBuffer(w, h, GL_TEXTURE2, GL_RGB8, GL_COLOR_ATTACHMENT2);

        // Create and attach a depth buffer
        createDepthBuffer(w, h);

        // Define an array of buffers into which outputs from 
        // the fragment shader data will be written
        GLenum drawBuffers[] = { GL_NONE, GL_COLOR_ATTACHMENT0,
                                GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(4, drawBuffers);

        // Unbind the framebuffer, and revert to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Check for errors
        GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error(std::string("Error ") +
                std::to_string(result) +
                ": Framebuffer is not complete.\n");
    }
};

#endif // FBO_H