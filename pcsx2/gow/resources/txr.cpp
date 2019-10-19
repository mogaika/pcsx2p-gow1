#include "PrecompiledHeader.h"

#include "gow/resources/utils.h"
#include "gow/resources/txr.h"

using namespace gow;

Texture::Texture(u32 offset) {
	auto gfx = rmem<u32>(offset + 0x10);
    auto pal = rmem<u32>(offset + 0x14);
    auto subTxr = rmem<u32>(offset + 0x18);

	DevCon.WriteLn("gow: texture %x %x %x", gfx, pal, subTxr);
}

Texture::~Texture() {
	if (imagesCount > 1) {
		delete[] images;
    }
}

GLuint &Texture::getImageRef(int index) {
    if (imagesCount == 1) {
        return image;
    } else {
        return images[index];
    }
}

void TextureManager::HookInstanceCtor() {
    auto offset = cpuRegs.GPR.n.v0.UL[0];
    auto texture = new Texture(offset);
	textures[offset] = texture;
}

void TextureManager::HookInstanceDtor() {
    auto offset = cpuRegs.GPR.n.v0.UL[0];
	auto texture = textures.find(offset);
	delete texture->second;
	textures.erase(texture);
}

