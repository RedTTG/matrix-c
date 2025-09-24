#ifndef HELPER_H
#define HELPER_H

#include "renderer.h"

void createQuadVertexData(renderer *rnd, float quadWidth, float quadHeight, float *vertices);
bool checkFileExists(std::string path);

#endif //HELPER_H
