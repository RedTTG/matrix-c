#ifndef ANDROID_WALLPAPER_H
#define ANDROID_WALLPAPER_H

#ifdef __ANDROID__

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>

struct renderer;

#define ANDROID_APPNAME "MatrixAndroid"

void setupEGLForWallpaper(renderer *rnd, ANativeWindow* window);
void android_SwapBuffers(renderer *rnd);
void destroyEGL(renderer *rnd);
void initializeGladES();

#endif // __ANDROID__

#endif // ANDROID_WALLPAPER_H
