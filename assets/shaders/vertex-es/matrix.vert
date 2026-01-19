#version 300 es
precision highp float;
precision highp int;

struct CharacterInfo {
    uint xOffset;
    uint yOffset;
    uint width;
    uint height;
};

layout(location = 0) in vec2 position;      // Per-instance position
layout(location = 1) in float colorOffset;  // Per-instance color offset
layout(location = 2) in int spark;          // Per-instance spark
layout(location = 3) in vec2 quadVertex;    // Per-vertex quad position (0-1 range)

layout(std140) uniform u_AtlasBuffer {
    CharacterInfo characterInfoList[64];
};

uniform mat4 u_Projection;
uniform vec2 u_ViewportSize;
uniform vec2 u_AtlasTextureSize;
uniform int u_MaxCharacters;
uniform float u_CharacterScaling;
uniform float u_Time;
uniform int u_Rotation;

out float v_ColorOffset;
flat out int v_Spark;
out vec2 v_TexCoord;
out vec2 v_ScreenCoord;

int generateRandomIndex(int instanceID, int maxIndex) {
    float timeValue = u_Time * 100.0;
    float floatInstanceID = float(instanceID);
    float floatMaxIndex = float(maxIndex);
    return abs(int(mod(floor(floatInstanceID + timeValue), floatMaxIndex)));
}

void main()
{
    // Get a random index for the character data
    int randomIndex = generateRandomIndex(gl_InstanceID+1, u_MaxCharacters);

    // Fetch the character data from the texture buffer using the random index
    CharacterInfo characterInfo = characterInfoList[randomIndex];

    float glyphXOffset = float(characterInfo.xOffset);
    float glyphYOffset = float(characterInfo.yOffset);
    float glyphWidth = float(characterInfo.width);
    float glyphHeight = float(characterInfo.height);

    float angle = radians(float(u_Rotation));
    mat2 rotationMatrix = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

    // Use quadVertex attribute to create the quad (0-1 range scaled to glyph size)
    vec2 vertexPosition = quadVertex * vec2(glyphWidth, glyphHeight);

    // Add the vertex position in screen space
    vec2 screenPosition = position + (vertexPosition * u_CharacterScaling);
    v_ScreenCoord = vec2(1.0) - vec2(1.0 - (screenPosition.x / u_ViewportSize.x), screenPosition.y / u_ViewportSize.y);
    vec2 atlasPosition = vec2(glyphXOffset, glyphYOffset) + vertexPosition;

    // Calculate the center of the character
    vec2 center = position + vec2(glyphWidth, glyphHeight) * 0.5 * u_CharacterScaling;

    // Translate to the center, apply rotation, and translate back
    screenPosition = rotationMatrix * (screenPosition - center) + center;

    // Apply the projection matrix to get the position in clip space
    gl_Position = u_Projection * vec4(screenPosition, 0.0, 1.0);

    v_ColorOffset = colorOffset;
    v_Spark = spark;
    v_TexCoord = vec2(atlasPosition.x, u_AtlasTextureSize.y - atlasPosition.y) / u_AtlasTextureSize;
}

