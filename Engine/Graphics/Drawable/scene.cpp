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
#include "gli/gli.hpp"

#include "scene.h"
#include "AssetManager.hpp"

 // For loading textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//For compressing textures
#include "compressonator.h"

namespace
{
#ifdef PLATFORM_WINDOWS
	enum class TextureType
	{
		BC_X, //For Windows
		ETC2 //For Android.
	};

	//RAII data containing compressed texture data.
	class CompressedTextureData
	{
	public:
		CompressedTextureData() = default;
		//For errors.
		CompressedTextureData(const std::string& error_str) noexcept
			: CompressedTextureData()
		{
			this->error_str = error_str;
		}

		CompressedTextureData(
			uint32_t                    width,
			uint32_t                    height,
			uint32_t                    mip_levels,
			CMP_FORMAT                  format,
			bool                        is_srgb,
			short                       channels,
			std::vector<std::byte>      data,
			std::optional<std::string>  error_str = std::nullopt
		) noexcept
			: width(width),
			height(height),
			mip_levels(mip_levels),
			format(format),
			is_srgb(is_srgb),
			channels(channels),
			data(std::move(data)),
			error_str(std::move(error_str))
		{
		}

		bool isValid() const noexcept { return !data.empty() && width > 0 && height > 0 && !error_str; }


	public:
		// Texture data
		uint32_t    width = 0;
		uint32_t    height = 0;
		uint32_t    mip_levels = 1;
		CMP_FORMAT  format = CMP_FORMAT_Unknown;
		bool        is_srgb = false;
		short       channels = 0;
		std::vector<std::byte> data;
		std::optional<std::string> error_str;
	};

	//RAII data containing uncompressed texture data loaded by stbi.
	class UnCompressedTextureData
	{
	public:
		UnCompressedTextureData() = default;
		UnCompressedTextureData(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels, bool is_srgb)
			: width{ width }, height{ height }, channels{ channels }, is_srgb{ is_srgb } {
			if (width > 0 && height > 0 && channels > 0)
			{
				this->data = std::vector<std::byte>(data, data + width * height * channels);
			}
		}

		bool isValid() const { return (width > 0 && height > 0 && channels > 0 && !data.empty()); }
	public:
		//RAII texture data. 
		std::vector<std::byte> data;
		uint32_t    width = 0;
		uint32_t    height = 0;
		uint32_t	channels = 0;
		bool is_srgb = false;
	};

