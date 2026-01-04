import os.path
import string
import struct
from decimal import Decimal

import pygameextra as pe

DEBUG = True
FONT = "../assets/fonts/Magic Red.ttf"
LETTERS = list(string.digits + string.ascii_uppercase + '$')
ATLAS_LIST = [
    {
        "file": "Cat",
        "save": "catAnimationsInfo.h",
        "name_upper": "CAT_ANIMATIONS",
        "animations_name": "catAnimations",
        "texture_size_name": "catAtlasTextureSize",
        "sx": 128,
        "sy": 128,
    },
    {
        "file": "Letters",
        "save": "letterAnimationsInfo.h",
        "name_upper": "LETTER_ANIMATIONS",
        "animations_name": "letterAnimations",
        "texture_size_name": "letterAtlasTextureSize",
        "sy": 26,
        "gen": "generate_font_map",
        "sort_func": lambda k: LETTERS.index(k) if k in LETTERS else 9999,
    },
    {
        "file": "UI",
        "save": "uiAnimationsInfo.h",
        "name_upper": "UI_ICONS",
        "animations_name": "uiIcons",
        "texture_size_name": "uiAtlasTextureSize",
        "sx": 16,
        "sy": 16
    }
]
h_file = """#ifndef {name_upper}_INFO_H
#define {name_upper}_INFO_H
#include "sprite.h"

const std::vector<AnimationInfo> {animations_name} = {{
{animations}
}};

enum {file}Animation {{
{enum}
}};

static constexpr TextureSize {texture_size_name} = {{
    .width = {texture_width},
    .height = {texture_height},
}};

#endif //{name_upper}_INFO_H"""

os.makedirs(INCLUDE_DIRECTORY := os.path.join('..', 'assets', 'sprites', 'include'), exist_ok=True)

# Initialize PGE
print("Using PGE version", pe.__version__)
pe.init((0, 0))

def generate_font_map(ainfo: dict):
    objs = []
    renames = {
        '$': 'DOLLAR'
    }

    sy = ainfo['sy']

    for l in LETTERS:
        text = pe.Text(l, FONT, sy)
        objs.append((l, text.obj))

    chars = {}

    mw = max(obj.get_width() for _, obj in objs)

    for l, obj in objs:
        surface = pe.Surface((mw, sy))
        with surface:
            pe.display.blit(obj, (mw / 2 - obj.get_width() / 2, 0))
        chars[renames.get(l, l)] = pe.Sheet(surface, pe.SheetHorizontal(mw, sy))

    ainfo['sx'] = mw

    return chars



for atlas_info in ATLAS_LIST:
    animations_dir = os.path.join('..', 'assets', 'sprites', atlas_info['file'])
    sheets = {}
    sort_func = atlas_info.get('sort_func', None)
    if atlas_info.get('gen'):
        sheets = globals()[atlas_info['gen']](atlas_info)
        sx = atlas_info.get('sx', 32)
        sy = atlas_info.get('sy', 32)
    else:
        sx = atlas_info.get('sx', 32)
        sy = atlas_info.get('sy', 32)
        for animation_file in os.listdir(animations_dir):
            if not animation_file.endswith('.png'):
                continue
            sheet = pe.Sheet(os.path.join(animations_dir, animation_file), pe.SheetHorizontal(sx, sy))
            sheets[animation_file.split('.')[0]] = sheet
    atlas = pe.Atlas.from_sheets(sheets)

    # Save the PNG file with the atlas
    atlas.surface.save_to_file(f"{animations_dir}.png")

    # Create a RAW RGBA file with the atlas
    rgba_bytes = pe.pygame.image.tobytes(atlas.surface.surface, 'RGBA', False)
    with open(f"{animations_dir}.rgba", 'wb') as f:
        f.write(rgba_bytes)

    animations_str = ""
    enum_str = []
    for i, (key, mapping) in enumerate(sorted(atlas.mappings.items(), key=lambda _: sort_func(_[0]) if sort_func else _[0])):
        animations_str += f"    // Animation: {key} {i}\n"
        animations_str += f"    {{{i}, {len(mapping)}, {{\n"
        for rect in mapping:
            size = (
                Decimal(rect[2]) / Decimal(atlas.surface.width),
                Decimal(rect[3]) / Decimal(atlas.surface.height),
            )
            uv0 = (
                Decimal(rect[0]) / Decimal(atlas.surface.width),
                Decimal(rect[1]) / Decimal(atlas.surface.height),
            )
            uv1 = (
                uv0[0] + size[0],
                uv0[1] + size[1],
            )
            animations_str += f"        {{{uv0[0]}, {uv0[1]}, {uv1[0]}, {uv1[1]}}},\n"
        animations_str += "    }},\n\n"
    for i, key in enumerate(sorted(atlas.mappings.keys(), key=lambda _: sort_func(_) if sort_func else _)):
        enum_str.append(f"    {atlas_info['file'].upper()}_{key.upper()} = {i}")

    with open(os.path.join(INCLUDE_DIRECTORY, atlas_info['save']), 'w') as f:
        f.write(h_file.format(
            file=atlas_info['file'].lower().capitalize(),
            enum=',\n'.join(enum_str),
            name_upper=atlas_info['name_upper'],
            animations_name=atlas_info['animations_name'],
            texture_size_name=atlas_info['texture_size_name'],
            animations=animations_str,
            texture_width=atlas.surface.width,
            texture_height=atlas.surface.height,
        ))
