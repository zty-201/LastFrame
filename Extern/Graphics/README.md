# Texture 
Author: Joel Lee

Textures are split into compressed and uncompressed textures.
When loading, compressed textures are prioritized first before uncompressed versions.
Uncompressed versions will be loaded if compressed versions aren't present/fail to load.

Upon first load, textures are compressed into BCx and ETC2 types and stored in respective subfolders.
Textures mentioned in do_not_compress.meta will not be compressed.
- Can be due to "Unable to compress" or "Leave as uncompressed for the highest quality possible".
- Textures mentioned in this folder is typically due to being 1-2 channel textures which fail to compress into ETC2 on mobile (no valid compression format yet).

It is safe to delete Texture/compressed_* folders as long as do_not_compress.meta file is also deleted to ensure full regeneration of compressed textures.
Delete both folders + meta if the textures are corrupted (or just meta + the last few textures compressed), e.g. if the game is closed while compressing new textures.

Do note that texture compression to ASTC can take very long. 

## Compressed
Compressed textures are split into two parts
- BCx: For Windows platform
- ASTC: For Android platform.

CompressAndWrite==
Quality can be specified on the scale of 0 to 1 in , where time to compress grows exponentially(?).
For ASTC, block sizes come in 4x4, 5x5, ..., 12x12. The larger the block size the smaller the compressed texture but the higher the quality loss.

Only one type is loaded, although both are generated on first load.

No mipmaps are generated for these due to implementation issues.

## Uncompressed
Mipmaps are generated for these textures.

## Guid.meta file
All versions of the texture file share the same guid, as they have the same name. 