	/*
		Returns compressed data of the texture filepath passed in.
		UnCompressedTextureData -> The loaded texture data. It should be flipped vertically.
		TextureType -> What type to compress texture to.
	*/
	CompressedTextureData CompressTexture(const UnCompressedTextureData& uncompressed_texture, TextureType compression_type)
	{
		if (!uncompressed_texture.isValid())
		{
			return CompressedTextureData("Invalid Texture Passed In");
		}

		/*
			Compression:
			- There are two types of compression, one for BCx (Windows) and one for ETC2 (Android).
			Among those, there are different formats for channels 1-4.

			is_srgb is defined as the color space and has the same uncompressed/compressed format as non-srgb.
			Thus, compress srgb the same way, as long as the gpu knows it's srgb when uploaded.
		*/

		/*
			Assign input and output formats.
		*/
		//==Input is assumed to always be 1 unsigned byte per channel (no floating pt).
		CMP_FORMAT input_format{};
		switch (uncompressed_texture.channels)
		{
		case 1:
			input_format = CMP_FORMAT_R_8;
			break;
		case 2:
			input_format = CMP_FORMAT_RG_8;
			break;
		case 3:
			input_format = CMP_FORMAT_RGB_888;
			break;
		case 4:
			input_format = CMP_FORMAT_RGBA_8888;
			break;
		}

		CMP_FORMAT output_format{};
		if (compression_type == TextureType::BC_X)
		{
			switch (uncompressed_texture.channels)
			{
			case 1:
				output_format = CMP_FORMAT_BC4;
				break;
			case 2:
				output_format = CMP_FORMAT_BC5;
				break;
			case 3:
				output_format = CMP_FORMAT_BC1;
				break;
			case 4:
				output_format = CMP_FORMAT_BC3;
				break;
			}
		}
		else if (compression_type == TextureType::ETC2)
		{
			switch (uncompressed_texture.channels)
			{
			case 1:
				return CompressedTextureData("1 Channel ETC2 unsupported");
				break;
			case 2:
				return CompressedTextureData("2 Channel ETC2 unsupported");
				break;
			case 3:
				output_format = CMP_FORMAT_ETC2_RGB;
				break;
			case 4:
				output_format = CMP_FORMAT_ETC2_RGBA;
				break;
			}
		}

		/*
			Setup source texture (input).
		*/
		CMP_Texture src_texture{};
		src_texture.dwSize = sizeof(CMP_Texture);
		src_texture.dwWidth = uncompressed_texture.width;
		src_texture.dwHeight = uncompressed_texture.height;
		src_texture.dwPitch = uncompressed_texture.width * uncompressed_texture.channels; // bytes per row
		src_texture.format = input_format;
		src_texture.dwDataSize = static_cast<CMP_DWORD>(uncompressed_texture.data.size());
		src_texture.pData = reinterpret_cast<CMP_BYTE*>(const_cast<std::byte*>(uncompressed_texture.data.data()));

		/*
			Setup destination texture (output).
		*/
		CMP_Texture dst_texture{};
		dst_texture.dwSize = sizeof(CMP_Texture);
		dst_texture.dwWidth = uncompressed_texture.width;
		dst_texture.dwHeight = uncompressed_texture.height;
		dst_texture.dwPitch = 0; // let CMP calculate
		dst_texture.format = output_format;
		dst_texture.dwDataSize = CMP_CalculateBufferSize(&dst_texture);
		dst_texture.pData = reinterpret_cast<CMP_BYTE*>(malloc(dst_texture.dwDataSize));

		if (!dst_texture.pData)
		{
			return CompressedTextureData("Failed to allocate destination buffer");
		}

		/*
			Set compression options.
		*/
		CMP_CompressOptions options{};
		options.dwSize = sizeof(CMP_CompressOptions);
		options.fquality = 0.88f;   // quality: 0.0 (fastest) – 1.0 (best)
		options.dwnumThreads = 0;       // 0 = use all available hardware threads

		/*
			Compress.
		*/
		CMP_ERROR cmp_error = CMP_ConvertTexture(&src_texture, &dst_texture, &options, nullptr /*no progress callback*/);

		if (cmp_error != CMP_OK)
		{
			free(dst_texture.pData);
			return CompressedTextureData(std::string("CMP_ConvertTexture failed, error code: ") + std::to_string(static_cast<int>(cmp_error)));
		}

		/*
			Move compressed bytes into the result object and free the CMP buffer.
		*/
		std::vector<std::byte> compressed_bytes(
			reinterpret_cast<std::byte*>(dst_texture.pData),
			reinterpret_cast<std::byte*>(dst_texture.pData) + dst_texture.dwDataSize);

		free(dst_texture.pData);

		return CompressedTextureData(
			uncompressed_texture.width, uncompressed_texture.height, 
			0, //Temporary.
			output_format, 
			uncompressed_texture.is_srgb, 
			uncompressed_texture.channels, 
			std::move(compressed_bytes)
		);
	}
#endif
}

#ifdef PLATFORM_WINDOWS

std::optional<std::string> Scene::CompressAndWriteTexture(const std::filesystem::path& src_path, bool compress_BCx, bool compress_ETC2)
{
	/*
		Compress into BC and ETC2 before writing both files to same directory as src (with different extensions).
		Do checks for srgb ("Albedo")
	*/

	//== Call stbi_load after GetDataFromFile, with flip_vertically toggled on.

	//== Call CompressTexture for each non-empty suffix, and write to folder.
}
#endif


//
	// This function utilizes the stb_image or gli library for loading image files and OpenGL 
	// for creating and handling the texture.
	// 
	// GLuint: The function returns a GLuint, which is a type used by OpenGL 
	//          to represent an identifier for the texture. This value is used 
	//          later to refer to the texture in rendering operations.
	// const char* filename: This parameter takes a string representing the file path 
	//          of the image from which the texture is to be loaded.
	// isSRGB: States if the texture is an srgb texture, which requires a different compression and loading process.
