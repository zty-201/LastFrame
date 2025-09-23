#ifndef SCENE_H
#define SCENE_H

#include "ShaderProgram.h"
#include "FBO.h"
#include "Mesh.h"
#include "Material.h"
#include "Light.h"
#include "Camera.h"


// Color and state definitions
#define BLACK           { 0.0f, 0.0f, 0.0f, 0.0f }
#define GREY            { 0.5f, 0.5f, 0.5f, 0.0f }
#define WHITE           { 1.0f, 1.0f, 1.0f, 0.0f }

#define DEFAULT         { }

// Mesh names for common 3D primitives
#define QUAD    "Quad"
#define PLANE   "Plane"
#define TORUS   "Torus"
#define CUBE    "Cube"
#define SPHERE  "Sphere"
#define TEAPOT  "Teapot"
#define SKYBOX  "SkyBox"

// Default dimensions and transforms
#define WIDTH   800
#define HEIGHT  600

#define NOTRANSFORM  glm::mat4(1.0f)

// Visibility and state controls
#define VISIBLE     true
#define INVISIBLE   false
#define ENABLE      true
#define DISABLE     false

// Functions for loading textures
GLuint loadTexture(const char* filename);
GLuint loadHdrCubeMap(std::initializer_list<const char*> filenames);

//
// The main Scene class encapsulates rendering passes, objects, textures, and cameras
//
struct Scene
{
    // Structure to represent a texture used in the scene
    struct Texture
    {
        GLuint id;        // Texture ID assigned by OpenGL
        GLuint channel;   // OpenGL texture channel (e.g., GL_TEXTURE0)
        GLuint target;    // Texture target (e.g., GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP)

        // Constructor for directly initializing texture properties
        Texture(GLuint id = 0, GLuint channel = GL_TEXTURE0, GLuint target = GL_TEXTURE_2D)
            : id{ id }, channel{ channel }, target{ target }
        {
        }

        // Constructor for creating textures from files (2D or cube maps)
        Texture(const std::initializer_list<const char*> filenames, GLuint channel = GL_TEXTURE0);

        // Destructor to free GPU resources
        ~Texture()
        {
            if (id != 0)
            {
                glDeleteTextures(1, &id);
                id = 0; // Prevent dangling.
            }
        }

        //Rule of 3, ensure it cannot be copied so texture destructor has no issues with double or permature deletion
        Texture(const Texture& rhs) = delete;
        Texture& operator=(const Texture& rhs) = delete;
    };

    // Structure representing a rendering pass
    struct Pass
    {
        // Structure representing an object in the scene
        struct Object
        {
            // Represents an instance of an object with its material and transform
            struct Instance
            {
                Material material;             // Material properties of the instance
                glm::mat4 modeltransform;      // **FIX: Changed from const glm::mat4& to glm::mat4**
                bool isVisible;                // Visibility flag

                // Constructor for initializing an instance
                Instance(const Material& material = { },
                    const glm::mat4& modeltransform = NOTRANSFORM,
                    bool isVisible = VISIBLE)
                    :
                    material{ material },
                    modeltransform{ modeltransform },
                    isVisible{ isVisible }
                {
                }
            };

            GLuint vao;                     // Vertex Array Object ID for the object
            std::vector<GLuint> buffers;    // Vertex buffer objects
            size_t size;                    // Number of indices in the object's index buffer
            std::vector<Instance> instances; // Instances of the object
            bool isVisible;                 // Object visibility flag

            // Constructor for creating an object with a single instance
            Object(const char* meshName,
                const Material& material = { },
                const glm::mat4& modeltransform = NOTRANSFORM,
                bool isVisible = VISIBLE)
                :
                instances{ },
                isVisible{ isVisible },
                vao{ 0 },
                buffers{ },
                size{ 0 }
            {
                instances.push_back({ material, modeltransform, VISIBLE });
                setBuffers(meshName);
            }

            // Constructor for creating an object with multiple instances
            Object(const char* meshName,
                const std::initializer_list<Instance>& instances,
                bool isVisible = VISIBLE)
                :
                instances{ instances },
                isVisible{ isVisible },
                vao{ 0 },
                buffers{ },
                size{ 0 }
            {
                setBuffers(meshName);
            }

