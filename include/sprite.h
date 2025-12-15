#ifndef SPRITE_H
#define SPRITE_H
#include <cmath>
#include <map>

#include "helper.h"
#include "shader.h"
#include "sprite_shader.h"

class Sprite;

struct AnimationInfo {
    int animationId;
    int frameCount;
    std::vector<UV> uvFrames;
};

struct SpriteState {
    explicit SpriteState(const Sprite &s);
    ~SpriteState() = default;
    int currentAnimation;
    int animationFrame;
    Rect rect;
    float scale;
    float screenScale;
};

struct TextureSize {
    int width;
    int height;
};

class Sprite {
public:
    Sprite(const std::vector<AnimationInfo> &animations, const Rect &screenArea, const Rect &screenSize,
           float screenScale, float scale = 1.0f, int spriteWidth = 32, int spriteHeight = 32);

    ~Sprite() = default;


    static void initSpriteVAO();

    void loadTexture(const unsigned char *source, TextureSize size);

    std::tuple<GLuint, TextureSize> getTexture();

    void reuseTexture(GLuint textureID, TextureSize size);

    void updateAnimation(float deltaTime, bool loop = true);

    void setFPS(int fps);

    void render();

    void setAnimation(int animationId);

    void setColor(const float r, const float g, const float b, const float a = 1.0) {
        color[0] = r;
        color[1] = g;
        color[2] = b;
        color[3] = a;
    }

    void setHue(float hue) {
        if (hue > 1.0f) hue -= 1.0f;

        const float r = std::fabs(hue * 6.0f - 3.0f) - 1.0f;
        const float g = 2.0f - std::fabs(hue * 6.0f - 2.0f);
        const float b = 2.0f - std::fabs(hue * 6.0f - 4.0f);

        setColor(
            clampf(r, 0.0f, 1.0f),
            clampf(g, 0.0f, 1.0f),
            clampf(b, 0.0f, 1.0f)
        );
    }

    // Get sprite dimensions
    float getW() const;

    float getH() const;

    // Set sprite positions
    void setCenterXY(const float x, const float y) {
        setCenterX(x);
        setCenterY(y);
    }

    // States
    void saveState(const std::string &name);
    void loadState(const std::string &name);

    void setCenterX(const float x) { rect.x = x - getW() / 2.0f; }
    void setCenterY(const float y) { rect.y = y - getH() / 2.0f; }

    void setRight(const float x) {
        rect.x = x - getH();
    }
    void setBottom(const float y) {
        rect.y = y - getH();
    }

    void drawSprite(UV uv) const;

    // Public read-only accessors for animation state
    [[nodiscard]] int getAnimationFrame() const { return animationFrame; }
    [[nodiscard]] int getCurrentAnimation() const { return currentAnimation; }
    [[nodiscard]] int getAnimationLooped() const { return animationLooped; }

    // GL data
    ShaderProgram *program = new ShaderProgram();

    // Sprite data
    Rect rect = {0, 0, 32, 32};
    Rect cutoffPercent = {0, 0, 1, 1};
    Rect screenArea;
    Rect screenSize;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float scale;
    float screenScale;

private:
    // GL Data
    GLuint textureID;
    TextureSize textureSize;

    // Animation dictionary
    std::map<int, AnimationInfo> animations;
    static GLuint spriteVAO, spriteVBO;
    int animationFrame = 0;
    float animationTimer = 0;
    float animationSpeed = 1.0 / 60.0; // seconds per frame
    int currentAnimation = 0;
    int animationLooped = 0;

    // State dictionary
    std::map<std::string, SpriteState> savedStates;
};

#endif //SPRITE_H
