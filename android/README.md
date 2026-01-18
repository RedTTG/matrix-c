# Building for Android

This document describes how to build the Matrix Rain live wallpaper for Android.

## Prerequisites

- Android Studio (latest version recommended)
- Android NDK r25c or newer
- CMake 3.22.1 or newer
- Gradle 8.2+
- Android SDK API Level 24 (Android 7.0) or higher

## Build Instructions

### Using Android Studio

1. Open Android Studio
2. Select "Open an Existing Project"
3. Navigate to the `android/` directory in the repository
4. Android Studio will automatically sync the Gradle project
5. Build the project using Build → Make Project
6. Install on device: Run → Run 'app' (or use the green play button)

### Using Command Line

```bash
cd android
./gradlew assembleDebug
```

The APK will be generated in `android/app/build/outputs/apk/debug/`

### Installing the Wallpaper

1. After installation, go to device Settings
2. Navigate to Display → Wallpaper (location may vary by device)
3. Select "Matrix Rain" from the live wallpapers list
4. Preview and set as wallpaper

## Architecture Overview

The Android implementation follows the same architecture as the Linux X11 version:

- **Platform Layer**: Uses EGL for OpenGL ES 3.0 context creation
- **Rendering**: Shared C++ renderer with framebuffer post-processing
- **Integration**: JNI bridge connects Kotlin WallpaperService to C++ code

### Key Components

- `android/app/src/main/cpp/jni_bridge.cpp` - JNI layer
- `src/android_wallpaper.cpp` - EGL setup and teardown
- `MatrixWallpaperService.kt` - Android wallpaper service
- `CMakeLists.txt` - Unified build system with Android support

## Debugging

To debug the native code:

1. Open Android Studio
2. Set breakpoints in C++ files
3. Use Run → Debug 'app' (or the debug icon)
4. Android Studio will attach the debugger to both Java and native code

View logs with:
```bash
adb logcat -s MatrixWallpaper MatrixJNI
```

## Known Limitations

- Minimum API level 24 (Android 7.0) required for OpenGL ES 3.0
- Wallpaper settings UI not yet implemented
- Performance may vary on older devices

## Troubleshooting

### Build fails with CMake errors
- Ensure NDK is properly installed in Android Studio
- Check that CMakeLists.txt path is correct in app/build.gradle.kts

### App installs but wallpaper doesn't appear
- Check logcat for native crashes
- Verify OpenGL ES 3.0 support on the device

### Black screen or no rendering
- Check shader compilation logs in logcat
- Verify EGL context creation succeeded
