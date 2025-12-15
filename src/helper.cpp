#include "helper.h"

void createQuadVertexData(renderer *rnd, float quadWidth, float quadHeight, GLfloat *vertices) {
    // Calculate the normalized device coordinates (NDC) for the quad
    float halfScreenWidth = rnd->opts->width / 2.0f;
    float halfScreenHeight = rnd->opts->height / 2.0f;
    float halfQuadWidth = quadWidth / 2.0f;
    float halfQuadHeight = quadHeight / 2.0f;

    float left = -halfQuadWidth / halfScreenWidth;
    float right = halfQuadWidth / halfScreenWidth;
    float top = halfQuadHeight / halfScreenHeight;
    float bottom = -halfQuadHeight / halfScreenHeight;

    // Define the quad vertices in NDC
    GLfloat quadVertices[] = {
        left, top, // Top-left
        right, top, // Top-right
        right, bottom, // Bottom-right
        left, bottom // Bottom-left
    };

    // Copy the vertices to the output array
    for (int i = 0; i < 8; ++i) {
        vertices[i] = quadVertices[i];
    }
}

bool checkFileExists(std::string path) {
    return access(path.c_str(), F_OK) != -1;
}

File::File() = default;

File::~File() {
    close();
}

bool File::open(const std::string& path, const Mode mode) const {
    close();

    std::ios::openmode om = std::ios::binary;
    if (mode == Mode::Read) {
        om |= std::ios::in;
        fs_.open(path, om);
    } else if (mode == Mode::Write) {
        // Try to open for read/write; if it doesn't exist create it then reopen.
        om |= std::ios::in | std::ios::out;
        fs_.open(path, om);
        if (!fs_.is_open()) {
            // create/truncate and then reopen for in/out
            std::ofstream create(path, std::ios::binary | std::ios::trunc);
            create.close();
            fs_.open(path, om);
        }
    } else { // ReadWrite
        om |= std::ios::in | std::ios::out;
        fs_.open(path, om);
    }

    if (!fs_.is_open()) {
        return false;
    }

    // Ensure both get/put pointers start at 0
    fs_.seekg(0, std::ios::beg);
    fs_.seekp(0, std::ios::beg);
    return true;
}

void File::close() const {
    if (fs_.is_open()) fs_.close();
}

bool File::readBytes(void* buffer, const std::size_t count) const {
    if (!fs_.is_open() || !buffer) return false;
    if (count == 0) return true;

    std::vector<char> tmp;
    tmp.resize(count);

    fs_.clear(); // clear EOF/fail before reading
    fs_.read(tmp.data(), static_cast<std::streamsize>(count));
    std::streamsize got = fs_.gcount();

    if (static_cast<std::size_t>(got) != count) {
        // do not modify caller's buffer on partial read
        return false;
    }

    // copy to caller's buffer
    std::memcpy(buffer, tmp.data(), count);
    return true;
}

std::size_t File::writeBytes(const void* buffer, const std::size_t count) const {
    if (!fs_.is_open() || !buffer || count == 0) return 0;
    fs_.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(count));
    if (!fs_) return 0;
    // write advances put pointer by count; assume all written when stream OK
    return count;
}

bool File::seek(const std::size_t pos) const {
    if (!fs_.is_open()) return false;
    fs_.clear(); // clear eof/fail before seeking
    fs_.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
    fs_.seekp(static_cast<std::streamoff>(pos), std::ios::beg);
    return static_cast<bool>(fs_);
}

std::size_t File::tell() const {
    if (!fs_.is_open()) return static_cast<std::size_t>(-1);
    std::streampos p = fs_.tellg();
    if (p == static_cast<std::streampos>(-1)) p = fs_.tellp();
    if (p == static_cast<std::streampos>(-1)) return static_cast<std::size_t>(-1);
    return p;
}

std::size_t File::size() const {
    if (!fs_.is_open()) return 0;
    // save current pos
    std::streampos cur = fs_.tellg();
    if (cur == static_cast<std::streampos>(-1)) cur = fs_.tellp();

    fs_.clear(); // clear EOF/fail before seeking
    fs_.seekg(0, std::ios::end);
    const std::streampos end = fs_.tellg();
    const std::size_t sz = end == static_cast<std::streampos>(-1) ? 0 : static_cast<std::size_t>(end);

    // restore previous pos for both get/put
    if (cur != static_cast<std::streampos>(-1)) {
        fs_.seekg(cur, std::ios::beg);
        fs_.seekp(cur, std::ios::beg);
    } else {
        fs_.seekg(0, std::ios::beg);
        fs_.seekp(0, std::ios::beg);
    }
    return sz;
}

bool File::isOpen() const {
    return fs_.is_open();
}