GLuint setupTexture(const char* filename, bool isSRGB = true)
{

	/*
		Note: If BCx is supported on android, then load that (as ETC2 doesn't have 1 or 2 channel compression and has to resort to uncompressed for those).
		When loading, load BC first (Windows, Android if possible), then ETC2 (Android if extension not supported), then uncompressed (png, jpg).
		When loading uncompressed check if texture is already loaded (via other methods like BC or ETC2)
	*/

	/*
		Standard Image Formats (png, jpg) --> stbi
		Compressed Texture (dxt5, etc2) --> gli
	*/
	//For compressed texture loading, i believe it support isSRGB and cubemaps.
	//Code taken from GLI documentation		
	constexpr std::string compressed_extension[]{ ".DDS", ".dds", ".ktx", ".ktx2" };
	if (std::find(std::begin(compressed_extension), std::end(compressed_extension), std::filesystem::path(filename).extension().string()) != std::end(compressed_extension))
	{
		std::vector<unsigned char> buffer = EngineFramework::AssetManagerSystem::GetDataFromFile(filename);
		gli::texture Texture = gli::load((const char*)buffer.data(), buffer.size()); //Self-Managed via RAII
		if (Texture.empty()) throw std::runtime_error(std::string("Failed to load texture: ") + filename);

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

		glm::tvec3<GLsizei> const extent(Texture.extent());
		GLsizei const FaceTotal = static_cast<GLsizei>(Texture.layers() * Texture.faces());

		switch (Texture.target())
		{
		case gli::TARGET_1D_ARRAY:
		case gli::TARGET_2D:
		case gli::TARGET_CUBE:
			glTexStorage2D(
				Target, static_cast<GLint>(Texture.levels()), Format.Internal,
				extent.x, Texture.target() == gli::TARGET_2D ? extent.y : FaceTotal);
			break;
		case gli::TARGET_2D_ARRAY:
		case gli::TARGET_3D:
		case gli::TARGET_CUBE_ARRAY:
			glTexStorage3D(
				Target, static_cast<GLint>(Texture.levels()), Format.Internal,
				extent.x, extent.y,
				Texture.target() == gli::TARGET_3D ? extent.z : FaceTotal);
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
	//For Standard image loading + Compression
	else
	{

#ifdef PLATFORM_ANDROID

		int width, height, bytesPerPix;
		stbi_set_flip_vertically_on_load(true);

		std::vector<unsigned char> texture_data = EngineFramework::AssetManagerSystem::GetDataFromFile(std::filesystem::path(filename));
		unsigned char* data = stbi_load_from_memory(
			texture_data.data(),
			static_cast<int>(texture_data.size()),
			&width, &height, &bytesPerPix, 0
		);

		if (!data)
			throw std::runtime_error(std::string("Failed to load texture: ") + filename);

		//Unable to (and should not) compress images in android. Just load as normal uncompressed texture, should this texture be uncompressed.
		//Default.
		GLint internalFormat = GL_RGBA8;
		GLint format = GL_RGBA;

		//Textures can have 1-4 channels, so switch formats to match that.
		switch (bytesPerPix)
		{
		case 1:
			internalFormat = GL_R8;
			format = GL_RED;
			break;
		case 2:
			internalFormat = GL_RG8;
			format = GL_RG;
			break;
		case 3:
			internalFormat = GL_RGB8;
			format = GL_RGB;
			if (isSRGB) internalFormat = GL_SRGB8;
			break;
		case 4:
			internalFormat = GL_RGBA8;
			format = GL_RGBA;
			if (isSRGB) internalFormat = GL_SRGB8_ALPHA8;
			break;
		default:
			throw std::runtime_error(std::string("Failed to load texture: ") + filename);
		}

		GLuint texID = 0;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		//Save previous setting so it can be loaded back afterwards.
		GLint previousAlignment = 0;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
		//Tightly packed texture, no padding.
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// Allocates AND uploads Level 0 in one go
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);


		// Automatically generates levels 1...N
		glGenerateMipmap(GL_TEXTURE_2D);
		//When texture is mapped to a larger surface than its size.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//When texture is mapped to a smaller surface than its size, mipmap is used. Trilinear filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		//Set texture to repeat vertically/horizontally if out-of-bounds.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);

		return texID;

#elif defined(PLATFORM_WINDOWS)

		//TODO: Compress texture.
#endif
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
	for (GLuint i = 0; i < 6; i++)
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
	if (file.find("Albedo") != std::string::npos) {
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
