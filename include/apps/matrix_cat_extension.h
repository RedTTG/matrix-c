#ifndef MATRIX_CAT_EXTENSION_H
#define MATRIX_CAT_EXTENSION_H
#include <ctime>
#include <queue>
#include <string>
#include <sys/types.h>

#include "../../../assets/sprites/include/catAnimationsInfo.h"
#include "../../../assets/sprites/include/headpatCursorFrameInfo.h"

#include "helper.h"
#include "sprite.h"

#define CAT_SAVE_THRESHOLD 100

class CatData;

enum CatSpriteLayers {
    LAYER_EYES,
    LAYER_FACE,
    LAYER_HAIR,
    LAYER_CLOTHES,
};

struct CatAllAnimation {
    CatAnimation eyesAnimation;
    CatAnimation faceAnimation;
    CatAnimation hairAnimation;
    CatAnimation clothesAnimation;
    int fps = 12;
};

static constexpr CatAllAnimation CAT_ALL_IDLE = {
    CAT_EYES_IDLE,
    CAT_FACE_IDLE,
    CAT_HAIR_IDLE,
    CAT_CLOTHES_IDLE
};

static constexpr CatAllAnimation CAT_ALL_SLEEP_LOOP = {
    CAT_EYES_SLEEP_LOOP,
    CAT_FACE_SLEEP_LOOP,
    CAT_HAIR_SLEEP_LOOP,
    CAT_CLOTHES_SLEEP_LOOP,
    6
};

static constexpr CatAllAnimation CAT_ALL_SLEEP_IN = {
    CAT_EYES_SLEEP_IN,
    CAT_FACE_SLEEP_IN,
    CAT_HAIR_SLEEP_IN,
    CAT_CLOTHES_SLEEP_IN
};

static constexpr CatAllAnimation CAT_ALL_SLEEP_OUT = {
    CAT_EYES_SLEEP_OUT,
    CAT_FACE_SLEEP_OUT,
    CAT_HAIR_SLEEP_OUT,
    CAT_CLOTHES_SLEEP_OUT
};

static constexpr CatAllAnimation CAT_ALL_BLINK = {
    CAT_EYES_BLINK,
    CAT_FACE_BLINK,
    CAT_HAIR_BLINK,
    CAT_CLOTHES_BLINK
};
static constexpr CatAllAnimation CAT_ALL_HEADPAT = {
    CAT_EYES_HEADPAT,
    CAT_FACE_HEADPAT,
    CAT_HAIR_HEADPAT,
    CAT_CLOTHES_HEADPAT
};

struct SingleFireAnimation {
    CatSpriteLayers layer;
    int animationId;
    int nextAnimationId;
};

class CatAnimationManager {
public:
    CatAnimationManager(CatData &catData, float screenScale, Rect screenSize);
    ~CatAnimationManager() = default;

    void render(float deltaTime);
    void logic(float deltaTime, const groupedEvents &events);

private:

    Sprite * getSpriteByLayer(CatSpriteLayers layer) const;
    void initializeSprite(Sprite *&sprite, int animationId);
    void fireSingleAnimation(CatSpriteLayers layer, CatAnimation animationId, CatAnimation nextAnimationId, int fps = 12);
    void fireSingleAnimation(const CatAllAnimation &animation, const CatAllAnimation &nextAnimation);
    void fireLoopAnimation(CatSpriteLayers layer, CatAnimation animationId, int fps = 12);
    void fireLoopAnimation(const CatAllAnimation &animation);
    static bool getShouldLoop(const Sprite *sprite);

    // Sprites
    Sprite *eyesSprite = nullptr;
    Sprite *faceSprite = nullptr;
    Sprite *hairSprite = nullptr;
    Sprite *clothesSprite = nullptr;
    Sprite *headpatCursor = nullptr;

    // Reused info between sprites
    GLuint textureID = 0;
    TextureSize textureSize;
    Rect spriteSize;

    // References
    CatData &catData;
    float screenScale;
    Rect screenSize;
    Rect screenArea;

    // Timers
    float blinkTimer = 0.0f;
    float sleepTimer = 0.0f;

    // Cursor state
    bool headpatCursorActive = false;

    // Track single-fire animations
    std::vector<SingleFireAnimation> singleFireAnimations;
};

class CatData {
public:

    CatData() = default;
    ~CatData() = default;

    void loadFile(const std::string &path);
    void saveFile(const std::string &path);

    void initialize(int width, int height);

    float u_hunger = 80.0f;        // 0..100
    float u_energy = 75.0f;        // 0..100
    float u_cleanliness = 85.0f;   // 0..100
    float u_affection = 45.0f;     // 0..100

    float u_mood = 15.0f;          // -100..100

    float u_playful = 0.55f;       // 0..1
    float u_clingy = 0.45f;        // 0..1
    float u_lazy = 0.40f;          // 0..1

    double u_last_update = now_unix();   // unix time

    [[nodiscard]] bool checkSaveRequired() const;


    // Run-time data
    Rect screenArea {};
    CatAnimationManager *catAnimation = nullptr;
    Sprite *charSprite = nullptr;
    bool mouseLock = false;
    int keysLock = 0;

private:
    std::time_t lastSaveTime = std::time(nullptr);
};

#endif //MATRIX_CAT_EXTENSION_H
