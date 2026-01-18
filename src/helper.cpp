#include "helper.h"
#include <unistd.h>

void createQuadVertexData(renderer *rnd, float quadWidth, float quadHeight, GLfloat *vertices) {
    // Calculate the normalized device coordinates (NDC) for the quad
    float halfScreenWidth = rnd->opts->width / 2.0f;
    float halfScreenHeight = rnd->opts->height / 2.0f;
    float halfQuadWidth = quadWidth / 2.0f;
    float halfQuadHeight = quadHeight / 2.0f;

    float left = -halfQuadWidth / halfScreenWidth;
    float right = halfQuadWidth / halfScreenWidth;
    float top = halfQuadHeight / halfScreenHeight;
    float bottom = -halfQuadHeight / halfScreenHeight;

    // Define the quad vertices in NDC
    GLfloat quadVertices[] = {
        left, top, // Top-left
        right, top, // Top-right
        right, bottom, // Bottom-right
        left, bottom // Bottom-left
    };

    // Copy the vertices to the output array
    for (int i = 0; i < 8; ++i) {
        vertices[i] = quadVertices[i];
    }
}

bool checkFileExists(std::string path) {
    return access(path.c_str(), F_OK) != -1;
}
