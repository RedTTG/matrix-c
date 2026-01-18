# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.kts.

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

# Keep wallpaper service
-keep class com.redttg.matrix.GLWallpaperSurfaceView { *; }

# Standard Android rules
-keepattributes *Annotation*
-keepattributes SourceFile,LineNumberTable
-keep public class * extends java.lang.Exception
