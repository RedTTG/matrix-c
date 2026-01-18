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

**Debug Build:**
```bash
cd android
./gradlew assembleDebug
```

The APK will be generated in `android/app/build/outputs/apk/debug/`

**Release Build:**
```bash
cd android
./gradlew assembleRelease
```

The APK will be generated in `android/app/build/outputs/apk/release/`

### Installing the Wallpaper

1. After installation, go to device Settings
2. Navigate to Display → Wallpaper (location may vary by device)
3. Select "Matrix Rain" from the live wallpapers list
4. Preview and set as wallpaper

## Building for Production

### Creating a Keystore

Before releasing your app, you need to sign it with a release keystore:

1. **Generate a keystore** (only needed once):
```bash
keytool -genkey -v -keystore matrix-release-key.jks -keyalg RSA -keysize 2048 -validity 10000 -alias matrix-key
```

2. Follow the prompts to set:
   - Keystore password
   - Key password
   - Your name and organization details

3. **Store the keystore securely** - Keep `matrix-release-key.jks` in a safe location (NOT in the repository)

### Configuring Signing

Create `android/keystore.properties` (add to .gitignore):

```properties
storeFile=/path/to/matrix-release-key.jks
storePassword=your_keystore_password
keyAlias=matrix-key
keyPassword=your_key_password
```

Update `android/app/build.gradle.kts` to use the keystore:

```kotlin
// Add at the top of the file
val keystorePropertiesFile = rootProject.file("keystore.properties")
val keystoreProperties = Properties()
if (keystorePropertiesFile.exists()) {
    keystoreProperties.load(FileInputStream(keystorePropertiesFile))
}

android {
    // ... existing config ...
    
    signingConfigs {
        create("release") {
            storeFile = keystoreProperties["storeFile"]?.let { file(it) }
            storePassword = keystoreProperties["storePassword"] as String?
            keyAlias = keystoreProperties["keyAlias"] as String?
            keyPassword = keystoreProperties["keyPassword"] as String?
        }
    }
    
    buildTypes {
        release {
            signingConfig = signingConfigs.getByName("release")
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
}
```

### Building Signed APK

```bash
cd android
./gradlew assembleRelease
```

The signed APK will be in `android/app/build/outputs/apk/release/app-release.apk`

### Building Android App Bundle (AAB) for Play Store

Google Play requires AAB format for new apps:

```bash
cd android
./gradlew bundleRelease
```

The AAB will be in `android/app/build/outputs/bundle/release/app-release.aab`

### Publishing to Google Play Store

1. **Create a Google Play Developer account** ($25 one-time fee)
   - Visit https://play.google.com/console

2. **Create a new application**
   - Click "Create app"
   - Fill in app details (name, language, type, category)

3. **Complete the store listing**
   - App icon (512x512 PNG)
   - Feature graphic (1024x500 PNG)
   - Screenshots (at least 2, recommended 4-8)
   - Short description (80 characters max)
   - Full description (4000 characters max)

4. **Set up content rating**
   - Complete the questionnaire
   - Get rating certificate

5. **Set pricing and distribution**
   - Free or paid
   - Select countries
   - Accept content guidelines

6. **Upload the AAB**
   - Go to "Production" → "Create new release"
   - Upload `app-release.aab`
   - Add release notes
   - Review and rollout

7. **App review process**
   - Google will review your app (usually 1-7 days)
   - Fix any issues flagged by the review team
   - Once approved, app goes live

### ProGuard Configuration

For release builds, create `android/app/proguard-rules.pro`:

```proguard
# Keep native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep JNI methods in MatrixWallpaperService
-keep class com.redttg.matrix.MatrixWallpaperService {
    native <methods>;
}

# Keep OpenGL classes
-keep class javax.microedition.khronos.** { *; }
-keep class android.opengl.** { *; }
```

### Version Management

Update version in `android/app/build.gradle.kts` before each release:

```kotlin
defaultConfig {
    versionCode = 2  // Increment for each release
    versionName = "1.1"  // User-facing version
}
```

**Important notes:**
- `versionCode` must be incremented for every update
- `versionName` is what users see (can be any string)
- Keep your keystore and passwords secure
- Back up your keystore - losing it means you can't update your app

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
