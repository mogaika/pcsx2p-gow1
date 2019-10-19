#include "PrecompiledHeader.h"

#include "gow/resources/txr.h"
#include "gow/gow.h"
#include "gow/gl.h"

using namespace gow;

Texture::Texture(u32 offset, char *name) :
	imagesCount(0), generated(false) {
	img = pmem<stTxr>(offset);
	
	auto gfx = img->gfxInstance();
  
	width = gfx->width;
	height = gfx->height;

	imagesCount = gfx->imagesCount;

	if (imagesCount > 1) {
		_images = new GLuint[imagesCount];
	}

	auto pal = img->palInstance();
	if (pal) {
        generateTextures(pal);
    }

	DevCon.WriteLn("gow: created texture %s %x (%d)", name, img, imagesCount);
}

u32 palSwizzle(u32 index) {
    static u32 remap[4] = {0, 2, 1, 3};
    u32 blockid = index / 8;
    u32 blockpos = index % 8;
    return blockpos + (remap[blockid % 4] + (blockid / 4) * 4) * 8;
}

u32 gfxUnswizzle(u32 x, u32 y, u32 width) {
    u32 blocklocation = (y & ((-1) ^ 0xf)) * width + (x & ((-1) ^ 0xf)) * 2;
    u32 swapselector = (((y + 2) >> 2) & 0x1) * 4;
    u32 posy = (((y & ((-1) ^ 3)) >> 1) + (y & 1)) & 0x7;
	u32 columnlocation = posy * width * 2 + ((x + swapselector) & 0x7) * 4;
	u32 bytenum = ((y >> 1) & 1) + ((x >> 2) & 2);
	return blocklocation + columnlocation + bytenum;
}

void Texture::generateTextures(stGfx *pal) {
    auto gfx = img->gfxInstance();

    DevCon.WriteLn("gow: generating texture for %x", img);
    // TODO: handle multiple pal case
    u32 palleteSize = pal->width * pal->height;
    auto pallete = new u32[palleteSize];
    /*
	auto pallete = new u32[256];
	for (u32 i = 0; i < 256; i++) {
        pallete[i] =
			(((i % 16) * 16) << 0) |
			((((i / 16) % 16) * 16) << 8) |
			(((((i / 16) / 16) % 16) * 16) << 16) |
			(0xff << 24);
	}
	*/

    u32 pixelsCount = width * height;
    auto imageData = new u32[pixelsCount];

	
	u32 *paldata = pal->gfxData<u32>(0);
	if (pal->height != 2) {
        for (u32 i = 0; i < palleteSize; i++) {
            pallete[palSwizzle(i)] = paldata[i];
        }
	} else {
        memcpy(pallete, paldata, palleteSize);
    }
	
	core->Window()->AttachContext();
	glGenTextures(imagesCount, &getImageRef(0));
    core->Renderer()->CheckErrors("gen texture");

	for (int i = 0; i < imagesCount; i++) {
        u32 *pPixel = imageData;
        u8 *pIndex = gfx->gfxData<u8>(i);

        if (gfx->bpp == 4) {
            for (u32 i = 0; i < pixelsCount / 2; i++) {
				u8 index = *(pIndex++);
				*(pPixel++) = pallete[index & 0xf];
                *(pPixel++) = pallete[(index >> 4) & 0xf];
            }
		} else {
			if (gfx->isSwizzled()) {
                DevCon.WriteLn("index: %x, gfx: %x, txr: %x", pIndex, gfx, img);
                for (GLsizei y = 0; y < height; y++) {
                    for (GLsizei x = 0; x < width; x++) {
                        *(pPixel++) = pallete[pIndex[gfxUnswizzle(x, y, gfx->width)]];
					}
                }
			} else {
                for (u32 i = 0; i < pixelsCount; i++) {
                    *(pPixel++) = pallete[*(pIndex++)];
                }
			}
		}

		glBindTexture(GL_TEXTURE_2D, getImageRef(i));
        core->Renderer()->CheckErrors("bind texture");
		if ((img->flags1 >> 16 & 0xff) == 0x51) {
			// ui textures
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        core->Renderer()->CheckErrors("glTexImage2D");

        glGenerateMipmap(GL_TEXTURE_2D);
	}

	generated = true;

	glBindTexture(GL_TEXTURE_2D, 0);

	auto subImg = img->subTxrInstance(); // we need to pass subtexture to mipmaps?
	if (subImg) {
        auto subTexture = managers.texture->GetTexture(subImg->instance.offset());
		if (subTexture) {
			subTexture->generateMip(pal);
        } else {
			DevCon.Error("gow: wasn't able to find subtexture %x", revmem(img));
		}
	}

    core->Window()->DetachContext();
	delete[] pallete;
}

void Texture::generateMip(stGfx *pal) {
	if (!img->palInstanceOffset) {
		generateTextures(pal);
	}
}

Texture::~Texture() {
	if (imagesCount > 0) {
		if (generated) {
			glDeleteTextures(imagesCount, &getImageRef(0));
        }
		if (imagesCount > 1) {
			delete[] _images;
		}
    }
}

Texture *gow::TextureManager::GetTexture(u32 offset) {
    auto texture = textures.find(offset);
	return (texture == textures.end()) ? nullptr : texture->second;
}

void TextureManager::HookInstanceCtor() {
    auto offset = cpuRegs.GPR.n.v0.UL[0];
    auto name = pmemz<char>(cpuRegs.GPR.n.s3);
    auto texture = new Texture(offset, name);
    textures.insert(std::pair<u32, Texture*>(offset, texture));
}

void TextureManager::HookInstanceDtor() {
    auto texture = textures.find(cpuRegs.GPR.n.a1.UL[0]);
	if (texture == textures.end()) {
        DevCon.Error("Wasn't able to find texture to remove: %x", cpuRegs.GPR.n.a1.UL[0]);
	}
	delete texture->second;
	textures.erase(texture);
}

