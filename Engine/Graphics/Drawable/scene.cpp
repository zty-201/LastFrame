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
GLuint loadTexture(const char* filename)
{
	// Declare variables for the image's width, height, and the number of bytes per pixel
	int width, height, bytesPerPix;

	// Set the stb_image library to flip images vertically upon loading
	// This is because OpenGL has its texture origin at the bottom-left, 
	// while many image formats (like PNG) have their origin at the top-left.
	stbi_set_flip_vertically_on_load(true);


	//Load using cross-platform compatible function.
	std::vector<unsigned char> texture_data = EngineFramework::AssetManagerSystem::GetDataFromFile(std::filesystem::path(filename));

	// Load the image data from the buffer using stb_image's stbi_load function
	// This will fill in the width, height, and bytesPerPix (number of channels)
	// The '4' argument tells stb_image to load the image as RGBA (4 channels).
	unsigned char* data = stbi_load_from_memory(texture_data.data(), static_cast<int>(texture_data.size()), &width, &height, &bytesPerPix, 4);

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
GLuint loadHdrCubeMap(const std::array<std::string, 6>& filenames)
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

	// Return the ID of the generated cube map texture
	return texID;
}


// Constructor for creating texture from a file (2D)
// channel: The OpenGL texture channel to bind the texture to(default is GL_TEXTURE0).
Scene::Texture::Texture(const std::string& file, GLuint channel)
	: id{ 0 }, channel{ channel }, target{ 0 }
{
	id = loadTexture(file.c_str());
	target = GL_TEXTURE_2D;
}


// Constructor for creating textures from files (cubemap)
// channel: The OpenGL texture channel to bind the texture to(default is GL_TEXTURE0).
Scene::Texture::Texture(const std::array<std::string, 6>& filenames, GLuint channel)
	: id{ 0 }, channel{ channel }, target{ 0 }
{
	id = loadHdrCubeMap(filenames);
	target = GL_TEXTURE_CUBE_MAP;
}
