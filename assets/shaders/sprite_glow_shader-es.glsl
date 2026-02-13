#shader vertex
#version 300 es

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

uniform mediump vec2 uPos;
uniform mediump vec2 uSize;
uniform mediump vec4 uUV;
uniform mediump vec2 uScreen;
uniform mediump vec2 uOffset;
uniform mediump float uScale;

out vec2 vUV;
out vec2 vLocalUV; // sprite-local 0..1 UV for cutoff checks

void main() {
    vec2 world = uPos + (aPos * uSize) * uScale + uOffset;
    float ny = 1.0 - (world.y / uScreen.y);
    vec2 ndc = vec2(
    (world.x / uScreen.x) * 2.0 - 1.0,
    ny * 2.0 - 1.0
    );
    gl_Position = vec4(ndc, 0.0, 1.0);

    vUV = mix(uUV.xy, uUV.zw, aUV);
    vLocalUV = aUV;
}

#shader fragment
#version 300 es
precision mediump float;

in vec2 vUV;
in vec2 vLocalUV;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform mediump vec4 uColor;
uniform mediump vec2 uTexSize;     // pass atlas width/height
uniform mediump float uScale;

uniform mediump vec4 uUV; // sprite rect in atlas: u0,v0,u1,v1
// cutoff: (left, bottom, right, top) in sprite-local 0..1 space
// localUV is computed from vUV and uUV so it's always sprite-local
uniform mediump vec4 uCutoff; // x:left, y:bottom, z:right, w:top

void main() {
    // Determine sprite-local UV robustly:
    vec2 localUV = vLocalUV;
    if (any(lessThan(localUV, vec2(0.0))) || any(greaterThan(localUV, vec2(1.0)))) {
        vec2 rectMin = uUV.xy;
        vec2 rectMax = uUV.zw;
        vec2 rectSize = rectMax - rectMin;
        rectSize = max(rectSize, vec2(1e-6));
        localUV = (vUV - rectMin) / rectSize;
    }

    // Early discard using sprite-local UV so cutoff only affects current sprite and avoids texture fetches
    vec4 cut = clamp(uCutoff, 0.0, 1.0);
    if (localUV.x < cut.x || localUV.x > cut.z || localUV.y < cut.y || localUV.y > cut.w) {
        discard;
    }

    vec4 tex = texture(uTexture, vUV);

    float a = tex.a;
    if (a > 0.9) {
        fragColor = tex * uColor;
        return;
    }

    vec2 px = (1.0 * uScale) / uTexSize;

    float edge =
    texture(uTexture, vUV + vec2(-px.x, 0.0)).a +
    texture(uTexture, vUV + vec2(px.x, 0.0)).a +
    texture(uTexture, vUV + vec2(0.0, -px.y)).a +
    texture(uTexture, vUV + vec2(0.0, px.y)).a;

    if (edge > 0.1 && a < 0.1) {
        fragColor = vec4(1.0 - uColor.rgb, uColor.a);
        return;
    }

    if (a < 0.01)
    discard;

    fragColor = tex * uColor;
}
