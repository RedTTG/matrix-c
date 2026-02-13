#shader vertex
#version 300 es

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

uniform mediump vec2 uPos;
uniform mediump vec2 uSize;
uniform mediump vec4 uUV;       // u0,v0,u1,v1
uniform mediump vec2 uScreen;   // screen width/height
uniform mediump vec2 uOffset;   // bottom-right cat zone offset
uniform mediump float uScale;   // uniform scale

out vec2 vUV;
out vec2 vLocalUV; // pass the sprite-local UV (0..1) so fragment can test cutoffs in sprite space

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
uniform mediump vec4 uColor; // tint, default (1,1,1,1)

uniform mediump vec4 uUV; // sprite rect in atlas: u0,v0,u1,v1
// cutoff: (left, bottom, right, top) in sprite-local 0..1 space
// vLocalUV is the sprite-local UV where y=0 is bottom and y=1 is top
uniform mediump vec4 uCutoff; // x:left%, y:bottom%, z:right%, w:top% (0.0 - 1.0, relative to the sprite)

void main() {
    // Determine sprite-local UV robustly:
    // Prefer the provided vertex-local UV (vLocalUV) if it looks like 0..1, otherwise remap vUV using uUV.
    vec2 localUV = vLocalUV;
    if (any(lessThan(localUV, vec2(0.0))) || any(greaterThan(localUV, vec2(1.0)))) {
        vec2 rectMin = uUV.xy;
        vec2 rectMax = uUV.zw;
        vec2 rectSize = rectMax - rectMin;
        rectSize = max(rectSize, vec2(1e-6));
        localUV = (vUV - rectMin) / rectSize;
    }

    // Use sprite-local UV (localUV in 0..1) for cutoff checks so this only affects the current sprite
    vec4 cut = clamp(uCutoff, 0.0, 1.0);

    // cut = (left, bottom, right, top)
    // Discard early if the fragment is outside the sprite-local visible rect
    if (localUV.x < cut.x || localUV.x > cut.z || localUV.y < cut.y || localUV.y > cut.w) {
        discard;
    }

    // Sample from the spritesheet using absolute UV (vUV)
    vec4 tex = texture(uTexture, vUV);
    fragColor = tex * uColor;
    if (fragColor.a < 0.01)
        discard;
}
