#include "sprite.h"

#include <ranges>

#include "gl_errors.h"
#include "helper.h"
#include "stb_image.h"

GLuint Sprite::spriteVAO = 0;
GLuint Sprite::spriteVBO = 0;

SpriteState::SpriteState(const Sprite &s)
    : currentAnimation(s.getCurrentAnimation()),
      animationFrame(s.getAnimationFrame()),
      rect(s.rect),
      scale(s.scale),
      screenScale(s.screenScale) {
}

Sprite::Sprite(const std::vector<AnimationInfo> &animations, const Rect &screenArea, const Rect &screenSize,
               const float screenScale, const float scale, const int spriteWidth, const int spriteHeight)
    : screenArea(screenArea),
      screenSize(screenSize),
      scale(scale),
      screenScale(screenScale),
      textureID(0),
      textureSize{0, 0} {
    // Expand animations into map
    for (const auto &anim: animations) {
        this->animations[anim.animationId] = anim;
    }

    rect.w = static_cast<float>(spriteWidth);
    rect.h = static_cast<float>(spriteHeight);

    // Load shader program
    program->loadShader(spriteShader, sizeof(spriteShader));
    program->linkProgram();

    // Initialize VAO and load texture
    initSpriteVAO();
}

void Sprite::initSpriteVAO() {
    if (spriteVAO != 0 && spriteVBO != 0) {
        return; // Already initialized
    }
    constexpr float quad[] = {
        // pos    // uv
        0.f, 0.f, 0.f, 0.f,
        1.f, 0.f, 1.f, 0.f,
        1.f, 1.f, 1.f, 1.f,

        0.f, 0.f, 0.f, 0.f,
        1.f, 1.f, 1.f, 1.f,
        0.f, 1.f, 0.f, 1.f
    };

    glGenVertexArrays(1, &spriteVAO);
    glGenBuffers(1, &spriteVBO);

    glBindVertexArray(spriteVAO);

    glBindBuffer(GL_ARRAY_BUFFER, spriteVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // aPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), static_cast<void *>(nullptr));

    glEnableVertexAttribArray(1); // aUV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Sprite::loadTexture(const unsigned char *source, const TextureSize size) {
    textureSize = size;
    GL_CHECK(glGenTextures(1, &textureID));
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureID));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, source));

    // Nearest filtering
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    // Prevent edge sampling artifacts
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // Check if the sprite dimensions match the texture size
    int calculatedWidth = 0;
    int calculatedHeight = 0;

    for (const auto &anim: animations | std::views::values) {
        for (const auto &[u0, v0, u1, v1]: anim.uvFrames) {
            const float frameWidthV =  (u1 - u0) * textureSize.width;
            const float frameHeightV = (v1 - v0) * textureSize.height;

            const int frameWidth = static_cast<int>(frameWidthV);
            const int frameHeight = static_cast<int>(frameHeightV);

            // std::cout << "Animation " << anim.animationId << " frame UVs: "
            //           << u0 << ", " << v0 << ", "
            //           << u1 << ", " << v1 << " => "
            //           << frameWidthV << "x" << frameHeightV << std::endl;

            if (frameWidth > calculatedWidth) {
                calculatedWidth = frameWidth;
            }
            if (frameHeight > calculatedHeight) {
                calculatedHeight = frameHeight;
            }
        }
    }

    if (rect.w != calculatedWidth || rect.h != calculatedHeight) {
        // std::cerr << "Warning: Sprite dimensions (" << rect.w << "x" << rect.h
        //           << ") do not match calculated frame size (" << calculatedWidth << "x" << calculatedHeight << ")."
        //           << " Using provided dimensions." << std::endl;
        rect.w = static_cast<float>(calculatedWidth);
        rect.h = static_cast<float>(calculatedHeight);
    }
}

std::tuple<GLuint, TextureSize> Sprite::getTexture() {
    return {textureID, textureSize};
}

void Sprite::reuseTexture(const GLuint textureID, const TextureSize size) {
    this->textureID = textureID;
    textureSize = size;
}

void Sprite::updateAnimation(const float deltaTime, const bool loop) {
    animationTimer += deltaTime;
    if (animationTimer >= animationSpeed) {
        animationTimer = 0;
        animationFrame++;
        if (animationFrame >= animations[currentAnimation].frameCount) {
            if (loop) {
                animationFrame = 0;
                animationLooped++;
            } else {
                animationFrame = animations[currentAnimation].frameCount - 1;
            }
        }
    }
}

void Sprite::setFPS(const int fps) {
    animationSpeed = 1.0f / static_cast<float>(fps);
}

void Sprite::render() {
    AnimationInfo anim = animations[currentAnimation];
    UV uv = anim.uvFrames[animationFrame];
    drawSprite(uv);
}

void Sprite::setAnimation(const int animationId) {
    currentAnimation = animationId;
    animationFrame = 0;
    animationTimer = 0;
    animationLooped = 0;
}

float Sprite::getW() const {
    return rect.w * scale * screenScale;
}
float Sprite::getH() const {
    return rect.h * scale * screenScale;
}

void Sprite::saveState(const std::string &name) {
    savedStates.insert_or_assign(name, SpriteState(*this));
}

void Sprite::loadState(const std::string &name) {
    auto it = savedStates.find(name);
    if (it == savedStates.end()) {
        throw std::runtime_error(std::string("Sprite::loadState: saved state not found: ") + name);
    }
    const SpriteState &st = it->second;
    currentAnimation = st.currentAnimation;
    animationFrame = st.animationFrame;
    animationTimer = 0.0f;
    rect = st.rect;
    scale = st.scale;
    screenScale = st.screenScale;
}

void Sprite::drawSprite(const UV uv) const {
    // show uvs
    // std::cout << "Drawing sprite with UVs: "
    //           << uv.u0 << ", " << uv.v0 << ", "
    //           << uv.u1 << ", " << uv.v1 << std::endl;
    program->useProgram();

    GL_CHECK(glUniform2f(program->getUniformLocation("uPos"), rect.x, rect.y));
    GL_CHECK(glUniform2f(program->getUniformLocation("uSize"), rect.w, rect.h));
    GL_CHECK(glUniform4f(program->getUniformLocation("uCutoff"), cutoffPercent.x, cutoffPercent.y, cutoffPercent.w, cutoffPercent.h));
    GL_CHECK(glUniform4f(program->getUniformLocation("uUV"), uv.u0, uv.v0, uv.u1, uv.v1));
    GL_CHECK(glUniform2f(program->getUniformLocation("uScreen"), screenSize.w, screenSize.h));
    GL_CHECK(glUniform2f(program->getUniformLocation("uOffset"), screenArea.x, screenArea.y));
    GL_CHECK(glUniform2f(program->getUniformLocation("uTexSize"), textureSize.width, textureSize.height));
    GL_CHECK(glUniform1f(program->getUniformLocation("uScale"), scale * screenScale));

    GL_CHECK(glUniform4f(program->getUniformLocation("uColor"),color[0], color[1], color[2], color[3]));

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureID));
    GL_CHECK(glUniform1i(program->getUniformLocation("uTexture"), 0));

    GL_CHECK(glBindVertexArray(spriteVAO));
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));
}
