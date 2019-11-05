#pragma once

#include <unordered_map>
#include <GL/GL.h>

#include "gow/utils.h"

// #define GOW_TEXTURE_DEBUG

namespace gow {

namespace raw {

#pragma pack(push, 1)
struct stGfx {
	stIInstance instance;
    gap_t _gap0[0x4];
    u16 width;
    u16 heightTotal;
    gap_t _gap18[2];
	u16 height;
    u16 imagesCount;
    u16 transferFormat;
    u16 textureFormat;
	u16 currentImageIndex;
    gap_t _gap24[6];
    u8 bpp;
    gap_t _gap2b[4];
	u8 bytesPerRow;
    u8 flags;
    gap_t _gap31[7];
    u32 pDmaGfxDataOffset;
    u32 animInstanceOffset;
    gap_t _gap[0x18];

	template <typename T> T *gfxData(u32 index) { return pmemz<T>(pDmaGfxDataOffset + 0x10 + index * ((u32(height) * u32(width) * u32(bpp))/ 8 + 0x10)); }

	// not works for pal 
	bool isSwizzled() { return !(flags & 1); }
};
static_assert(sizeof(stGfx) == 0x58, "gfx size");

struct stTxr {
    stIInstance instance;
	u32 gfxInstanceOffset;
    u32 palInstanceOffset;
	u32 subTxrInstanceOffset;
	gap_t _gap1c[4];
	u64 flags1;
	u64 flags2;
	gap_t _gap30[0x28];
	gap_t _gap5c[0x18];

	stGfx *gfxInstance() { return pmemz<stGfx>(gfxInstanceOffset); }
    stGfx *palInstance() { return pmemz<stGfx>(palInstanceOffset); }
	stTxr *subTxrInstance() { return pmemz<stTxr>(subTxrInstanceOffset); }
};
static_assert(sizeof(stTxr) == 0x70, "txr size");
#pragma pack(pop)

} // namespace raw

class Texture {
protected:
	union {
		GLuint *_images;
		GLuint _image;
    };
	int imagesCount;
    raw::stTxr *img;
	GLsizei width, height;
	float yScale;
    bool generated;
	char name[32];

	GLuint &getImageRef(int index) { return imagesCount == 1 ? _image : _images[index]; };
    void generateTextures(raw::stGfx *pal);
    void generateMip(raw::stGfx *pal);

public:
	Texture(u32 offset, char *name = nullptr);
	~Texture();

	GLuint GetAnimatedCurrentGL() { return getImageRef(img->gfxInstance()->currentImageIndex); }
	GLuint GetGl(int index) { return getImageRef(index); }
    char *GetName() { return name; }
    float GetYScale() { return yScale; }
    raw::stTxr *GetRaw() { return img; }
};

class TextureManager {
protected:
	std::unordered_map<u32, Texture*> textures;
public:
    TextureManager(){};

	Texture *GetTexture(u32 offset);

    void HookInstanceCtor();
    void HookInstanceDtor();
};

} // namespace gow
