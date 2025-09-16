
#include "pch.hpp"
#include "Scene.h"

// For loading textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


//
    // This function utilizes the stb_image library for loading image files and OpenGL 
    // for creating and handling the texture.
    // 
    // GLuint: The function returns a GLuint, which is a type used by OpenGL 
    //          to represent an identifier for the texture. This value is used 
    //          later to refer to the texture in rendering operations.
    // const char* filename: This parameter takes a string representing the file path 
    //          of the image from which the texture is to be loaded.
    //
GLuint loadTexture(const char* filename)
{
    // Declare variables for the image's width, height, and the number of bytes per pixel
    int width, height, bytesPerPix;

    // Set the stb_image library to flip images vertically upon loading
    // This is because OpenGL has its texture origin at the bottom-left, 
    // while many image formats (like PNG) have their origin at the top-left.
    stbi_set_flip_vertically_on_load(true);

    // Load the image data from the file using stb_image's stbi_load function
    // This will fill in the width, height, and bytesPerPix (number of channels)
    // The '4' argument tells stb_image to load the image as RGBA (4 channels).
    unsigned char* data = stbi_load(filename, &width, &height, &bytesPerPix, 4);

    // If the image fails to load (data is null), throw an exception with the filename
    if (!data)
        throw std::runtime_error(std::string("file ") + filename + " not found.");

    GLuint tex = 0; // The texture ID

    // Generate a new texture ID using OpenGL
    glGenTextures(1, &tex);

    // Bind the generated texture ID to the GL_TEXTURE_2D target
    // This makes the texture the current active texture for subsequent texture operations
    glBindTexture(GL_TEXTURE_2D, tex);

    // Allocate storage for the texture (no data uploaded yet)
    // Internal format is RGBA8 (8 bits per channel, 4 channels: Red, Green, Blue, Alpha)
    // The texture dimensions are specified by 'width' and 'height'
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);

    // Upload the image data to the GPU as the content for the texture
    // The image data is in RGBA format, and each pixel is represented as an unsigned byte
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Set texture parameters:
    // GL_TEXTURE_MAG_FILTER: Controls how textures are sampled when they are magnified (scaled up)
    // GL_LINEAR: Linear interpolation (smooth, blending between texels)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // GL_TEXTURE_MIN_FILTER: Controls how textures are sampled when they are minified (scaled down)
    // GL_NEAREST: Set the minification filter to use linear filtering with mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Generate mipmaps for the texture
    glGenerateMipmap(GL_TEXTURE_2D);

    // Free the memory allocated by stb_image for the image data
    // The data is now on the GPU, so it is no longer needed in CPU memory
    stbi_image_free(data);

    // Return the texture ID to the caller for future reference in rendering operations
    return tex;
}

//
// The loadHdrCubeMap function generates and configures a cube map texture using HDR image files.
GLuint loadHdrCubeMap(const std::initializer_list<const char*> filenames)
{
    GLuint texID;

    // Generate a texture ID for the cube map
    glGenTextures(1, &texID);

    // Bind the texture as a cube map
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    GLuint i = 0;
    // Iterate through the provided file names
    for (const char* filename : filenames)
    {
        GLint w, h; // Variables to store the width and height of the texture

        // Load the HDR image data for the current file
        float* data = stbi_loadf(filename, &w, &h, NULL, 3);

        if (!data) // Throw an exception if the file cannot be loaded
            throw std::runtime_error(std::string("file ") + filename + " not found.");

        if (i == 0) // Allocate immutable storage for the whole cube map texture
            glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGB32F, w, h);

        // Upload the image data to the appropriate cube map face
        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, data);

        // Free the image data after uploading it to the GPU
        stbi_image_free(data);

        i++;
    }

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Return the ID of the generated cube map texture
    return texID;
}

