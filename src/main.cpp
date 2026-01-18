#include <chrono>

#include "shader.h"
#include "renderer.h"
#include <iostream>
#include <options.h>
#include <thread>

#if defined(__linux__) && !defined(__ANDROID__)
#include "x11.h"
#endif

int main(const int argc, char *argv[]) {
    options *opts = parseOptions(argc, argv);
    auto *rnd = new renderer(opts);

    rnd->initialize();

    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (true) {
        rnd->getEvents();
        if (rnd->events->quit) {
            break;
        }

        if (!opts->loopWithSwap || rnd->clock->frameSwapDeltaTime >= opts->swapTime) {
            rnd->frameBegin();
            rnd->loopApp();
            rnd->frameEnd();

            rnd->swapBuffers();
        }
    }

    rnd->destroy();
    return 0;
}
