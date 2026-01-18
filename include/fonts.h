#ifndef FONTS_H
#define FONTS_H

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#else
#include <glad.h>
#endif

#include <vector>
#include <boost/mpl/int.hpp>

struct CharacterInfo {
    unsigned int xOffset;
    unsigned int yOffset;
    unsigned int width;
    unsigned int height;
};

struct FontInfo {
    const int width, height, size, characterCount;
    const CharacterInfo* characterInfoList;
};

struct FontAtlas {
    GLuint glyphTexture, glyphBuffer;
    float atlasWidth;
    float atlasHeight;

    void destroy() const;
};

FontAtlas *createFontTextureAtlas(const unsigned char *source, const FontInfo *fontInfo);

#endif //FONTS_H
