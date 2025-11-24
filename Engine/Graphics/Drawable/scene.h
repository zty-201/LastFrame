/**
 * @file scene.h
 * @brief Defines the Scene class and related utilities for managing 3D scenes.
 *
 * This file contains the `Scene` class, which encapsulates rendering passes, objects,
 * textures, and cameras for a 3D graphics application. It also includes utility functions
 * for setting up textures and cube maps, as well as various constants for default colors,
 * dimensions, and visibility states.
 *
 * Key components:
 * - `Scene::Texture`: A structure for managing OpenGL textures, including 2D and cube maps.
 * - Utility functions: `setupTexture` and `setupHdrCubeMap` for loading textures.
 * - Constants: Definitions for default colors, transforms, and common 3D primitives.
 *
 * @author Zhang TongYan
 * @date Sept, 2025
 */
#ifndef SCENE_H
#define SCENE_H

#include "ShaderProgram.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"
#include "TextureComponent.h"
#include "NormalMapComponent.h"
#include "AnimationComponent.h"

#include "FBO.h"
#include "Light.h"
#include "Camera.h"
#include "Mesh.h"

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
GLuint setupTexture(const char* filename);
GLuint setupHdrCubeMap(const std::array<std::string, 6>&);

//
// The main Scene class encapsulates rendering passes, objects, textures, and cameras
//
struct Scene
{
    //float aspect;
    //// Structure to represent a texture used in the scene
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

        // Constructor for creating textures from a file (2D)
        Texture(const std::string& file, GLuint channel = GL_TEXTURE0);

        // Constructor for creating textures from files (cubemap)
        Texture(const std::array<std::string, 6>& filenames, GLuint channel = GL_TEXTURE0);

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

    glm::vec3 fogColor;
    float fogMinDist;
    float fogMaxDist;
    int shadingModel;

   
};

#endif