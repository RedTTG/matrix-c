#ifndef HELPER_H
#define HELPER_H

#include "renderer.h"

#include <string>
#include <fstream>
#include <cstddef>
#include <cstdint>

void createQuadVertexData(renderer *rnd, float quadWidth, float quadHeight, float *vertices);
bool checkFileExists(std::string path);

static float clampf(const float v, const float lo, const float hi) {
    return std::max(lo, std::min(v, hi));
}

inline double now_unix() {
    using namespace std::chrono;
    return duration_cast<duration<double>>(
        system_clock::now().time_since_epoch()
    ).count();
}


class File {
public:
    enum class Mode { Read, Write, ReadWrite };

    File();
    ~File();

    // Open file in the requested mode. On success, stream positions are set to 0.
    bool open(const std::string& path, Mode mode) const;

    // Close the file if open.
    void close() const;

    // Read up to count bytes into buffer. Returns number of bytes actually read.
    bool readBytes(void* buffer, std::size_t count) const;

    // Write count bytes from buffer. Returns number of bytes actually written (0 on error).
    std::size_t writeBytes(const void* buffer, std::size_t count) const;

    // Absolute seek (both get and put pointers). Returns true on success.
    bool seek(std::size_t pos) const;

    // Return current position (prefer get pointer, fallback to put). Returns static_cast<std::size_t>(-1) on error.
    std::size_t tell() const;
    std::size_t size() const;

    bool isOpen() const;

private:
    mutable std::fstream fs_;
};

struct Rect {
    float x;
    float y;
    float w;
    float h;
};

struct UV {
    float u0;
    float v0;
    float u1;
    float v1;
};


#endif //HELPER_H
