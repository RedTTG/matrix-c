cmake_minimum_required(VERSION 3.10...4.2)
project(matrix)

set(CMAKE_CXX_STANDARD 20)
set(RC_DEPENDS "")

# Detect Android platform
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(ANDROID_BUILD ON)
    message(STATUS "Building for Android")
else()
    set(ANDROID_BUILD OFF)
endif()

include(FetchContent)

# Policy and compatibility settings to silence developer warnings:
# - CMP0167: keep the legacy FindBoost behavior (avoid removal warning)
# - CMP0072: prefer GLVND in FindOpenGL when available
# These are conservative choices to avoid changing build behaviour while removing warnings.
cmake_policy(SET CMP0167 OLD)
cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)

FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	8d1fd52e5ab5590e2c81768ace50c72bae28f2ed #refs/tags/1.0.3
)

FetchContent_MakeAvailable(glm)


file(MAKE_DIRECTORY "generated")

function(embed_resource resource_file_name source_file_name variable_name)
    file(READ ${resource_file_name} hex_content HEX)

    string(REPEAT "[0-9a-f]" 32 column_pattern)
    string(REGEX REPLACE "(${column_pattern})" "\\1\n" content "${hex_content}")

    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " content "${content}")

    string(REGEX REPLACE ", $" "" content "${content}")

    set(array_definition "#pragma once\nstatic constexpr unsigned char ${variable_name}[] =\n{\n${content}\n};")

    set(source "// Auto generated file.\n${array_definition}\n")

    file(WRITE "${source_file_name}" "${source}")

    # Add the resource file as a dependency so CMake rebuilds when it changes
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${resource_file_name})
    list(APPEND RC_DEPENDS ${resource_file_name})
    set(RC_DEPENDS ${RC_DEPENDS} PARENT_SCOPE)
endfunction()

set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_STATIC ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# Skip GLFW and Boost on Android
if(NOT ANDROID_BUILD)
    find_package(Boost REQUIRED COMPONENTS chrono)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED STATIC)
endif()

# Include directories
if(NOT ANDROID_BUILD)
    include_directories(${OPENGL_INCLUDE_DIR})
    include_directories(${GLFW_INCLUDE_DIRS})
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories("include")
include_directories("generated")
include_directories("assets/fonts/include")

# Embed files
embed_resource("assets/help_message.txt" "generated/help_message.h" "helpMessage")
embed_resource("assets/apps_message.txt" "generated/apps_message.h" "appsMessage")

# Embed shaders - use different versions for Android (ES) vs Desktop (core)
if(ANDROID_BUILD)
    # OpenGL ES 3.0 shaders for Android
    embed_resource("assets/shaders/triangle-es.glsl" "generated/triangle_shader.h" "triangleShader")
    embed_resource("assets/shaders/vertex-es/matrix.vert" "generated/matrix_vertex_shader.h" "matrixVertexShader")
    embed_resource("assets/shaders/fragment-es/matrix_rainbow.frag" "generated/matrix_fragment_rainbow_shader.h" "matrixFragRainbowShader")
    embed_resource("assets/shaders/vertex-es/basic_texture_vertex_shader.vert" "generated/basic_texture_vertex_shader.h" "basicTextureVertexShader")
    embed_resource("assets/shaders/fragment-es/basic_texture_fragment_shader.frag" "generated/basic_texture_fragment_shader.h" "basicTextureFragmentShader")
    embed_resource("assets/shaders/fragment-es/ghosting_fragment_shader.frag" "generated/ghosting_fragment_shader.h" "ghostingFragmentShader")
    embed_resource("assets/shaders/fragment-es/blur_fragment_shader.frag" "generated/blur_fragment_shader.h" "blurFragmentShader")
    # Sprite shaders (ES versions for Android)
    embed_resource("assets/shaders/sprite_shader-es.glsl" "generated/sprite_shader.h" "spriteShader")
    embed_resource("assets/shaders/sprite_glow_shader-es.glsl" "generated/sprite_glow_shader.h" "spriteGlowShader")
