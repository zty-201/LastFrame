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
 *  Compatibility Notes:
    Glad.h has been generated for opengl es 3.2 for android and windows platform.

    Android: ETC2 texture guaranteed, ASTC probably available on most modern phones.
    Desktop: ETC2 texture may be available but emulated to uncompressed. Use BCx texture (added via extensions) instead.

 * @author Zhang TongYan
 * @date Sept, 2025
 */
#include "pch.hpp"
#include <utility>
#include "gli/gli.hpp"

#include "scene.h"
#include "AssetManager.hpp"

 // For loading textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef PLATFORM_WINDOWS
//For compressing textures
#include "compressonator.h"
#endif

#include <algorithm>
//== Types
namespace
{
	enum class CompressionTextureType
	{
		BC_X, //For Windows
		ETC2, //For Android.
		UNKNOWN
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
			std::vector<std::pair<uint32_t, uint32_t>> mipmaps_width_height,
			std::vector<uint32_t> compressed_data_size,
			uint32_t                    mip_levels,
			GLenum      compressed_type,
			bool                        is_srgb,
			int                       channels,
			std::vector<std::byte>      data,
			std::optional<std::string>  error_str = std::nullopt
		) noexcept
			:
			mipmaps_width_height(std::move(mipmaps_width_height)),
			compressed_data_size(std::move(compressed_data_size)),
			mip_levels(mip_levels),
			compressed_type(compressed_type),
			is_srgb(is_srgb),
			channels(channels),
			data(std::move(data)),
			error_str(std::move(error_str))
		{
		}

		bool isValid() const noexcept { return !error_str && !data.empty() && mipmaps_width_height.size() > 0 && mip_levels > 0 && channels > 0 && compressed_type != 0; }


