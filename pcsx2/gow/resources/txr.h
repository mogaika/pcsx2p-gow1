#pragma once

#include <unordered_map>
#include <GL/GL.h>

namespace gow {

class Texture {
protected:
	union {
		GLuint *images;
		GLuint image;
    };
	int imagesCount;

	GLuint &getImageRef(int index);
public:
	Texture(u32 offset);
	~Texture();

	GLuint GetGl(int index) { return getImageRef(index); }
};

class TextureManager {
protected:
	std::unordered_map<u32, Texture*> textures;
public:
    TextureManager(){};

    void HookInstanceCtor();
    void HookInstanceDtor();
};

} // namespace gow
