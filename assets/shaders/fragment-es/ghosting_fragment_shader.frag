#version 300 es
precision highp float;

uniform sampler2D u_textureC;
uniform sampler2D u_textureP;
uniform float u_previousFrameOpacity;
in vec2 v_texcoord;
out vec4 fragColor;

void main() {
    vec4 currentFrame = texture(u_textureC, v_texcoord);
    vec4 previousFrame = texture(u_textureP, v_texcoord);

    // Blend current frame with faded previous frame for ghosting trail effect
    // The current frame is fully opaque, previous frame is faded based on opacity
    vec4 fadedPrevious = previousFrame * u_previousFrameOpacity;

    // Use max to ensure the brightest pixels show through (additive-like blending)
    fragColor = max(currentFrame, fadedPrevious);
}

