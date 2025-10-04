#ifndef TEXTURE_COMPONENT
#define TEXTURE_COMPONENT
struct TextureComponent
    {
	    xresource::full_guid texture_guid; // Full GUID for the texture resource
		bool isNormalMap = false; // Flag to indicate if this texture is a normal map
    };
#endif // !TEXTURE_COMPONENT
