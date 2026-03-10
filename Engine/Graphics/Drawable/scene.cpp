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
		ASTC, //For Android.
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

		bool isValid() const noexcept {
			return !error_str && !data.empty() && mipmaps_width_height.size() > 0 && mip_levels == mipmaps_width_height.size() && compressed_data_size.size() == mip_levels && channels > 0 && compressed_type != 0;
		}
#ifdef PLATFORM_WINDOWS  //Only write compressed files in windows, for android just read data.
		/*
			Writes the fully initialized class to the filepath specified.
			Ensure parent folder is valid.
			\return
			Nothing if successful, error string on error.
		*/
		[[nodiscard]] std::optional<std::string> WriteDataToFile(const std::filesystem::path& file_path) const;
#endif
		/*
			Reads and initializes itself from a compressed file.
			\return
			Nothing if successful, error string on error.
		*/
		[[nodiscard]] std::optional<std::string> ReadDataFromFile(const std::filesystem::path& file_path);

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



#ifdef PLATFORM_WINDOWS
	//======================================================================
	[[nodiscard]] std::optional<std::string> CompressedTextureData::WriteDataToFile(const std::filesystem::path& file_path) const
	{
		if (!isValid())
			return "Cannot write invalid data, this object is not correctly initialized yet";
		std::ofstream file(file_path, std::ios::binary);
		if (!file.is_open())
			return "Failed to open file for writing: " + file_path.string();

		//Simple non-ptr data.
		file.write(reinterpret_cast<const char*>(&mip_levels), sizeof(mip_levels));
		file.write(reinterpret_cast<const char*>(&compressed_type), sizeof(compressed_type));
		file.write(reinterpret_cast<const char*>(&is_srgb), sizeof(is_srgb));
		file.write(reinterpret_cast<const char*>(&channels), sizeof(channels));

		// Write vectors. mip_levels is guaranteed to be the size of these 2 vectors, as isValid() checks for that.
		file.write(reinterpret_cast<const char*>(mipmaps_width_height.data()), mip_levels * sizeof(std::pair<uint32_t, uint32_t>));
		file.write(reinterpret_cast<const char*>(compressed_data_size.data()), mip_levels * sizeof(uint32_t));

		// Write data
		uint64_t data_size = static_cast<uint64_t>(data.size());
		file.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
		file.write(reinterpret_cast<const char*>(data.data()), data_size); //1 byte per element.

		if (!file.good())
			return "Error occurred while writing to file: " + file_path.string();

		return {};
	}
#endif
	[[nodiscard]] std::optional<std::string> CompressedTextureData::ReadDataFromFile(const std::filesystem::path& file_path)
	{
		std::vector<unsigned char> file_data;
		try
		{
			file_data = std::move(EngineFramework::AssetManagerSystem::GetDataFromFile(file_path));
		}
		catch (...)
		{
			return "Unable to read file: " + file_path.string();
		}

		size_t offset = 0;
		const auto read = [&](void* dst, size_t size)
			{
				std::memcpy(dst, file_data.data() + offset, size);
				offset += size;
			};

		// Simple non-ptr data.
		read(&mip_levels, sizeof(mip_levels));
		read(&compressed_type, sizeof(compressed_type));
		read(&is_srgb, sizeof(is_srgb));
		read(&channels, sizeof(channels));

		// Read vectors, now that mip_levels is known.
		mipmaps_width_height.resize(mip_levels);
		compressed_data_size.resize(mip_levels);
		read(mipmaps_width_height.data(), mip_levels * sizeof(std::pair<uint32_t, uint32_t>));
		read(compressed_data_size.data(), mip_levels * sizeof(uint32_t));

		// Read data
		uint64_t data_size = 0;
		read(&data_size, sizeof(data_size));
		data.resize(data_size);
		read(data.data(), data_size);

		if (offset > file_data.size())
			return "Unexpected end of data while reading from file: " + file_path.string();
		if (!isValid())
			return "Invalid data read from file: " + file_path.string();


		return {};
	}
}

//Helper Functions
namespace
{
	//Forward Declarations
	[[nodiscard]] std::optional<std::string> GetGLTextureFormat(GLenum& current_format, GLint& internal_format, int channels, bool is_srgb);
#ifdef PLATFORM_WINDOWS //Only for windows, where it actively compresses textures and uses Compressonator defines.

