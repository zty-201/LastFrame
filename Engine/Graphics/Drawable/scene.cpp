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
#include "scene.h"
#include "AssetManager.hpp"
#include "gli/gli.hpp"
#ifdef PLATFORM_WINDOWS //For Compression of image to etc2 / dxt5
#include "compressonator.h"
#endif
// For loading textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//
	// This function utilizes the stb_image or gli library for loading image files and OpenGL 
	// for creating and handling the texture.
	// 
	// GLuint: The function returns a GLuint, which is a type used by OpenGL 
	//          to represent an identifier for the texture. This value is used 
	//          later to refer to the texture in rendering operations.
	// const char* filename: This parameter takes a string representing the file path 
	//          of the image from which the texture is to be loaded.
	//
GLuint setupTexture(const char* filename, bool isSRGB = true)
{
	/*
		Standard Image Formats (png, jpg) --> stbi
		Compressed Texture (dxt5, etc2) --> gli
	*/
	LOGD("Loading Texture");
	//For compressed texture loading, i believe it support isSRGB and cubemaps.
	//Code taken from GLI documentation
	constexpr std::string compressed_extension[]{ ".dds", ".ktx", ".ktx2" };
	if (std::find(std::begin(compressed_extension), std::end(compressed_extension), std::filesystem::path(filename).extension().string()) != std::end(compressed_extension))
	{
		std::string buffer = EngineFramework::AssetManagerSystem::GetStringFromFile(filename);
		gli::texture Texture = gli::load(buffer.data(), buffer.size()); //Self-Managed via RAII
		if(Texture.empty()) throw std::runtime_error(std::string("Failed to load texture: ") + filename);

		//There's no need to flip texture, since it should already flipped during compression process.

		gli::gl GL(gli::gl::PROFILE_ES30);
		gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
		GLenum Target = GL.translate(Texture.target());

		GLuint TextureName = 0;
		glGenTextures(1, &TextureName);
		glBindTexture(Target, TextureName);
		glTexParameteri(Target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);

		glm::tvec3<GLsizei> const Extent(Texture.extent());
		GLsizei const FaceTotal = static_cast<GLsizei>(Texture.layers() * Texture.faces());

		switch (Texture.target())
		{
		case gli::TARGET_1D_ARRAY:
		case gli::TARGET_2D:
		case gli::TARGET_CUBE:
			glTexStorage2D(
				Target, static_cast<GLint>(Texture.levels()), Format.Internal,
				Extent.x, Texture.target() == gli::TARGET_2D ? Extent.y : FaceTotal);
			break;
		case gli::TARGET_2D_ARRAY:
		case gli::TARGET_3D:
		case gli::TARGET_CUBE_ARRAY:
			glTexStorage3D(
				Target, static_cast<GLint>(Texture.levels()), Format.Internal,
				Extent.x, Extent.y,
				Texture.target() == gli::TARGET_3D ? Extent.z : FaceTotal);
			break;
		default:
			assert(0);
			break;
		}

		for (std::size_t Layer = 0; Layer < Texture.layers(); ++Layer)
			for (std::size_t Face = 0; Face < Texture.faces(); ++Face)
				for (std::size_t Level = 0; Level < Texture.levels(); ++Level)
				{
					GLsizei const LayerGL = static_cast<GLsizei>(Layer);
					glm::tvec3<GLsizei> Extent(Texture.extent(Level));
					Target = gli::is_target_cube(Texture.target())
						? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face)
						: Target;

					switch (Texture.target())
					{
					case gli::TARGET_1D_ARRAY:
					case gli::TARGET_2D:
					case gli::TARGET_CUBE:
						if (gli::is_compressed(Texture.format()))
							glCompressedTexSubImage2D(
								Target, static_cast<GLint>(Level),
								0, 0,
								Extent.x,
								Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
								Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
								Texture.data(Layer, Face, Level));
						else
							glTexSubImage2D(
								Target, static_cast<GLint>(Level),
								0, 0,
								Extent.x,
								Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
								Format.External, Format.Type,
								Texture.data(Layer, Face, Level));
						break;
					case gli::TARGET_2D_ARRAY:
					case gli::TARGET_3D:
					case gli::TARGET_CUBE_ARRAY:
						if (gli::is_compressed(Texture.format()))
							glCompressedTexSubImage3D(
								Target, static_cast<GLint>(Level),
								0, 0, 0,
								Extent.x, Extent.y,
								Texture.target() == gli::TARGET_3D ? Extent.z : LayerGL,
								Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
								Texture.data(Layer, Face, Level));
						else
							glTexSubImage3D(
								Target, static_cast<GLint>(Level),
								0, 0, 0,
								Extent.x, Extent.y,
								Texture.target() == gli::TARGET_3D ? Extent.z : LayerGL,
								Format.External, Format.Type,
								Texture.data(Layer, Face, Level));
						break;
					default: assert(0); break;
					}
				}
		return TextureName;
	}
	//For Standard image loading
	else
	{
		int width, height, bytesPerPix;
		stbi_set_flip_vertically_on_load(true);

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
		glBindTexture(GL_TEXTURE_2D, texID);

		GLint internalFormat = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;

		// Allocates AND uploads Level 0 in one go
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		// Automatically generates levels 1...N
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);

		return texID;
	}
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
	if (file.find("Albedo") != std::string::npos){
		id = setupTexture(file.c_str(), true);
	}
	else {
		id = setupTexture(file.c_str(), false);
	}
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

