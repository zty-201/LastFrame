#ifndef TEXTURE_COMPONENT
#define TEXTURE_COMPONENT
struct TextureComponent
    {
        GLuint id;        // Texture ID assigned by OpenGL
        GLuint channel;   // OpenGL texture channel (e.g., GL_TEXTURE0)
        GLuint target;    // Texture target (e.g., GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP)

        // Constructor for directly initializing texture properties
        TextureComponent(GLuint id = 0, GLuint channel = GL_TEXTURE0, GLuint target = GL_TEXTURE_2D)
            : id{ id }, channel{ channel }, target{ target }
        {
        }

        // Constructor for creating textures from a file (2D)
        TextureComponent(const std::string& file, GLuint channel = GL_TEXTURE0);

        // Constructor for creating textures from files (cubemap)
        TextureComponent(const std::array<std::string, 6>& filenames, GLuint channel = GL_TEXTURE0);

        // Destructor to free GPU resources
        ~TextureComponent()
        {
            if (id != 0)
            {
                glDeleteTextures(1, &id);
                id = 0; // Prevent dangling.
            }
        }

        //Rule of 3, ensure it cannot be copied so texture destructor has no issues with double or permature deletion
        TextureComponent(const TextureComponent& rhs) = delete;
        TextureComponent& operator=(const TextureComponent& rhs) = delete;
    };
#endif // !TEXTURE_COMPONENT
