#pragma once

#include <unordered_map>
#include <GL/GL.h>

#include "gow/utils.h"

// #define GOW_TEXTURE_DEBUG

namespace gow {

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
    gap_t _gap22[8];
    u8 bpp;
    gap_t _gap2b[5];
    u8 flags;
    gap_t _gap31[7];
    u32 gfxDataOffset;
    u32 animInstanceOffset;
    gap_t _gap[0x18];

	template <typename T> T *gfxData(u32 index) { return pmemz<T>(gfxDataOffset + (index + 1) * 0x10 + (index * height * width)); }

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

class Texture {
protected:
	union {
		GLuint *_images;
		GLuint _image;
    };
	int imagesCount;
    stTxr *img;
	GLsizei width, height;
    bool generated;
	char name[32];

	GLuint &getImageRef(int index) { return imagesCount == 1 ? _image : _images[index]; };
    void generateTextures(stGfx *pal);
    void generateMip(stGfx *pal);
public:
	Texture(u32 offset, char *name = nullptr);
	~Texture();

	GLuint GetGl(int index) { return getImageRef(index); }
    char *GetName() { return name; }
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
