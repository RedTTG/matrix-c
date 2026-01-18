#version 300 es
precision highp float;
precision highp int;

out vec4 fragColor;

uniform sampler2D u_AtlasTexture;
uniform float u_BaseColor;

in float v_ColorOffset;
flat in int v_Spark;
in vec2 v_TexCoord;

vec3 hueToRgb(float hue) {
    float r = abs(hue * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(hue * 6.0 - 2.0);
    float b = 2.0 - abs(hue * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

void main()
{
    float glyphColor = texture(u_AtlasTexture, v_TexCoord).r;

    vec3 color;

    if (v_Spark == 0) {
        color = vec3(1.0, 1.0, 1.0);
    } else {
        float hue = mod(u_BaseColor + v_ColorOffset, 1.0);
        color = hueToRgb(hue);
    }

    fragColor = vec4(color * glyphColor, glyphColor);
}