	/*
		Sets the CMP_FORMAT value of the uncompressed_format variable, based on number of channels.
		Assumes 8 bits per channel.
		\param [out] uncompressed_format
		The original format of the data before compression
		\return
		Error string.
	*/
	[[nodiscard]] std::optional<std::string> GetCMPUncompressedFormat(int num_channels, CMP_FORMAT& uncompressed_format)
	{
		switch (num_channels)
		{
		case 1:
			uncompressed_format = CMP_FORMAT_R_8;
			break;
		case 2:
			uncompressed_format = CMP_FORMAT_RG_8;
			break;
		case 3:
			uncompressed_format = CMP_FORMAT_RGB_888;
			break;
		case 4:
			uncompressed_format = CMP_FORMAT_RGBA_8888;
			break;
		default: return "Invalid number of channels";
		}
		return {};
	}

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
	[[nodiscard]] std::optional<std::string> GetTextureCompressionFormat(const CMP_FORMAT& uncompressed_format, CompressionTextureType compression_type, CMP_FORMAT& compressed_format, int& num_channels)
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
			- There are two types of compression, one for BCx (Windows) and one for ASTC (Android).
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
		else if (compression_type == CompressionTextureType::ASTC)
		{
			switch (uncompressed_format)
			{
			case CMP_FORMAT_R_8:
				compressed_format = CMP_FORMAT_ASTC;  //Potential for ASTC or .basis file.
				break;
			case CMP_FORMAT_RG_8:
				compressed_format = CMP_FORMAT_ASTC;
				break;
			case CMP_FORMAT_RGB_888:
				compressed_format = CMP_FORMAT_ASTC;
				break;
			case CMP_FORMAT_RGBA_8888:
				compressed_format = CMP_FORMAT_ASTC;
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
		\param astc_block_size
		Get the opengl format for an astc block_size x block_size.
		e.g. "6" gives "GL_COMPRESSED_RGBA_ASTC_6x6"
		Valid: 4,5,6,8,10,12
		Only used if cmp_format is CMP_FORMAT_ASTC
		\return
		Nothing on success, Error string otherwise.
	*/
	[[nodiscard]] std::optional<std::string> GetGLTextureFormat(CMP_FORMAT compressed_cmp_format, bool is_srgb, GLenum& compressed_opengl_format, int astc_block_size = 6)
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
			///ASTC compression.
		case CMP_FORMAT_ASTC:
			switch (astc_block_size) {
			case 4:  compressed_opengl_format = is_srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4 : GL_COMPRESSED_RGBA_ASTC_4x4;   break;
			case 5:  compressed_opengl_format = is_srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5 : GL_COMPRESSED_RGBA_ASTC_5x5;   break;
			case 6:  compressed_opengl_format = is_srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6 : GL_COMPRESSED_RGBA_ASTC_6x6;   break;
			case 8:  compressed_opengl_format = is_srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8 : GL_COMPRESSED_RGBA_ASTC_8x8;   break;
			case 10: compressed_opengl_format = is_srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10 : GL_COMPRESSED_RGBA_ASTC_10x10; break;
			case 12: compressed_opengl_format = is_srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12 : GL_COMPRESSED_RGBA_ASTC_12x12; break;
			default: return "Invalid ASTC block size passed in for astc format";
			}
			break;
			//ETC2 compression
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
		Returns compressed data of the texture data passed in.
		\param texture_data
		The texture data, ensure data is tightly packed with the correct number of channels.
		While it is taken in as non-const reference, it will not be changed.
		\param is_srgb
		Whether texture file data is in srgb/gamma colorspace.
		\param TextureType
		What type to compress texture to.
		\param quality
		From 0 to 1.0. Much higher performance hit at higher levels.

		NOTE: For compression to ASTC (default on mobile platform), the data must be in 4 channels. i.e. add padding for 1-3 channels.
	*/
	CompressedTextureData CompressTexture(UnCompressedTextureData& texture_data, CompressionTextureType compression_type, float quality)
	{
		//TODO: Look into supporting 1-2 channel compression on mobile using ASTC compression or ETC2 RGB, and see if it's beneficial.
		//TODO: Look into compressing to .basis file, then transcoding during runtime to GPU supported texture (more compatibility, no need to worry about astc).
		if (quality < 0 || quality > 1.0) return CompressedTextureData("Invalid Compression Quality passed in, must be in range [0, 1]");
		if (!texture_data.isValid()) return CompressedTextureData("Invalid texture data passed in for compression");

		/*
			Process
			1. Get important parameters
			2. Compress texture
			3. Write data to CompressedTextureData and free textures used.
		*/
		int width = texture_data.width, height = texture_data.height, channels = texture_data.channels;
		bool is_srgb = texture_data.is_srgb;

		//==Get input and output formats.

		//Get texture format as CMP_FORMAT
		CMP_FORMAT input_format{};
		auto err_code = GetCMPUncompressedFormat(channels, input_format);
		if (err_code.has_value())
		{
			return CompressedTextureData("Invalid number of channels passed in, logic error?");
		}
		
		CMP_FORMAT output_format{};
		int num_channels{};
		err_code = GetTextureCompressionFormat(input_format, compression_type, output_format, num_channels);
		if (err_code.has_value())
		{
			return CompressedTextureData(err_code.value());
		}

		//TODO: May want to lower this to 4 or increase up to 12, depending on number of channels and type of texture (normal texture need more quality).
		constexpr int astc_block_size = 8;

		//compressed output_format, but as an opengl enum.
		GLenum compressed_type;
		err_code = GetGLTextureFormat(output_format, is_srgb, compressed_type, astc_block_size);
		if (err_code.has_value())
		{
			return CompressedTextureData(err_code.value());
		}

		//== Set input and output variables.
		CMP_Texture src_texture{};
		src_texture.dwSize = sizeof(src_texture);
		src_texture.dwPitch = width * channels; //size of row. assuming tightly packed data.
		src_texture.format = input_format;
		src_texture.dwWidth = width;
		src_texture.dwHeight = height;
		src_texture.pData = reinterpret_cast<CMP_BYTE*>(texture_data.data.data());
		src_texture.dwDataSize = width * height * channels;

		CMP_Texture dst_texture{};
		dst_texture.dwSize = sizeof(dst_texture);
		dst_texture.dwWidth = width;
		dst_texture.dwHeight = height;
		dst_texture.dwPitch = 0;
		dst_texture.format = output_format;
		//Specify block dimensions for astc to compress to.
		if (dst_texture.format == CMP_FORMAT_ASTC)
		{
			dst_texture.nBlockWidth = astc_block_size;
			dst_texture.nBlockDepth = 1;
			dst_texture.nBlockHeight = astc_block_size;
		}
		if (CMP_IsCompressedFormat(CMP_FORMAT_ASTC))
		{
			LOGD("yes");
		}
		dst_texture.dwDataSize = CMP_CalculateBufferSize(&dst_texture);
		std::vector<std::byte> output_data(dst_texture.dwDataSize);
		dst_texture.pData = reinterpret_cast<CMP_BYTE*>(output_data.data());


		//== Compression
		CMP_CompressOptions options = { 0 };
		options.dwSize = sizeof(options);
		options.fquality = quality;
		options.dwnumThreads = 0; //Let it select.
		options.bDisableMultiThreading = false;
		options.bDXT1UseAlpha = false; //Only RGB is converted to DXT1 for our case, so disable this because there's no alpha.
		options.bUseRefinementSteps = true;
		options.DestFormat = output_format;
		options.SourceFormat = input_format;
		//TODO: See if this is needed.
		if (is_srgb) options.useSRGBFrames = true;
		options.bUseCGCompress = true;
		options.nEncodeWith = CMP_GPU_OCL;


		CMP_ERROR cmp_status = CMP_ConvertTexture(&src_texture, &dst_texture, &options, nullptr);
		if (cmp_status != CMP_OK)
		{
			return CompressedTextureData("Failed to compress texture");
		}

		// Copy to result.
		// Note that these variables indicate a potential for future mipmap compression to be added.
		// As it is just 1 texture, just set level to 1.
		std::vector<std::pair<uint32_t, uint32_t>> mipmap_width_height{ {width, height} };
		std::vector<uint32_t> compressed_data_size{ static_cast<uint32_t>(output_data.size()) };
		return CompressedTextureData(
			std::move(mipmap_width_height),
			std::move(compressed_data_size),
			1,
			compressed_type,
			is_srgb,
			channels,
			std::move(output_data)
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
		std::vector<unsigned char> file_data = EngineFramework::AssetManagerSystem::GetDataFromFile(file_path);
		if (file_data.empty())
			return std::string("Failed to read texture file: ") + file_path.string();

		//RAII management of stbi data.
		auto stbi_data_free = [](unsigned char* data) {
			if (data) stbi_image_free(data);
			};
		std::unique_ptr<unsigned char, decltype(stbi_data_free)> stbi_data{
			stbi_load_from_memory(
			file_data.data(),
			static_cast<int>(file_data.size()),
			&width, &height, &bytesPerPix, 0), stbi_data_free };
		if (!stbi_data)
			return std::string("Unable to process texture data: ") + file_path.string();


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

	/*
		Uploads a compressed texture.
		If failed, an error code is returned and the texture is not created.

		\param [out] created_texture
		ID of the created texture is written here
		\param file_path
		File path of the texture
	*/
	[[nodiscard]] std::optional<std::string> LoadCompressedTexture(GLuint& loaded_texture, const std::filesystem::path& file_path)
	{
		loaded_texture = 0;

		//== Load compressed data from file
		CompressedTextureData compressed_texture{};
		auto err_code = compressed_texture.ReadDataFromFile(file_path);
		if (err_code.has_value())
		{
			return err_code;
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, compressed_texture.mip_levels - 1); //Ensure gpu only uses what was uploaded.

		//Set texture to repeat vertically/horizontally if out-of-bounds.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//Texture swizzling R and B to account for flipped texture during compression for RGB and RGBA.
		switch (compressed_texture.compressed_type)
		{
		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			break;
			//Otherwise no need to swizzle.
		}
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
	[[nodiscard]] std::optional<std::string> GetGLTextureFormat(GLenum& current_format, GLint& internal_format, int channels, bool is_srgb)
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
}


#ifdef PLATFORM_WINDOWS

[[nodiscard]] std::optional<std::string> Scene::CompressAndWriteTexture(const std::filesystem::path& file_path, const std::filesystem::path& folder_BCx, const std::filesystem::path& folder_ASTC)
{
	if (folder_BCx.empty() && folder_ASTC.empty())
	{
		return "At least one compression type should be selected.";
	}




	/*
	*	Load texture data from file
		\param requested_channel
		Texture data will be loaded with that many channels.
		Set to 0 to use original texture channels.
		\param [out] loaded_texture
		Where loaded texture will be written to.
		\return
		Nothing on success, error code otherwise.
	*/
	auto LoadTextureData_lambda = [&file_path](UnCompressedTextureData& loaded_texture, uint32_t requested_channel = 0)
		-> std::optional<std::string>
		{
			//==Load texture data into a usable format for CompressTexture.
			stbi_set_flip_vertically_on_load(true);
			std::vector<unsigned char> file_data = EngineFramework::AssetManagerSystem::GetDataFromFile(file_path);
			if (file_data.empty())
				return std::string("Failed to read texture file: ") + file_path.string();

			//RAII management of stbi data.
			auto stbi_data_free = [](unsigned char* data) {
				if (data) stbi_image_free(data);
				};
			int width{}, height{}, channels{};
			std::shared_ptr<unsigned char> stbi_data{
				stbi_load_from_memory(
				file_data.data(),
				static_cast<int>(file_data.size()),
				&width, &height, &channels, requested_channel), stbi_data_free };
			//If requested_channel was set, then use that.
			channels = (requested_channel != 0)? requested_channel : channels;
			if (!stbi_data)
				return "Unable to process texture data" + file_path.string();

			bool is_srgb = (file_path.string().find("Albedo") != std::string::npos);

			//Copy to out param.
			loaded_texture = UnCompressedTextureData(stbi_data, width, height, channels, is_srgb);
			//Logically this should not ever happen, otherwise it could be due to pre-checks failing to validate data correctly.
			if (!loaded_texture.isValid()) return "Unknown error when reading/parsing texture data";

			return {};
		};

	/*
	 	Compresses texture data, then writes to appropriate file.
	    \param uncompressed_texture
	    - Load data into this before passing.
		- While taken as a non-const reference, it is not changed.
		\param out_dir
		The directory to store in
		\param file_name
		The stemmed file name, i.e. without folder prefix or extension postfix.
		\return
		Nothing on success, error string otherwise.
	*/
	auto CompressAndWriteTextureDir_lambda = [](UnCompressedTextureData& uncompressed_texture, const std::filesystem::path& out_dir, const std::string& file_name, CompressionTextureType compression_type, float quality) -> std::optional<std::string> {
		/*
			 Custom extensions used for each compression type.
			 This denotes that the texture is compressed as X type, but stored in a custom file that does not follow BCx or ASTC standard headers.
		*/
		std::string extension{};
		switch (compression_type)
		{
		case CompressionTextureType::BC_X:
			extension = ".compiled_bc";
			break;
		case CompressionTextureType::ASTC:
			extension = ".compiled_astc";
			break;
		default:
			return "Invalid Compression Type";
		}

		//==Compress texture
		CompressedTextureData compressed_texture(std::move(CompressTexture(uncompressed_texture, compression_type, quality)));
		if (!compressed_texture.isValid())
			return compressed_texture.error_str;

		//==Write to file in designated folder with the custom extension and same file name.
		auto err_code = compressed_texture.WriteDataToFile(out_dir / (file_name + extension));

		if (err_code.has_value()) return err_code;

		return {};
		};

	/*
		Compress data to respective types.
		Write results to respective folders with the appropriate extensions and the same file name.
	*/
	std::string file_name = file_path.stem().string(); //Just get the file name by itself.
	float quality = 0.1f; //0 to 1, where 1 is the highest quality with the longest compression time. Processing time may scale exponentially?
	std::string full_err_code{}; //Accumulate errors, don't return immediately on error but let the other compression type also run.
	
	
	/*
		BCx -> Can pass in original number of channels
		ASTC -> Must load texture as 4 channels, even if it's 1-3 channels it should be padded.
	*/
	if (!folder_BCx.empty())
	{
		UnCompressedTextureData uncompressed_texture;
		auto file_err = LoadTextureData_lambda(uncompressed_texture);
		if (file_err) return file_err; //Immediate stop if texture file cannot be read properly.

		//Check if folderpath is invalid, and tell the user instead of silently failing. 
		if (!std::filesystem::exists(folder_BCx) || !std::filesystem::is_directory(folder_BCx))
		{
			full_err_code += "Invalid directory passed for CompressAndWrite: " + folder_BCx.string() + " BCx not compressed.";
		}
		else
		{
			auto err_code = CompressAndWriteTextureDir_lambda(uncompressed_texture, folder_BCx, file_name, CompressionTextureType::BC_X, quality);
			if (err_code) full_err_code += err_code.value();
		}
	}
	if (!folder_ASTC.empty())
	{
		UnCompressedTextureData uncompressed_texture;
		auto file_err = LoadTextureData_lambda(uncompressed_texture, 4);
		if (file_err) return file_err; //Immediate stop if texture file cannot be read properly.

		if (!std::filesystem::exists(folder_ASTC) || !std::filesystem::is_directory(folder_ASTC))
		{
			full_err_code += "Invalid directory passed for CompressAndWrite: " + folder_ASTC.string() + " ASTC not compressed.";
		}
		else
		{
			auto err_code = CompressAndWriteTextureDir_lambda(uncompressed_texture, folder_ASTC, file_name, CompressionTextureType::ASTC, quality);
			if (err_code) full_err_code += err_code.value();
		}
	}
	if (!full_err_code.empty()) return full_err_code; //Error
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
		- Compressed: ".compiled_astc", ".compiled_bc"

		Use extensions to determine what type the image is.
	*/
	constexpr std::array<const char*, 3> uncompressed_extensions{ ".png", ".jpg", ".jpeg" };
	constexpr std::array<const char*, 4> compressed_extensions{ ".compiled_astc", ".compiled_bc" };
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
		error_code = LoadCompressedTexture(loaded_texture, filename);
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