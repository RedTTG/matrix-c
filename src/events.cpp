#include "events.h"
#include <unordered_set>

#if defined(__linux__) && !defined(__ANDROID__)
void handleMousePress(groupedEvents *events, const int number, bool pressed) {
    switch (number) {
        case 1:
            events->mouseLeft = pressed;
            break;
        case 2:
            events->mouseMiddle = pressed;
            break;
        case 3:
            events->mouseRight = pressed;
            break;
        default:
            break;
    }
}

void handleX11Events(const renderer *rnd) {
    XEvent event;

    // Get the mouse position
    int x, y;
    Window root, child;
    int rootX, rootY;
    unsigned int mask;
    XQueryPointer(rnd->display, rnd->window, &root, &child, &rootX, &rootY, &x, &y, &mask);
    rnd->events->mouseX = x;
    rnd->events->mouseY = y;

    while (XPending(rnd->display) > 0) {
        XNextEvent(rnd->display, &event);
        if (rnd->x11MouseEvents && event.xcookie.type == GenericEvent && event.xcookie.extension == rnd->
            xinputOptCode) {
            XGetEventData(rnd->display, &event.xcookie);
            if (event.xcookie.evtype == XI_RawMotion) {
                rnd->events->lastMouseMotion = rnd->clock->now();
            } else if (event.xcookie.evtype == XI_RawKeyPress) {
                rnd->events->keysPressed++;
                // std::cout << "Key pressed: " << static_cast<XIRawEvent *>(event.xcookie.data)->detail << std::endl;
            } else if (event.xcookie.evtype == XI_RawKeyRelease) {
                rnd->events->keysPressed--;
                if (rnd->events->keysPressed < 0)
                    rnd->events->keysPressed = 0;
                // std::cout << "Key released: " << static_cast<XIRawEvent *>(event.xcookie.data)->detail << std::endl;
            } else if (event.xcookie.evtype == XI_RawButtonPress) {
                handleMousePress(rnd->events, static_cast<XIRawEvent *>(event.xcookie.data)->detail, true);
            } else if (event.xcookie.evtype == XI_RawButtonRelease) {
                handleMousePress(rnd->events, static_cast<XIRawEvent *>(event.xcookie.data)->detail, false);
            }
            XFreeEventData(rnd->display, &event.xcookie);
        } else if (!rnd->x11MouseEvents) {
            if (event.type == KeyPress) {
                rnd->events->keysPressed++;
                // std::cout << "X11 Key pressed: " << event.xkey.keycode << std::endl;
            } else if (event.type == KeyRelease) {
                rnd->events->keysPressed--;
                if (rnd->events->keysPressed < 0)
                    rnd->events->keysPressed = 0;
                // std::cout << "X11 Key released: " << event.xkey.keycode << std::endl;
            } else if (event.type == ButtonPress) {
                handleMousePress(rnd->events, event.xbutton.button, true);
            } else if (event.type == ButtonRelease) {
                handleMousePress(rnd->events, event.xbutton.button, false);
            } else if (event.type == MotionNotify) {
                rnd->events->lastMouseMotion = rnd->clock->now();
            }
        }
        if (event.type == DestroyNotify) {
            rnd->events->quit = true;
        }
    }
}
#endif

#ifdef __ANDROID__
// Note: handleAndroidEvents is available for future use but not currently called.
// Android touch events are handled directly via JNI callbacks in jni_bridge.cpp
// which update the events structure synchronously. This function remains available
// for potential AInputEvent processing in the future.
void handleAndroidEvents(const renderer *rnd, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        
        rnd->events->mouseX = static_cast<long>(x);
        rnd->events->mouseY = static_cast<long>(y);
        
        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_MOVE) {
            rnd->events->mouseLeft = true;
            rnd->events->lastMouseMotion = rnd->clock->now();
        } else if (action == AMOTION_EVENT_ACTION_UP) {
            rnd->events->mouseLeft = false;
        }
    }
}
#endif

void handleGLFWEvents(const renderer *rnd) {
    static std::unordered_set<int> pressedKeys;
    glfwPollEvents();
    if (glfwWindowShouldClose(rnd->glfwWindow)) {
        rnd->events->quit = true;
    }

    if (glfwGetMouseButton(rnd->glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        rnd->events->mouseLeft = true;
    else
        rnd->events->mouseLeft = false;

    if (glfwGetMouseButton(rnd->glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
        rnd->events->mouseMiddle = true;
    else
        rnd->events->mouseMiddle = false;
    if (glfwGetMouseButton(rnd->glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        rnd->events->mouseRight = true;
    else
        rnd->events->mouseRight = false;

    double x, y;
    glfwGetCursorPos(rnd->glfwWindow, &x, &y);
    rnd->events->mouseX = static_cast<long>(x);
    rnd->events->mouseY = static_cast<long>(y);

    pressedKeys.clear();
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        if (glfwGetKey(rnd->glfwWindow, key) == GLFW_PRESS) {
            pressedKeys.insert(key);
        }
    }
    rnd->events->keysPressed = pressedKeys.size();
}