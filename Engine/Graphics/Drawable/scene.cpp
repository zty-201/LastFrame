/**
 * @file scene.cpp
 * @brief Implementation of texture loading and management for 3D scenes.
 *
 * This file contains functions and constructors for setting up and managing textures
 * in a 3D graphics application. It utilizes the stb_image library for loading image files
 * and OpenGL for creating and handling textures. The file supports both 2D textures and
 * cube map textures, including HDR textures.
 *
 * Key functionalities include:
 * - Loading textures from files or memory.
 * - Generating OpenGL texture objects for 2D and cube map textures.
 * - Configuring texture parameters and mipmaps.
 *
 * @author Zhang TongYan
 * @date Sept, 2025
 */
#include "pch.hpp"
#include "Scene.h"
// For loading textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "AssetManager.hpp"
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
GLuint setupTexture(const char* filename)
{
	int width, height, bytesPerPix;
	stbi_set_flip_vertically_on_load(true);

	//Load using cross-platform compatible function.
	std::vector<unsigned char> texture_data = EngineFramework::AssetManagerSystem::GetDataFromFile(std::filesystem::path(filename));
	unsigned char* data = stbi_load_from_memory(
		texture_data.data(),
		static_cast<int>(texture_data.size()),
		&width, &height, &bytesPerPix, 4
	);

	if (!data)
		throw std::runtime_error(std::string("Failed to load texture: ") + filename);

	GLuint texID = 0;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);  // Bind temporarily for setup

	// Allocate storage
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);

	// Upload data
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

	// Set parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	// Generate mipmaps
	glGenerateMipmap(GL_TEXTURE_2D);

	// Unbind after creation
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	return texID;  // Return unbound texture ID

}

//
// The loadHdrCubeMap function generates and configures a cube map texture using HDR image files.
GLuint setupHdrCubeMap(const std::array<std::string, 6>& filenames)
{
	GLuint texID;

	// Generate a texture ID for the cube map
	glGenTextures(1, &texID);

	// Bind the texture as a cube map
	glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

	// Iterate through the provided file names
	for (GLuint i = 0; i<6; i++)
	{
		GLint w, h; // Variables to store the width and height of the texture

		// Load the HDR image data for the current file
		//Load using cross-platform compatible function.
		std::vector<unsigned char> hdr_data = EngineFramework::AssetManagerSystem::GetDataFromFile(std::filesystem::path(filenames[i]));

		float* data = stbi_loadf_from_memory(hdr_data.data(), static_cast<int>(hdr_data.size()), &w, &h, NULL, 3);

		if (!data) // Throw an exception if the file cannot be loaded
			throw std::runtime_error(std::string("file ") + filenames[i] + " not found.");

		if (i == 0) // Allocate immutable storage for the whole cube map texture
			glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGB32F, w, h);

		// Upload the image data to the appropriate cube map face
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, data);

		// Free the image data after uploading it to the GPU
		stbi_image_free(data);
	}

	// Set texture filtering parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// Return the ID of the generated cube map texture
	return texID;
}


// Constructor for creating texture from a file (2D)
// channel: The OpenGL texture channel to bind the texture to(default is GL_TEXTURE0).
Scene::Texture::Texture(const std::string& file, GLuint channel)
	: id{ 0 }, channel{ channel }, target{ 0 }
{
	id = setupTexture(file.c_str());
	target = GL_TEXTURE_2D;
}


// Constructor for creating textures from files (cubemap)
// channel: The OpenGL texture channel to bind the texture to(default is GL_TEXTURE0).
Scene::Texture::Texture(const std::array<std::string, 6>& filenames, GLuint channel)
	: id{ 0 }, channel{ channel }, target{ 0 }
{
	id = setupHdrCubeMap(filenames);
	target = GL_TEXTURE_CUBE_MAP;
}
