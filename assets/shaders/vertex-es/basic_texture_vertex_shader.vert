#version 300 es
precision highp float;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;

out vec2 v_texcoord;

void main() {
    gl_Position = vec4(position.x, position.y, 0.0, 1.0);
    v_texcoord = texcoord;
}