        private:
            // Sets up OpenGL buffers and VAO for the object's mesh
            void setBuffers(const char* meshName);
        };

        using SetUniformCallback = std::function<void(ShaderProgram& shader)>;

        FBO fbo;                           // Framebuffer Object for the pass
        std::vector<int> viewport;         // Viewport dimensions
        GLenum depthMode;                  // Depth function mode flags
        std::vector<Object> objects;       // Objects rendered in this pass
        std::vector<Camera> cameras;       // Cameras used in this pass
        std::vector<Light> lights;         // Lights affecting this pass
        std::vector<std::shared_ptr<Texture>> textures;     // Textures used in this pass
        SetUniformCallback setUniformCallback; // Optional callback for setting uniforms

        // Constructor for older frameworks (single depth buffer flag)
        Pass(
            const FBO& fbo = { },
            const std::initializer_list<int>& viewport = { },
            GLenum depthFunction = GL_LESS,
            const std::initializer_list<Object>& objects = { },
            const std::initializer_list<Camera>& cameras = { },
            const std::initializer_list<Light>& lights = { },
            const std::initializer_list<std::shared_ptr<Texture>> textures = { },
            SetUniformCallback setUniformCallback = nullptr
        )
            :
            fbo{ fbo },
            viewport{ viewport },
            depthMode{ depthFunction },
            objects{ objects },
            cameras{ cameras },
            lights{ lights },
            textures{ textures },
            setUniformCallback{ setUniformCallback }
        {
            //this->enableDepthBuffer.push_back(enableDepthBuffer);
        }

        // Resizes the framebuffer and viewport for the pass
        void resize(int width, int height)
        {
            viewport[2] = width;
            viewport[3] = height;
            fbo.resize(width, height);
        }

        //Changes framebuffer.
        void SetFBO(const FBO& fbo_to_set = { })
        {
            fbo = fbo_to_set;
        }
    };

    ShaderProgram shader;          // Shader program for the scene
    std::vector<Pass> passes; // Rendering passes in the scene

    // Constructors for the Scene class
    Scene(
        const char* sv = "",
        const char* sf = "",
        const std::initializer_list<Pass>& passes = { }
    )
        :
        shader(sv, sf),
        passes{ passes }
    {
    }

    Scene(
        GLuint shaderHandle = 0,
        const std::initializer_list<Pass>& passes = { }
    )
        :
        shader(shaderHandle),
        passes{ passes }
    {
    }

    // Main render function for the scene
    void render();

    // Resizes all passes in the scene
    void resize(int width, int height)
    {
        for (Scene::Pass& pass : passes)
            pass.resize(width, height);
    }

    // Updates camera types for all passes
    void camerasSetType(CameraType cameraType)
    {
        for (Scene::Pass& pass : passes)
            for (Camera& camera : pass.cameras)
                camera.setType(cameraType);
    }

    void camerasSetPos_andTarget(glm::vec3 offset)
    {
        for (Scene::Pass& pass : passes)
            for (Camera& camera : pass.cameras)
                camera.Move_Cam_and_Target(offset);
    }



    // Updates camera orientation based on cursor movement
    void camerasOnCursor(double xoffset, double yoffset)
    {
        for (Scene::Pass& pass : passes)
            for (Camera& camera : pass.cameras)
                camera.onCursor(xoffset, yoffset);
    }

    // Updates light positions based on cursor movement
    void lightsOnCursor(double xoffset, double yoffset, ShaderProgram* pProgram = nullptr)
    {
        int i = 0;
        for (Scene::Pass& pass : passes)
            for (Light& light : pass.lights)
                light.onCursor(xoffset, yoffset, pProgram, i++);
    }

    // Updates camera zoom based on scroll input
    void camerasOnScroll(double xoffset, double yoffset)
    {
        for (Scene::Pass& pass : passes)
            for (Camera& camera : pass.cameras)
                camera.onScroll(xoffset, yoffset);
    }

    // Updates light properties based on scroll input
    void lightsOnScroll(double xoffset, double yoffset, ShaderProgram* pProgram = nullptr)
    {
        int i = 0;
        for (Scene::Pass& pass : passes)
            for (Light& light : pass.lights)
                light.onScroll(xoffset, yoffset, pProgram, i++);
    }
};

#endif