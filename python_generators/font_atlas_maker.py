import os.path
import struct

import pygameextra as pe

FONT = "JiyunoTsubasa.ttf"
NAME = "matrix_font"
INFO = "matrixFontInfo"
CHARACTERS = "ﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ012345789Z:・.\"=*+-<>¦｜"
FONT_SIZE = 100
DEBUG = False
h_file = """
#ifndef FONT_ATLAS_INFO_H
#define FONT_ATLAS_INFO_H
#include "fonts.h"

constexpr CharacterInfo {}[] = {{
    {}
}};

static constexpr FontInfo {} = {{
    .width = {},
    .height = {},
    .size = {},
    .characterCount = {},
    .characterInfoList = {}
}};

#endif //FONT_ATLAS_INFO_H

"""

os.makedirs(INCLUDE_DIRECTORY := os.path.join('..', 'assets', 'fonts', 'include'), exist_ok=True)


class CustomMetricsHandler(pe.SheetHandler):
    def __init__(self, metrics):
        x = 0
        self.mapping = []
        for c in metrics:
            self.mapping.append((x, 0, c[4], FONT_SIZE))
            x += c[4]

    def map(self, surface):
        pass


# Initialize PGE
print("Using PGE version", pe.__version__)
pe.init((0, 0))

text = pe.Text(CHARACTERS, os.path.join('..', 'assets', 'fonts', FONT), FONT_SIZE, colors=[pe.colors.white, None])
metrics = text.font.metrics(text.text)

# Pack the characters into a texture atlas
sheet = pe.Sheet(text.obj, CustomMetricsHandler(metrics))
atlas = pe.Atlas.from_sheets({
    "_": sheet,
})

# Create a PNG file with the atlas and debug text
if DEBUG:
    with atlas.surface:
        c = 0
        change = 255 // len(atlas.mappings['_'])

        pe.fill.interlace(pe.colors.green, 5)

        for i, mapping in enumerate(atlas.mappings['_']):
            pe.draw.rect((c, c, c), mapping, 1)
            text = pe.Text(str(i), font_size=20, colors=[pe.colors.white, (c, c, c)])
            text.rect.topleft = mapping[:2]
            text.display()
            c += change

    atlas.surface.save_to_file(f"atlas_debug.png")

with open(os.path.join('..', 'assets', 'fonts', f'{NAME}.raw'), 'wb') as file:
    for y in range(atlas.surface.height):
        for x in range(atlas.surface.width):
            r, g, b, a = atlas.surface.get_at((x, y))
            i = (r + g + b) // 3
            intensity = int(i * (a / 255))

            file.write(struct.pack('B', intensity))


characterListName= f'{INFO}CharacterList'
with open(os.path.join(INCLUDE_DIRECTORY, f'{NAME}_info.h'), 'w') as file:
    file.write(
        h_file.format(
            characterListName,
            ',\n\t'.join(
                f'{{{mapping_element[0]},'
                f'{atlas.surface.height - mapping_element[1] - FONT_SIZE},'
                f'{mapping_element[2]},'
                f'{mapping_element[3]}}}'
                for mapping_element in atlas.mappings['_']
            ),
            INFO, atlas.surface.width, atlas.surface.height, FONT_SIZE, len(CHARACTERS), characterListName
        )
    )
3