else()
    # OpenGL 3.3 core shaders for Desktop
    embed_resource("assets/shaders/triangle.glsl" "generated/triangle_shader.h" "triangleShader")
    embed_resource("assets/shaders/vertex/matrix.vert" "generated/matrix_vertex_shader.h" "matrixVertexShader")
    embed_resource("assets/shaders/fragment/matrix_rainbow.frag" "generated/matrix_fragment_rainbow_shader.h" "matrixFragRainbowShader")
    embed_resource("assets/shaders/fragment/matrix_wallpaper.frag" "generated/matrix_fragment_wallpaper_shader.h" "matrixFragWallPaperShader")
    embed_resource("assets/shaders/vertex/cursor_motion.vert" "generated/cursor_motion_vertex_shader.h" "cursorMotionVertexShader")
    embed_resource("assets/shaders/fragment/debug.frag" "generated/debug_fragment_shader.h" "debugFragmentShader")
    embed_resource("assets/shaders/fragment/basic_texture_fragment_shader.frag" "generated/basic_texture_fragment_shader.h" "basicTextureFragmentShader")
    embed_resource("assets/shaders/vertex/basic_texture_vertex_shader.vert" "generated/basic_texture_vertex_shader.h" "basicTextureVertexShader")
    embed_resource("assets/shaders/fragment/ghosting_fragment_shader.frag" "generated/ghosting_fragment_shader.h" "ghostingFragmentShader")
    embed_resource("assets/shaders/fragment/blur_fragment_shader.frag" "generated/blur_fragment_shader.h" "blurFragmentShader")
    # Sprite shaders for Desktop
    embed_resource("assets/shaders/sprite_shader.glsl" "generated/sprite_shader.h" "spriteShader")
    embed_resource("assets/shaders/sprite_glow_shader.glsl" "generated/sprite_glow_shader.h" "spriteGlowShader")
endif()

embed_resource("assets/fonts/matrix_font.raw" "generated/matrix_font.h" "matrixFont")

# Embed texture atlases
embed_resource("assets/sprites/Cat.rgba" "generated/cat_atlas_texture.h" "catAtlasTexture")
embed_resource("assets/sprites/HeadpatCursor.rgba" "generated/headpat_cursor_atlas_texture.h" "headpatCursorAtlasTexture")
embed_resource("assets/sprites/Letters.rgba" "generated/letter_atlas_texture.h" "letterAtlasTexture")
embed_resource("assets/sprites/UI.rgba" "generated/ui_atlas_texture.h" "uiAtlasTexture")

# Define source files
set(MATRIX_SOURCES
        src/shader.cpp
        src/sprite.cpp
        src/options.cpp
        src/renderer.cpp
        src/events.cpp
        src/clock.cpp
        src/helper.cpp
        src/fonts.cpp
        src/gl_errors.cpp
        src/apps/triangle.cpp
        src/apps.cpp
        src/apps/matrix.cpp
        src/apps/matrix_cat/matrix_cat_extension.cpp
        src/apps/matrix_cat/matrix_cat_animation.cpp
        src/apps/debug.cpp
)

# Add platform-specific sources
if(ANDROID_BUILD)
    list(APPEND MATRIX_SOURCES 
        src/android_wallpaper.cpp
        android/app/src/main/cpp/jni_bridge.cpp
    )
    # Build as shared library for Android
    add_library(matrix SHARED ${MATRIX_SOURCES})
else()
    list(APPEND MATRIX_SOURCES 
        src/glad.c
        src/main.cpp
    )
    # Build as executable for other platforms
    add_executable(matrix ${MATRIX_SOURCES})
endif()

# Link libraries
if(ANDROID_BUILD)
    target_link_libraries(
            matrix
            android
            EGL
            GLESv3
            log
            glm::glm
    )
    # Add Android include directory
    target_include_directories(matrix PRIVATE include-android)
else()
    target_link_libraries(
            matrix
            ${OPENGL_LIBRARIES}
            glfw
            ${Boost_LIBRARIES}
            glm::glm
    )
endif()

# Add X11 support for Unix/Linux systems (non-Android)
if (UNIX AND NOT ANDROID_BUILD)
    find_package(X11 REQUIRED)
    include_directories("include-linux")
    include_directories(${X11_INCLUDE_DIR})
    target_sources(matrix PRIVATE src/x11.cpp)
    target_link_libraries(matrix ${X11_LIBRARIES} X11 Xrender Xi)
endif ()
