#include "apps.h"

#include "clock.h"
#include <cstring>
#include <iostream>
#include <thread>
#include <apps/debug.h>
#include <apps/triangle.h>
#include <apps/matrix.h>
#include "apps_message.h"

App::App(renderer *rnd) {
    this->rnd = rnd;
}

App *initializeApp(renderer *rnd, const char *name) {
    if (strcmp(name, "triangle") == 0) {
        const auto app = new TriangleApp(rnd);
        app->setup();
        return app;
    } else if (strcmp(name, "matrix") == 0) {
        const auto app = new MatrixApp(rnd);
        app->setup();
        return app;
    } else if (strcmp(name, "debug") == 0) {
        const auto app = new DebugApp(rnd);
        app->setup();
        return app;
    } else {
        std::cerr << "Unknown app: " << name << std::endl;

        std::this_thread::sleep_for(chrono_impl::milliseconds(10));

        std::string appsText(reinterpret_cast<const char*>(appsMessage), sizeof(appsMessage));
        std::cout << appsText << std::endl;
        std::cout.flush();
        exit(1);
    }
}