//
// Constructor for Scene::Texture::Texture class. It initializes a texture object 
// with either a single 2D texture or a cube map texture.
//
// filenames: An initializer list containing file paths to the texture(s).
// channel: The OpenGL texture channel to bind the texture to(default is GL_TEXTURE0).
//
Scene::Texture::Texture(const std::initializer_list<const char*> filenames, GLuint channel /*= GL_TEXTURE0*/)
    : id{ 0 }, channel{ channel }, target{ 0 }
{
    if (filenames.size() == 1)
    {
        id = loadTexture(*filenames.begin());
        target = GL_TEXTURE_2D;
    }
    else if (filenames.size() == 6)
    {
        id = loadHdrCubeMap(filenames);
        target = GL_TEXTURE_CUBE_MAP;
    }
}

//
// Setting up the necessary OpenGL buffers and vertex attributes 
// for a 3D object. It handles different types of mesh data (positions, normals,
// textures, tangents) and organizes them in OpenGL buffers, which are then linked
// to a VAO for efficient rendering. Each type of mesh data (e.g., positions,
// normals, texCoords) is stored in a different buffer, and the VAO keeps track
// of how to use these buffers during rendering.
//
void Scene::Pass::Object::setBuffers(const char* meshName)
{
    // Load the mesh data (vertices, indices, normals, etc.) from the given mesh file
    TriangleMesh mesh(meshName);

    // Check if the mesh has valid data for indices, points (vertices), and normals.
    if (mesh.indices.empty() || mesh.points.empty() || mesh.normals.empty())
        return;

    // Store the number of indices.
    size = mesh.indices.size();

    // Declare OpenGL buffer object IDs for indices, positions, normals, texCoords, and tangents
    GLuint indexBuf = 0, posBuf = 0, normBuf = 0, tcBuf = 0, tangentBuf = 0;

    // Create and bind a buffer for the index data (element array buffer)
    glGenBuffers(1, &indexBuf);
    buffers.push_back(indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(GLuint), mesh.indices.data(), GL_STATIC_DRAW);

    // Create and bind a buffer for the vertex positions (array buffer)
    glGenBuffers(1, &posBuf);
    buffers.push_back(posBuf);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf);
    glBufferData(GL_ARRAY_BUFFER, mesh.points.size() * sizeof(GLfloat), mesh.points.data(), GL_STATIC_DRAW);

    // Create and bind a buffer for the normal data (array buffer)
    glGenBuffers(1, &normBuf);
    buffers.push_back(normBuf);
    glBindBuffer(GL_ARRAY_BUFFER, normBuf);
    glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(GLfloat), mesh.normals.data(), GL_STATIC_DRAW);

    // If the mesh contains texture coordinates, create and bind a buffer for them
    if (!mesh.texCoords.empty())
    {
        glGenBuffers(1, &tcBuf);
        buffers.push_back(tcBuf);
        glBindBuffer(GL_ARRAY_BUFFER, tcBuf);
        glBufferData(GL_ARRAY_BUFFER, mesh.texCoords.size() * sizeof(GLfloat), mesh.texCoords.data(), GL_STATIC_DRAW);
    }

    // If the mesh contains tangent data, create and bind a buffer for tangents
    if (!mesh.tangents.empty())
    {
        glGenBuffers(1, &tangentBuf);
        buffers.push_back(tangentBuf);
        glBindBuffer(GL_ARRAY_BUFFER, tangentBuf);
        glBufferData(GL_ARRAY_BUFFER, mesh.tangents.size() * sizeof(GLfloat), mesh.tangents.data(), GL_STATIC_DRAW);
    }

    // Generate and bind a Vertex Array Object (VAO) to store the vertex attribute bindings
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Bind the index buffer (element array buffer) for use in drawing
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);

    // Set up the vertex position attribute (3D coordinates: x, y, z)
    glBindBuffer(GL_ARRAY_BUFFER, posBuf);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Set up the vertex normal attribute (3D normal vector: nx, ny, nz)
    glBindBuffer(GL_ARRAY_BUFFER, normBuf);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // If texture coordinates are available, set up the texture coordinate attribute (2D: u, v)
    if (!mesh.texCoords.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, tcBuf);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(2);
    }

    // If tangents are available, set up the tangent attribute (4D vector: tx, ty, tz, tw)
    if (!mesh.tangents.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, tangentBuf);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(3);
    }

    // Unbind the VAO to finish setting up the vertex buffers and attributes
    glBindVertexArray(0);
}

