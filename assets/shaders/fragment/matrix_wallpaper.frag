#version 330 core
out vec4 fragColor;

uniform sampler2D u_AtlasTexture;
uniform sampler2D u_WallpaperTexture;
uniform float u_BaseColor;

in float v_ColorOffset;
flat in int v_Spark;
in vec2 v_TexCoord;
in vec2 v_ScreenCoord;

void main()
{
    float glyphColor = texture(u_AtlasTexture, v_TexCoord).r;

    vec3 wallpaperColor = texture(u_WallpaperTexture, v_ScreenCoord).rgb;

    fragColor = vec4(wallpaperColor * glyphColor, glyphColor);
}