#shader vertex
#version 300 es
precision highp float;

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 a_Color;

out vec3 v_Color;

uniform float u_Time;
void main()
{
    float angle = radians(u_Time * 360.0 / 5.0);
    mat4 rotation = mat4(
        cos(angle), -sin(angle), 0.0, 0.0,
        sin(angle), cos(angle), 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    gl_Position = rotation * position;
    v_Color = a_Color;
}

#shader fragment
#version 300 es
precision highp float;

in vec3 v_Color;

out vec4 fragColor;

uniform float u_Time;
const float quantizationFactor = 4.0;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
    vec3 shiftedColor = vec3(
    0.5 * sin(u_Time + v_Color.r * 5.0) + 0.5,
    0.5 * cos(u_Time + v_Color.g * 5.0) + 0.5,
    0.5 * sin(u_Time + v_Color.b * 5.0 + 1.57) + 0.5
    );

    // Add dithering
    vec2 st = gl_FragCoord.xy / vec2(800.0, 600.0); // Adjust based on your resolution
    vec3 dither = vec3(random(st), random(st + 1.0), random(st + 2.0)) / quantizationFactor;
    shiftedColor = floor((shiftedColor + dither) * quantizationFactor) / quantizationFactor;

    fragColor = vec4(shiftedColor, 1.0);
}

