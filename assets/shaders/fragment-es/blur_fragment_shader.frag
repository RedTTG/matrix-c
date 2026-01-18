#version 300 es
precision highp float;

uniform sampler2D u_textureC;
in vec2 v_texcoord;
out vec4 fragColor;

uniform float u_blurSize;

void main()
{
    vec2 texOffset = u_blurSize / vec2(textureSize(u_textureC, 0));  // Calculate offset based on texture size
    vec4 color = vec4(0.0);  // Initialize color to black

    // Apply the blur using the neighboring pixels (a simple box blur here)
    color += texture(u_textureC, v_texcoord + texOffset * vec2(-1.0, -1.0));  // Top-left
    color += texture(u_textureC, v_texcoord + texOffset * vec2( 0.0, -1.0));  // Top
    color += texture(u_textureC, v_texcoord + texOffset * vec2( 1.0, -1.0));  // Top-right
    color += texture(u_textureC, v_texcoord + texOffset * vec2(-1.0,  0.0));  // Left
    color += texture(u_textureC, v_texcoord);  // Center (current pixel)
    color += texture(u_textureC, v_texcoord + texOffset * vec2( 1.0,  0.0));  // Right
    color += texture(u_textureC, v_texcoord + texOffset * vec2(-1.0,  1.0));  // Bottom-left
    color += texture(u_textureC, v_texcoord + texOffset * vec2( 0.0,  1.0));  // Bottom
    color += texture(u_textureC, v_texcoord + texOffset * vec2( 1.0,  1.0));  // Bottom-right

    fragColor = color / 9.0;  // Average the colors
}