	public:
		// Texture data
		std::vector<std::pair<uint32_t, uint32_t>> mipmaps_width_height; //Width and Height of each mipmap.
		std::vector<uint32_t> compressed_data_size{}; //Data size of each compressed mipmap
		uint32_t    mip_levels = 1;
		GLenum compressed_type{};
		bool        is_srgb = false;
		int       channels = 0;
		std::vector<std::byte> data;
		std::optional<std::string> error_str;
	};

	//RAII data containing uncompressed texture data loaded by stbi.
	class UnCompressedTextureData
	{
	public:
		UnCompressedTextureData() = default;
		UnCompressedTextureData(std::shared_ptr<unsigned char> data, uint32_t width, uint32_t height, uint32_t channels, bool is_srgb)
			: width{ width }, height{ height }, channels{ channels }, is_srgb{ is_srgb } {
			if (width > 0 && height > 0 && channels > 0)
			{
				this->data = std::vector<std::byte>(reinterpret_cast<std::byte*>(data.get()), reinterpret_cast<std::byte*>(data.get()) + width * height * channels);
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

}

//Helper Functions
namespace
{
	//Forward Declarations
	[[nodiscard]] std::optional<std::string> GetGLTextureFormat(GLenum& current_format, GLint& internal_format, int channels, bool is_srgb);
#ifdef PLATFORM_WINDOWS //Only for windows, where it actively compresses textures and uses Compressonator defines.

	/*
		Sets the values of the compressed_format variables passed in, based on the current format and compression type chosen.
		Bits per channel assumed to be a fixed 8 bits.

		\param uncompressed_format
		The format the texture is currently in.
		\param compression_type
		The compression type to compress to	
		\param [out] compressed_format
		The format to compress to
		\param [out] num_channels
		Number of channels of the image


		Returns an error string if unable to set a format.
	*/
	[[nodiscard]] std::optional<std::string> GetTextureCompressionFormat(const CMP_FORMAT& uncompressed_format, CompressionTextureType compression_type, CMP_FORMAT& compressed_format, int &num_channels)
	{
		//Bits per channel assumed to be a fixed 8 bits.
		switch (uncompressed_format)
		{
		case CMP_FORMAT_R_8:
			num_channels = 1;
			break;
		case CMP_FORMAT_RG_8:
			num_channels = 2;
			break;
		case CMP_FORMAT_RGB_888:
			num_channels = 3;
			break;
		case CMP_FORMAT_RGBA_8888:
			num_channels = 4;
			break;
		default:
			return "Texture format currently unsupported outside of R to RGBA 8 bit channel";
		}

		/*
			Compression:
			- There are two types of compression, one for BCx (Windows) and one for ETC2 (Android).
			Among those, there are different formats for channels 1-4.

			is_srgb is defined as the color space and has the same uncompressed/compressed format as non-srgb.
			Thus, compress srgb the same way, as long as the gpu knows it's srgb when uploaded.
		*/
		if (compression_type == CompressionTextureType::BC_X)
		{
			switch (uncompressed_format)
			{
			case CMP_FORMAT_R_8:
				compressed_format = CMP_FORMAT_BC4;
				break;
			case CMP_FORMAT_RG_8:
				compressed_format = CMP_FORMAT_BC5;
				break;
			case CMP_FORMAT_RGB_888:
				compressed_format = CMP_FORMAT_BC1;
				break;
			case CMP_FORMAT_RGBA_8888:
				compressed_format = CMP_FORMAT_BC3;
				break;
			}
		}
		else if (compression_type == CompressionTextureType::ETC2)
		{
			switch (uncompressed_format)
			{
			case CMP_FORMAT_R_8:
				return "1 Channel ETC2 unsupported";  //Potential for ASTC or .basis file.
				break;
			case CMP_FORMAT_RG_8:
				return "2 Channel ETC2 unsupported";
				break;
			case CMP_FORMAT_RGB_888:
				compressed_format = CMP_FORMAT_ETC2_RGB;
				break;
			case CMP_FORMAT_RGBA_8888:
				compressed_format = CMP_FORMAT_ETC2_RGBA;
				break;
			}
		}
		else return "Invalid Compression Type";

		return {}; //No Error.
	}

	/*
		This overload is for compressed texture.
		Sets the opengl formats for the current compressed texture type based on CMP format passed in.
		Function assumes every channel is 8 bits.

		\param compressed_cmp_format
		CMP_FORMAT of the data
		\param is_srgb
		Whether texture is in srgb (gamma) color space
		Useful as CMP_FORMAT doesn't inherently store this information.
		\param [out] compressed_opengl_format
		The corresponding opengl compressed format.
		\return
		Nothing on success, Error string otherwise.
	*/
	[[nodiscard]] std::optional<std::string> GetGLTextureFormat(CMP_FORMAT compressed_cmp_format, bool is_srgb, GLenum& compressed_opengl_format)
	{
		switch (compressed_cmp_format)
		{
			//BCx compression.
		case CMP_FORMAT_BC4:
			compressed_opengl_format = GL_COMPRESSED_RED_RGTC1_EXT;
			break;
		case CMP_FORMAT_BC5:
			compressed_opengl_format = GL_COMPRESSED_RED_GREEN_RGTC2_EXT;
			break;
		case CMP_FORMAT_BC1:
			compressed_opengl_format = (is_srgb) ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			break;
		case CMP_FORMAT_BC3:
			compressed_opengl_format = (is_srgb) ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;

			///ETC2 compression.
		case CMP_FORMAT_ETC2_RGB:
			compressed_opengl_format = (is_srgb) ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
			break;
		case CMP_FORMAT_ETC2_RGBA:
			compressed_opengl_format = (is_srgb) ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
			break;

		default:
			return "Unknown CMP Format passed in. Update GetGLTextureFormat if a new compression type has been added.";
		}
		return {}; //No Error.
	}

	void flipTextureVertical(CMP_MipSet& mipset) {
		UNREFERENCED_PARAMETER(mipset);
		//use dwLinearSize / dwHeight for row size for each mipmap level separately.
		
	}

	/*
		Returns compressed data of the texture filepath passed in.
		\param is_srgb
		Whether texture file data is in srgb/gamma colorspace.
		\param TextureType
		What type to compress texture to.
		\param quality
		From 0 to 1.0. Much higher performance hit at higher levels.
		\param max_mipmap_level
		Maximum mipmap levels that can be generated from the image. 
		This is the requested level, however less levels may be generated if it is not possible.
	*/
	CompressedTextureData CompressTexture(const std::filesystem::path& file_path, bool is_srgb, CompressionTextureType compression_type, float quality, unsigned int max_mipmap_level = 100)
	{
		//TODO: Look into supporting 1-2 channel compression on mobile using ASTC compression or ETC2 RGB, and see if it's beneficial.
		//TODO: Look into compressing to .basis file, then transcoding during runtime to GPU supported texture (more compatibility, no need to worry about astc).
		if (quality < 0 || quality > 1.0) return CompressedTextureData("Invalid Compression Quality passed in, must be in range [0, 1]");
		/*
			Process
			1. Load texture into mipmap set
			2. Get important parameters like compression format, num channels
			3. Generate mipmap chain if not present.
			4. Compress mipmap
			5. Write mipmap to CompressedTextureData and free mipmaps used.
		*/
		//== Load texture data into mipmap set.
		CMP_MipSet mipset_in;
		memset(&mipset_in, 0, sizeof(CMP_MipSet));
		auto cmp_status = CMP_LoadTexture(file_path.string().c_str(), &mipset_in);
		if (cmp_status != CMP_OK) {
			return CompressedTextureData("Unable to load texture file");
		}

		/*
			TODO:
			1. Try to manually load into mipset instead of using CMP_LoadTexture which defaults to always load as RGBA
			- Run once and record all the settings set by LoadTexture.
			2. If so, then setting stbi_image_load_vertically(true) will remove the need to manually flip image.
		*/
		flipTextureVertical(mipset_in); //Vertically flip all levels before generating mipmap chain.

		
		//==Generate mipmap chain if not present.
		if (mipset_in.m_nMipLevels <= 1)
		{
			/*
			*	Calculates the minimum size of the lowest-level mipmap after generation.
			*	If requested level is too high, then an adjusted size will be returned.
			*/
			CMP_INT nMinSize = CMP_CalcMinMipSize(mipset_in.m_nHeight, mipset_in.m_nWidth, (CMP_INT) max_mipmap_level);

			//Generate levels.
			CMP_GenerateMIPLevels(&mipset_in, nMinSize);
		}


		/*
			Get important parameters.
			Note: srgb and non-srgb is treated the same way, as the format is the same (only the color space is different) and thus they can be compressed
			the same way (so long as gpu knows it's srgb when uploading).
		*/

		//==Input is assumed to always be 8bits per channel (no floating pt).
		CMP_FORMAT output_format{};
		int num_channels{};
		auto err_code = GetTextureCompressionFormat(mipset_in.m_format, compression_type, output_format, num_channels);
		if (err_code.has_value())
		{
			CMP_FreeMipSet(&mipset_in);
			return CompressedTextureData(err_code.value());
		}
		/*
			Compress mipmap.
		*/

		//== Set Options
		KernelOptions kernel_options;
		memset(&kernel_options, 0, sizeof(KernelOptions));
		kernel_options.format = output_format;   // Set the format to process
		kernel_options.fquality = quality;     // Set the quality of the result
		kernel_options.threads = 0;            // Auto setting
		if(is_srgb) kernel_options.useSRGBFrames = true;

		// Mipmap set buffer for compressed texture
		CMP_MipSet mipset_out;
		memset(&mipset_out, 0, sizeof(CMP_MipSet));

		//===============================================
		// Compress the texture using Compressonator Lib
		//===============================================
		cmp_status = CMP_ProcessTexture(&mipset_in, &mipset_out, kernel_options, nullptr);
		if (cmp_status != CMP_OK)
		{
			CMP_FreeMipSet(&mipset_in);
			return CompressedTextureData("Error when compressing texture: " + std::to_string(cmp_status));
		}

		/*
			Copy over to result.
		*/
		// --- Concatenate all data into a single array, keep track of important info.
		std::vector<std::byte> all_compressed_data;
		std::vector<uint32_t> compressed_data_size{}; //Each mipmap data size after compression.
		std::vector<std::pair<uint32_t, uint32_t>> mipmaps_width_height{}; //Keep track of each mipmaps dimensions.
		int num_mipmaps = mipset_out.m_nMipLevels;
		for (CMP_INT mip = 0; mip < num_mipmaps; mip += 1)
		{
			//Get mipmap data and dimensions for that level.
			CMP_MipLevel* mip_level = nullptr;
			CMP_GetMipLevel(&mip_level, &mipset_out, mip, 0);
			if (!mip_level)
			{
				CMP_FreeMipSet(&mipset_in);
				CMP_FreeMipSet(&mipset_out);
				return CompressedTextureData("Failed to create MipMap Set");
			}
			//Set important information for each mip level individually.
			mipmaps_width_height.push_back(std::pair<uint32_t, uint32_t>(mip_level->m_nWidth, mip_level->m_nHeight));
			all_compressed_data.insert(all_compressed_data.end(),
				reinterpret_cast<std::byte*>(mip_level->m_pbData), reinterpret_cast<std::byte*>(mip_level->m_pbData) + mip_level->m_dwLinearSize);
			compressed_data_size.push_back(static_cast<uint32_t>(mip_level->m_dwLinearSize));
		}

		CMP_FreeMipSet(&mipset_in);
		CMP_FreeMipSet(&mipset_out);


		//Compressed format opengl equivalent.
		GLenum opengl_compressed_format;
		err_code = GetGLTextureFormat(output_format, is_srgb, opengl_compressed_format);
		if (err_code.has_value())
		{
			//Something went wrong, return val from GetTextureCompressionFormat doesn't match with GetGLTextureFormat
			return CompressedTextureData(err_code.value());
		}

		return CompressedTextureData(
			std::move(mipmaps_width_height),
			std::move(compressed_data_size),
			num_mipmaps,
			opengl_compressed_format,
			is_srgb,
			num_channels,
			std::move(all_compressed_data)
		);
	}
#endif	

	/*
		Uploads an uncompressed texture.
		If failed, an error code is returned and the texture is not created.

		\param [out] created_texture
		ID of the created texture is written here
		\param file_path
		File path of the texture
		\param is_srgb
		Whether texture is in srgb color space, typically used for Albedo textures.
	*/
	[[nodiscard]] std::optional<std::string> LoadUncompressedTexture(GLuint& loaded_texture, const std::filesystem::path& file_path, bool is_srgb)
	{
		//For now, just load as is without run-time compression. 
		//TODO: Look into runtime-compression for these uncompressed files using generic compression_format and gl_teximage2d. May be too poor quality however.
		loaded_texture = 0;
		
		//== Load data from file.
		int width, height, bytesPerPix;
		stbi_set_flip_vertically_on_load(true);
		std::vector<unsigned char> texture_data = EngineFramework::AssetManagerSystem::GetDataFromFile(file_path);
		if (texture_data.empty())
			return std::string("Failed to read texture file: ") + file_path.string();
		unsigned char* stbi_data_DONOTUSE = stbi_load_from_memory(
			texture_data.data(),
			static_cast<int>(texture_data.size()),
			&width, &height, &bytesPerPix, 0
		);

		//RAII dtor for raw stbi data
		auto free_stbi_data = [](unsigned char* stbi_data)
		{
			if (stbi_data) stbi_image_free(stbi_data);
		};
		std::shared_ptr<unsigned char> stbi_data(stbi_data_DONOTUSE, free_stbi_data);

		if (!stbi_data)
			return std::string("Failed to load texture: ") + file_path.string();
		

		//TODO: delete runtime-compress code, just use normal code (since no need to compress at runtime). This is just to test if compressor is working.
#ifdef PLATFORM_WINDOWS
#define RUNTIME_COMPRESSION_TEST
#endif
#ifdef RUNTIME_COMPRESSION_TEST
		//Mimic loading of BCx precompressed texture. For now it just uses runtime-compression instead.
		CompressedTextureData compressed_texture(std::move(CompressTexture(file_path, is_srgb, CompressionTextureType::BC_X, 0.88f)));

		//Compressed texture is invalid, fallback to uncompressed.
		if (!compressed_texture.isValid())
		{
			LOGD("Unable to compress, using uncompressed: %s", file_path.string().c_str());
			//internal_format -> what format to upload as.  current_format -> what format the texture data is prior to upload.
			GLenum current_format = GL_RGBA;
			GLint internal_format = GL_RGBA8;
			std::optional<std::string> error_code = GetGLTextureFormat(current_format, internal_format, bytesPerPix, is_srgb);
			if (error_code)
			{
				return error_code;
			}
			//==Upload to GPU with various params.
			GLuint texID = 0;
			glGenTextures(1, &texID);
			if (texID == 0) return "Unable to allocate texture in gpu mem";
			glBindTexture(GL_TEXTURE_2D, texID);

			//When texture is mapped to a larger surface than its size.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//When texture is mapped to a smaller surface than its size, mipmap is used. Trilinear filtering.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			//Set texture to repeat vertically/horizontally if out-of-bounds.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			//Save previous setting so it can be loaded back afterwards.
			GLint previousAlignment = 0;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
			//Tightly packed texture, no padding.
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Allocates AND uploads Level 0 in one go
			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, current_format, GL_UNSIGNED_BYTE, stbi_data.get());

			// Automatically generates levels 1...N
			glGenerateMipmap(GL_TEXTURE_2D);

			glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
			glBindTexture(GL_TEXTURE_2D, 0);

			//Check for error when uploading.
			GLenum err = glGetError();
			if (err != GL_NO_ERROR) {
				if (texID) glDeleteTextures(1, &texID);
				return std::string("Error when uploading texture: ERR_CODE ") + std::to_string(err);
			}

			loaded_texture = texID;
			return {}; //No Error.
		}
		//Upload compressed texture.
		else
		{			
			//==Upload to GPU with various params.
			GLuint texID = 0;
			glGenTextures(1, &texID);
			if (texID == 0) return "Unable to allocate texture in gpu mem";
			glBindTexture(GL_TEXTURE_2D, texID);

			//When texture is mapped to a larger surface than its size.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//When texture is mapped to a smaller surface than its size, mipmap is used. Trilinear filtering.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, compressed_texture.mip_levels-1);

			//Set texture to repeat vertically/horizontally if out-of-bounds.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			//Save previous setting so it can be loaded back afterwards.
			GLint previousAlignment = 0;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
			//Tightly packed texture, no padding.
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			/*
				Compressed texture is actually a chain of individually compressed mipmaps.
				Thus, load each mipmap one-by-one.

				Use glCompressedTexImage2D (takes in pre-compressed data) instead of glTexImage2D (which only takes in uncompressed)
			*/
			
			{
				//Updated for each mipmap loaded, to point to the next mipmap.
				int compressed_data_offset = 0; 
				for (uint32_t mip = 0; mip < compressed_texture.mip_levels; ++mip)
				{
					const uint32_t mip_width = compressed_texture.mipmaps_width_height[mip].first;
					const uint32_t mip_height = compressed_texture.mipmaps_width_height[mip].second;
					const uint32_t mip_data_size = compressed_texture.compressed_data_size[mip];
					
					glCompressedTexImage2D(
						GL_TEXTURE_2D,
						static_cast<GLint>(mip),  // Mipmap level
						compressed_texture.compressed_type,           // Internal compressed format (e.g. GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
						static_cast<GLsizei>(mip_width),
						static_cast<GLsizei>(mip_height),
						0,                        // Border, must be 0
						static_cast<GLsizei>(mip_data_size),
						static_cast<const void*>(&compressed_texture.data[compressed_data_offset])
					);
					//Move offset to next mipmap.
					compressed_data_offset += mip_data_size;
				}
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
			glBindTexture(GL_TEXTURE_2D, 0);

			//Check for error when uploading.
			GLenum err = glGetError();
			if (err != GL_NO_ERROR) {
				if (texID) glDeleteTextures(1, &texID);
				return std::string("Error when uploading texture: ERR_CODE ") + std::to_string(err);
			}

			loaded_texture = texID;
			return {}; //No Error.
		}
#endif
#ifndef RUNTIME_COMPRESSION_TEST
		//internal_format -> what format to upload as.  current_format -> what format the texture data is prior to upload.
		GLenum current_format = GL_RGBA;
		GLint internal_format = GL_RGBA8;
		std::optional<std::string> error_code = GetGLTextureFormat(current_format, internal_format, bytesPerPix, is_srgb);
		if (error_code)
		{
			return error_code;
		}
		//==Upload to GPU with various params.
		GLuint texID = 0;
		glGenTextures(1, &texID);
		if (texID == 0)
		{
			return "Unable to allocate texture in gpu mem";
		}
		glBindTexture(GL_TEXTURE_2D, texID);

		//When texture is mapped to a larger surface than its size.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//When texture is mapped to a smaller surface than its size, mipmap is used. Trilinear filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		//Set texture to repeat vertically/horizontally if out-of-bounds.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//Save previous setting so it can be loaded back afterwards.
		GLint previousAlignment = 0;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
		//Tightly packed texture, no padding.
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// Allocates AND uploads Level 0 in one go
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, current_format, GL_UNSIGNED_BYTE, data);

		// Automatically generates levels 1...N
		glGenerateMipmap(GL_TEXTURE_2D);
		

		glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
		glBindTexture(GL_TEXTURE_2D, 0);


		//Check for error when uploading.
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			if(texID) glDeleteTextures(1, &texID);
			return std::string("Error when uploading texture: ERR_CODE ") + std::to_string(err);
		}

		loaded_texture = texID;
		return {}; //No Error.
#endif
	}

	/*
		Uploads a compressed texture.
		If failed, an error code is returned and the texture is not created.

		\param [out] created_texture
		ID of the created texture is written here
		\param file_path
		File path of the texture
		\param is_srgb
		Whether texture is in srgb color space, typically used for Albedo textures.
	*/
	[[nodiscard]] std::optional<std::string> LoadCompressedTexture(GLuint& loaded_texture, const std::filesystem::path& file_path, bool is_srgb)
	{
		loaded_texture = 0;
		UNREFERENCED_PARAMETER(file_path);
		UNREFERENCED_PARAMETER(is_srgb);
		return {};

		/*
			NOTE: GL_COMPRESSEDTEXIMAGE2D should be used when uploading pre-compressed data, as specifying compressed_format in gl_teximage2d ends up doing runtime-compression that is poor quality.
		*/

		///*
		//	Note: If BCx is supported on android, then load that (as ETC2 doesn't have 1 or 2 channel compression and has to resort to uncompressed for those).
		//	When loading, load BC first (Windows, Android if possible), then ETC2 (Android if extension not supported), then uncompressed (png, jpg).
		//	When loading uncompressed check if texture is already loaded (via other methods like BC or ETC2)

		//	For SRGB, use the respective opengl srgb formats. Format the compressed texture was stored in stays the same, Format to upload is different.
		//*/

		///*
		//	Standard Image Formats (png, jpg) --> stbi
		//	Compressed Texture (dxt5, etc2) --> gli
		//*/
		////For compressed texture loading, i believe it support isSRGB and cubemaps.
		////Code taken from GLI documentation		
		//constexpr std::string compressed_extension[]{ ".DDS", ".dds", ".ktx", ".ktx2" };
		//if (std::find(std::begin(compressed_extension), std::end(compressed_extension), std::filesystem::path(filename).extension().string()) != std::end(compressed_extension))
		//{
		//	std::vector<unsigned char> buffer = EngineFramework::AssetManagerSystem::GetDataFromFile(filename);
		//	gli::texture Texture = gli::load((const char*)buffer.data(), buffer.size()); //Self-Managed via RAII
		//	if (Texture.empty()) throw std::runtime_error(std::string("Failed to load texture: ") + filename);

		//	//There's no need to flip texture, since it should already flipped during compression process.

		//	gli::gl GL(gli::gl::PROFILE_ES30);
		//	gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
		//	GLenum Target = GL.translate(Texture.target());

		//	GLuint TextureName = 0;
		//	glGenTextures(1, &TextureName);
		//	glBindTexture(Target, TextureName);
		//	glTexParameteri(Target, GL_TEXTURE_BASE_LEVEL, 0);
		//	glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
		//	glTexParameteri(Target, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
		//	glTexParameteri(Target, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
		//	glTexParameteri(Target, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
		//	glTexParameteri(Target, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);

		//	glm::tvec3<GLsizei> const extent(Texture.extent());
		//	GLsizei const FaceTotal = static_cast<GLsizei>(Texture.layers() * Texture.faces());

		//	switch (Texture.target())
		//	{
		//	case gli::TARGET_1D_ARRAY:
		//	case gli::TARGET_2D:
		//	case gli::TARGET_CUBE:
		//		glTexStorage2D(
		//			Target, static_cast<GLint>(Texture.levels()), Format.Internal,
		//			extent.x, Texture.target() == gli::TARGET_2D ? extent.y : FaceTotal);
		//		break;
		//	case gli::TARGET_2D_ARRAY:
		//	case gli::TARGET_3D:
		//	case gli::TARGET_CUBE_ARRAY:
		//		glTexStorage3D(
		//			Target, static_cast<GLint>(Texture.levels()), Format.Internal,
		//			extent.x, extent.y,
		//			Texture.target() == gli::TARGET_3D ? extent.z : FaceTotal);
		//		break;
		//	default:
		//		assert(0);
		//		break;
		//	}

		//	for (std::size_t Layer = 0; Layer < Texture.layers(); ++Layer)
		//		for (std::size_t Face = 0; Face < Texture.faces(); ++Face)
		//			for (std::size_t Level = 0; Level < Texture.levels(); ++Level)
		//			{
		//				GLsizei const LayerGL = static_cast<GLsizei>(Layer);
		//				glm::tvec3<GLsizei> Extent(Texture.extent(Level));
		//				Target = gli::is_target_cube(Texture.target())
		//					? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face)
		//					: Target;

		//				switch (Texture.target())
		//				{
		//				case gli::TARGET_1D_ARRAY:
		//				case gli::TARGET_2D:
		//				case gli::TARGET_CUBE:
		//					if (gli::is_compressed(Texture.format()))
		//						glCompressedTexSubImage2D(
		//							Target, static_cast<GLint>(Level),
		//							0, 0,
		//							Extent.x,
		//							Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
		//							Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
		//							Texture.data(Layer, Face, Level));
		//					else
		//						glTexSubImage2D(
		//							Target, static_cast<GLint>(Level),
		//							0, 0,
		//							Extent.x,
		//							Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
		//							Format.External, Format.Type,
		//							Texture.data(Layer, Face, Level));
		//					break;
		//				case gli::TARGET_2D_ARRAY:
		//				case gli::TARGET_3D:
		//				case gli::TARGET_CUBE_ARRAY:
		//					if (gli::is_compressed(Texture.format()))
		//						glCompressedTexSubImage3D(
		//							Target, static_cast<GLint>(Level),
		//							0, 0, 0,
		//							Extent.x, Extent.y,
		//							Texture.target() == gli::TARGET_3D ? Extent.z : LayerGL,
		//							Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
		//							Texture.data(Layer, Face, Level));
		//					else
		//						glTexSubImage3D(
		//							Target, static_cast<GLint>(Level),
		//							0, 0, 0,
		//							Extent.x, Extent.y,
		//							Texture.target() == gli::TARGET_3D ? Extent.z : LayerGL,
		//							Format.External, Format.Type,
		//							Texture.data(Layer, Face, Level));
		//					break;
		//				default: assert(0); break;
		//				}
		//			}
		//	return TextureName;
		//}
	}

	/*
		This overload is for uncompressed texture.
		Sets the opengl formats for the current texture type, and what to upload the texture as.
		Function assumes every channel is 8 bits.

		\param [out] current_format
		Writes what format the texture data is prior to upload.
		\param [out] internal_format
		Writes what format the texture should be uploaded as
		\param channels
		Number of channels for texture
		\param is_srgb
		Whether texture is in srgb (gamma) color space
		\return
		Nothing on success, Error string otherwise.
	*/
	[[nodiscard]] std::optional<std::string> GetGLTextureFormat(GLenum& current_format, GLint &internal_format, int channels, bool is_srgb)
	{
		//Textures can have 1-4 channels, so switch formats to match that.
		//SRGB only applies to 3-4 channel (rgb is needed for gamma color space), ignore if 1-2.
		switch (channels)
		{
		case 1:
			internal_format = GL_R8;
			current_format = GL_RED;
			break;
		case 2:
			internal_format = GL_RG8;
			current_format = GL_RG;
			break;
		case 3:
			current_format = GL_RGB;
			if (is_srgb) internal_format = GL_SRGB8;
			else internal_format = GL_RGB8;
			break;
		case 4:
			current_format = GL_RGBA;
			if (is_srgb) internal_format = GL_SRGB8_ALPHA8; 
			else internal_format = GL_RGBA8;
			break;
		default:
			return "Invalid number of channels";
		}
		return {}; //No Error.
	}

	/*
	* //TODO: This function is currently being unused, as GetGLTextureFormat uses CMP_FORMAT overload version for compressed textures.
		This overload is for compressed texture.
		Sets the opengl formats for the current compressed texture type.
		Function assumes every channel is 8 bits.

		\param [out] current_format
		Writes what format the compressed texture data is prior to upload.
		\param channels
		Number of channels for texture
		\param is_srgb
		Whether texture is in srgb (gamma) color space
		\param compression_type
		BCx for BC1-5, ETC2 for ETC2 textures.
		\return
		Nothing on success, Error string otherwise.
	*/
	[[nodiscard]] std::optional<std::string> GetGLTextureFormat(GLenum& current_format, int channels, bool is_srgb, CompressionTextureType compression_type)
	{
		//Textures can have 1-4 channels, so switch formats to match that.
		//SRGB only applies to 3-4 channel (rgb is needed for gamma color space), ignore if 1-2.
		if (channels < 1 || channels > 4) return "Invalid number of channels";
		if (compression_type == CompressionTextureType::BC_X)
		{
#ifdef PLATFORM_WINDOWS //Compression types might not be exposed for android glad header.
			switch (channels)
			{
			case 1:
				current_format = GL_COMPRESSED_RED_RGTC1_EXT;
				break;
			case 2:
				current_format = GL_COMPRESSED_RED_GREEN_RGTC2_EXT;
				break;
			case 3:
				/*
					SRGB / RGB is used instead of the + 1 bit Alpha version of DXT1.
					Both formats are the same size, but since original data was in RGB format it'll always be opaque.
				*/
				current_format = (is_srgb)? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				break;
			case 4:
				current_format = (is_srgb)? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
			}
#else
			return "Android should not load BCx compression formats";
#endif
		}
		else if (compression_type == CompressionTextureType::ETC2)
		{
			switch (channels)
			{
			case 3:
				current_format = (is_srgb)? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
				break;
			case 4:
				current_format = (is_srgb)? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
				break;
			default:
				return "Current Compression Algorithm does not support 1-2 channel ETC2 compression. Update GetGLTextureFormat if it does.";
			}
		}
		return {}; //No Error
	}


}


