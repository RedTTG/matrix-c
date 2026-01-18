# Matrix Rain - OpenGL Renderer

A high-performance Matrix digital rain effect renderer with multiple platform support and advanced post-processing effects.

## Features

- **Matrix Digital Rain Effect**: Authentic falling characters with color variations
- **Post-Processing Effects**: 
  - Ghosting (motion blur trail effect)
  - Gaussian blur
  - Customizable opacity and blur size
- **Multiple Rendering Modes**:
  - Windowed mode (GLFW)
  - Wallpaper mode (Linux X11)
  - Live wallpaper (Android)
- **Cross-Platform Support**:
  - Linux (X11)
  - Windows/macOS (GLFW)
  - Android (OpenGL ES 3.0)

## Platform Support

### Linux (X11)
Full support with wallpaper mode for desktop environments.

**Build requirements:**
- CMake 3.22+
- GCC/Clang with C++20
- Boost (chrono component)
- GLFW 3.3+
- OpenGL 3.3+
- X11 development libraries

**Building:**
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

**Running:**
```bash
# Window mode
./matrix

# Wallpaper mode (renders behind windows)
./matrix -w

# Custom size
./matrix --width 1920 --height 1080
```

### Android
Live wallpaper support for Android 7.0+ devices.

**See [android/README.md](android/README.md) for detailed build instructions.**

**Quick start:**
```bash
cd android
./gradlew assembleDebug
```

### Windows/macOS
Windowed mode using GLFW.

## Command Line Options

```
-h, --help          Show help message
-w, --wallpaper     Run in wallpaper mode (Linux X11 only)
-m, --minimized     Run in minimized mode
--width WIDTH       Set window width
--height HEIGHT     Set window height
--app APP           Set app to run (default: matrix)
--image PATH        Set wallpaper background image
```

## Architecture

The renderer uses a framebuffer-based pipeline:

```
Input (Events) → Renderer Core → Framebuffers → Post-Processing → Display
                      ↓
                 App Logic
                 (Matrix Rain)
```

All platforms share the same:
- Rendering pipeline
- Post-processing shaders
- Matrix rain logic
- Font rendering

Platform-specific code is isolated to:
- Window/context creation
- Event handling
- Buffer swapping

## Post-Processing

### Ghosting Effect
Creates motion blur trails by blending current and previous frames:
- Configurable opacity (0.0 - 1.0)
- Optional blur for smoother trails
- GPU-accelerated using FBOs

### Blur Effect
Gaussian blur for softer appearance:
- Adjustable blur size
- Can be combined with ghosting
- Optimized for real-time rendering

## Technical Details

- **Graphics API**: OpenGL 3.3 / OpenGL ES 3.0
- **Shading Language**: GLSL 3.30 / GLSL ES 3.00
- **Rendering**: Multisampled framebuffers with post-processing
- **Font System**: Custom bitmap font atlas
- **Build System**: CMake with platform detection

## Project Structure

```
matrix-c/
├── src/              # Core C++ implementation
├── include/          # Header files
├── include-linux/    # Linux X11 specific headers
├── include-android/  # Android EGL specific headers
├── android/          # Android project files
├── assets/           # Shaders and fonts
└── CMakeLists.txt    # Unified build system
```

## Contributing

Contributions are welcome! Please ensure:
- Code follows existing style
- Platform-specific code is properly isolated with `#ifdef`
- Changes maintain backwards compatibility
- New features are documented

## License

[License information to be added by repository owner]

## Credits

Created by RedTTG

Inspired by The Matrix (1999)

## Implementation Notes

For detailed information about the Android implementation, see [ANDROID_IMPLEMENTATION.md](ANDROID_IMPLEMENTATION.md).