//
// The function orchestrates the rendering process for a scene by iterating through a series 
// of rendering passes. This function handles various stages of the rendering pipeline, 
// including framebuffer setup, shader configuration, texture binding, lighting, 
// and object rendering, making it versatile for multi-pass rendering in OpenGL.
//
void Scene::render()
{
    static GLuint currentHandle = 0xFFFFFF;

    int i = 0;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Iterate through all rendering passes
    for (const Scene::Pass& pass : passes)
    {
        // Set the framebuffer object (FBO) as the render target

        // Check if framebuffer is already bound to improve performance 
        // by avoiding unnecessary bindings
        if (currentHandle != pass.fbo.handle)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo.handle);
            currentHandle = pass.fbo.handle;
        }

        // Set the pass number and other parameters for rendering if any
        shader.use();
        shader.setUniform("Pass", i++);
        if (pass.setUniformCallback)
            pass.setUniformCallback(shader);
        shader.disuse();

        // Configure the viewport if specified
        if (pass.viewport.size() == 4)
            glViewport(pass.viewport[0], pass.viewport[1], pass.viewport[2], pass.viewport[3]);

        // Clear the color buffer if a clear color is specified
        //if (pass.clearColor.size() == 4)
        //{
        //    //std::cout << "Clearing class" << std::endl;
        //    glClearColor(pass.clearColor[0], pass.clearColor[1], pass.clearColor[2], pass.clearColor[3]);
        //    glClear(GL_COLOR_BUFFER_BIT);
        //}

        // Set and clear the depth buffer
        if (pass.enableDepthBuffer.size() > 0)
        {
            GLboolean enableDepthTest;
            glGetBooleanv(GL_DEPTH_TEST, &enableDepthTest);

            // Enable or disable depth testing based on the pass settings
            if (enableDepthTest)
                if (pass.enableDepthBuffer[0])
                    glClear(GL_DEPTH_BUFFER_BIT);
                else
                {
                    glDisable(GL_DEPTH_TEST);
                    glClear(GL_DEPTH_BUFFER_BIT);
                }
            else
                if (pass.enableDepthBuffer[0])
                {
                    glEnable(GL_DEPTH_TEST);
                    glClear(GL_DEPTH_BUFFER_BIT);
                }
        }

        // Bind textures for the current pass
        for (const Texture& info : pass.textures)
            if (info.id)
            {
                glActiveTexture(info.channel);
                glBindTexture(info.target, info.id);
            }

        // Configure lighting for the pass
        for (int idx = 0; idx < pass.lights.size(); ++idx)
            pass.lights[idx].setUniforms(&shader, idx);


        glm::mat4 V = glm::mat4(1.0f); // View matrix
        glm::mat4 P = glm::mat4(1.0f); // Projection matrix

        // If a single camera is used, set its uniforms and update matrices
        if (pass.cameras.size() == 1)
        {
            pass.cameras[0].setUniforms(&shader);

            if (pass.viewport.size() == 4 && pass.viewport[3])
            {
                V = pass.cameras[0].getLookAt();
                P = pass.cameras[0].getPerspective((float)pass.viewport[2] / pass.viewport[3]);
            }
        }

        // Set the view and projection matrices in the shader
        shader.use();
        shader.setUniform("V", V);
        shader.setUniform("P", P);
        shader.disuse();

        // Render objects for the pass
        for (const Scene::Pass::Object& object : pass.objects)
            if (object.isVisible && !object.instances.empty())
            {
                for (const Scene::Pass::Object::Instance& instance : object.instances)
                    if (instance.isVisible)
                    {
                        // Set material properties for the instance
                        instance.material.setUniforms(&shader);

                        // Set the model transformation matrix and render the instance
                        shader.use();
                        shader.setUniform("M", instance.modeltransform);

                        glBindVertexArray(object.vao);
                        glDrawElements(GL_TRIANGLES, (GLsizei)object.size, GL_UNSIGNED_INT, 0);
                        glBindVertexArray(0);

                        shader.disuse();
                    }
            }

        // Ensure all OpenGL commands are finished before moving to the next pass
        glFinish();
    }
}