#ifdef PLATFORM_WINDOWS

[[nodiscard]] std::optional<std::string> Scene::CompressAndWriteTexture(const std::filesystem::path& src_path, bool compress_BCx, bool compress_ETC2)
{
	if (!(compress_BCx || compress_ETC2))
	{
		return "At least one compression type should be selected.";
	}
	//TODO: CompressAndWriteTexture for storing texture as compressed texture files with different extensions.
	UNREFERENCED_PARAMETER(src_path);
	UNREFERENCED_PARAMETER(compress_BCx);
	UNREFERENCED_PARAMETER(compress_ETC2);
	/*
		Compress into BC and ETC2 before writing both files to same directory as src (with different extensions).
		Do checks for srgb ("Albedo")
	*/

	//== Call stbi_load after GetDataFromFile, with flip_vertically toggled on.

	//== Call CompressTexture for each non-empty suffix, and write to folder.

	//== TODO: Define class methods for CompressedTextureData to read/write from file (using GetDataFromFile?) ? Or use typical dds/ktx file types.

	return {}; //No Error.
}
#endif



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




// This function utilizes the stb_image or gli library for loading uncompressed and compressed image files, 
// before uploading using OpenGL to create and handle the texture.
// It does not create any compressed files.
// 
// GLuint: The function returns a GLuint, which is a type used by OpenGL 
//          to represent an identifier for the texture. This value is used 
//          later to refer to the texture in rendering operations.
// 
//			0 is returned if there is an issue and the texture cannot be created/uploaded.
// const char* filename: This parameter takes a string representing the file path 
//          of the image from which the texture is to be loaded.
// isSRGB: States if the texture is an srgb texture, which requires a different compression and loading process.
[[nodiscard]] GLuint setupTexture(const char* filename, bool is_srgb)
{

	/*
		There are two types of image loading:
		- Uncompressed: png, jpg, jpeg
		- Compressed: ".DDS", ".dds", ".ktx", ".ktx2"

		Use extensions to determine what type the image is.
	*/
	constexpr std::array<const char*, 3> uncompressed_extensions{ ".png", ".jpg", ".jpeg" };
	constexpr std::array<const char*, 4> compressed_extensions{ ".DDS", ".dds", ".ktx", ".ktx2" };
	static auto isExtensionValid = []<std::size_t N>(const std::array<const char*, N>&extension_arr, const std::string & extension)
		-> bool
	{
		return (std::find(std::begin(extension_arr), std::end(extension_arr), extension) != std::end(extension_arr));
	};

	std::string extension = std::filesystem::path(filename).extension().string();

	GLuint loaded_texture = 0;
	std::optional<std::string> error_code{};
	//For Standard Image loading
	if (isExtensionValid(uncompressed_extensions, extension))
	{
		error_code = LoadUncompressedTexture(loaded_texture, filename, is_srgb);
	}
	//For Compressed Image loading
	else if (isExtensionValid(compressed_extensions, extension))
	{
		error_code = LoadCompressedTexture(loaded_texture, filename, is_srgb);
	}
	//Invalid file type
	else
	{
		error_code = "Unsupported Texture File Extension";
	}
	if (error_code)
	{
		LOGE("Unable to Load Texture: %s, Error: %s", filename, error_code.value().c_str());
		return 0;
	}
	return loaded_texture;
}

//
// The loadHdrCubeMap function generates and configures a cube map texture using HDR image files.
[[nodiscard]] GLuint setupHdrCubeMap(const std::array<std::string, 6>& filenames)
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