#ifdef PLATFORM_WINDOWS
std::vector<unsigned char> Scene::Texture::CompressTexture(const char* filename, bool convertBC3)
{
	int width, height, bytesPerPix;
	stbi_set_flip_vertically_on_load(true);

	//Load raw data
	std::vector<unsigned char> texture_data = EngineFramework::AssetManagerSystem::GetDataFromFile(std::filesystem::path(filename));
	unsigned char* data = stbi_load_from_memory(
		texture_data.data(),
		static_cast<int>(texture_data.size()),
		&width, &height, &bytesPerPix, 4
	);

	if (!data) return {}; //Return empty to show unable to compress, since unable to load.

	//Handle SRGB correctly, and set the correct formats.
	bool isSRGB = false;
	if (std::string_view(filename).find("Albedo") != std::string::npos) isSRGB = true;
	CMP_FORMAT input_format = CMP_FORMAT_RGBA_8888; //Both normal and srgb have 4 channels of 8 bits each.
	CMP_FORMAT output_format{};
	if (convertBC3)
	{
		output_format = CMP_FORMAT_BC3;
	}
	else if (isSRGB)
	{
		output_format = CMP_FORMAT_ETC2_SRGBA;
	}
	else output_format = CMP_FORMAT_ETC2_RGBA;

	//==========================
	// Load Source Texture
	//==========================
	CMP_Texture srcTexture{};
	srcTexture.dwSize = sizeof(srcTexture);
	srcTexture.dwWidth = width;
	srcTexture.dwHeight = height;
	srcTexture.dwPitch = width*4; //Probably the size of a row in bytes.
	srcTexture.format = input_format; //Both normal and srgb have 4 channels of 8 bits each.
	srcTexture.dwDataSize = width*height*4; //Size of each pixel is 4 * 8 bits = 4 bytes. Size of entire grid = w * h * 4
	srcTexture.pData = data;

	//===================================
	// Initialize Compressed Destination
	//===================================
	CMP_Texture destTexture{};
	destTexture.dwSize = sizeof(destTexture);
	destTexture.dwWidth = srcTexture.dwWidth;
	destTexture.dwHeight = srcTexture.dwHeight;
	destTexture.format = output_format;
	destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
	destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);

	//==========================
	// Set Compression Options
	//==========================
	CMP_CompressOptions options = { 0 };
	options.dwSize = sizeof(options);
	options.dwnumThreads = 8;
	options.bUseCGCompress = true;
	//Max quality, lower speed.
	options.fquality = 1.0f;
	// Recommended for BC3 quality
	options.bUseRefinementSteps = true;
	options.nRefinementSteps = 1;

	//Compression type
	options.useSRGBFrames = isSRGB;  // Ensures gamma-correct processing
	options.SourceFormat = input_format;
	options.DestFormat = output_format;

	//ETC2 Cli --> CompressonatorCLI.exe -fd ETC2 -UseGPUDecompress  input.png output.ktx
	//BC3 Cli --> CompressonatorCLI.exe -fd BC3 -quality 1.0 -threads 8 -refine 1 -srgba input.png output.dds
	//BC3 Cli --> CompressonatorCLI.exe -fd BC3 -quality 1.0 -threads 8 -refine 1 -srgba "C:\input_folder\*.png" "C:\output_folder"
	//BC3 or ETC2_RGBA  -ff <ext>,…,<ext> for png,jpg etc. -fx <ext> for dds,ktx etc. 
	/*
		-UseSRGBFrames -EncodeWith GPU   //Only if srgb
		-fd ETC2_RGBA or BC3
		-RefineSteps 2 -CompressionSpeed 0
		-ff png,jpg
		CompressonatorCLI.exe -fd ETC2_RGBA -EncodeWith GPU -ff png,jpg -UseSRGBFrames -RefineSteps 2 -CompressionSpeed 0 "../" "../"
	*/

	//Leave out -srgba if not srgba.

	
	//==========================
	// Compress Texture
	//==========================
	CMP_ERROR   cmp_status;
	cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, NULL);
	if (cmp_status != CMP_OK)
	{
		free(srcTexture.pData);
		free(destTexture.pData);
		return {};
	}

	//==========================
	// Save Compressed Testure
	//==========================
	

	stbi_image_free(srcTexture.pData);
	free(destTexture.pData);
	return {}; //Temporary.
}
#endif