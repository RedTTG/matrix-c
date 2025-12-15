#include "apps/matrix_cat_extension.h"
#include "cat_atlas_texture.h"

CatAnimationManager::CatAnimationManager(CatData &catData, float screenScale, Rect screenSize) : catData(catData),
    screenArea(catData.screenArea),
    screenScale(screenScale),
    screenSize(screenSize) {
    initializeSprite(eyesSprite, CAT_EYES_OPEN);
    initializeSprite(faceSprite, CAT_FACE_SMILE);
    initializeSprite(hairSprite, CAT_HAIR_IDLE);
    initializeSprite(clothesSprite, CAT_CLOTHES);
}

void CatAnimationManager::render(const float deltaTime) {
    // Update animation
    eyesSprite->updateAnimation(deltaTime, getShouldLoop(eyesSprite));
    faceSprite->updateAnimation(deltaTime, getShouldLoop(faceSprite));
    hairSprite->updateAnimation(deltaTime, getShouldLoop(hairSprite));
    clothesSprite->updateAnimation(deltaTime, getShouldLoop(clothesSprite));

    int i = 0;
    while (i < singleFireAnimations.size()) {
        const auto &singleAnim = singleFireAnimations[i];
        const auto [layer, animationId, nextAnimationId] = singleAnim;
        if (Sprite *sprite = getSpriteByLayer(layer); sprite->getAnimationLooped() > 0) {
            sprite->setAnimation(nextAnimationId);
            singleFireAnimations.erase(singleFireAnimations.begin() + i);
        } else {
            ++i;
        }
    }

    // Render sprites IN ORDER
    clothesSprite->render();
    faceSprite->render();
    hairSprite->render();
    eyesSprite->render();

    // Increment timers
    blinkTimer += deltaTime;

    // Check for random events
    if (const auto anim = eyesSprite->getCurrentAnimation();
        (anim == CAT_EYES_STAY_OPEN || anim == CAT_EYES_OPEN) && blinkTimer > 3) {
        fireSingleAnimation(LAYER_EYES, CAT_EYES_BLINK, CAT_EYES_STAY_OPEN);
        if (rand() % 100 < 10) {
            blinkTimer -= 1.0f; // shorter time to next blink
        } else {
            blinkTimer = 0.0f; // reset timer
        }
    }
    if (const auto anim = hairSprite->getCurrentAnimation(); rand() % 1000 < 5 && anim == CAT_HAIR_IDLE) {
        int random = rand() % 2;
        switch (random) {
            case 0:
                fireSingleAnimation(LAYER_HAIR, CAT_HAIR_TWITCH_EAR, CAT_HAIR_IDLE);
                break;
            case 1:
                fireSingleAnimation(LAYER_HAIR, CAT_HAIR_WAG, CAT_HAIR_IDLE);
                break;
        }
    }
    if (const auto anim = faceSprite->getCurrentAnimation(); rand() % 1000 < 5 && anim == CAT_FACE_SMILE) {
        fireSingleAnimation(LAYER_FACE, CAT_FACE_BLEW, CAT_FACE_SMILE);
    }
}

Sprite *CatAnimationManager::getSpriteByLayer(const CatSpriteLayers layer) const {
    switch (layer) {
        case LAYER_EYES:
            return eyesSprite;
        case LAYER_FACE:
            return faceSprite;
        case LAYER_HAIR:
            return hairSprite;
        case LAYER_CLOTHES:
            return clothesSprite;
        default:
            return nullptr;
    }
}

void CatAnimationManager::initializeSprite(Sprite *&sprite, const int animationId) {
    sprite = new Sprite(
        catAnimations,
        screenArea,
        screenSize,
        screenScale,
        2.5f
    );

    // Set the animation and FPS
    sprite->setAnimation(animationId);
    sprite->setFPS(12);

    if (textureID != 0) {
        sprite->reuseTexture(textureID, textureSize);
        sprite->rect = spriteSize;
        return;
    }

    // Load texture
    sprite->loadTexture(catAtlasTexture, catAtlasTextureSize);

    // Position sprite
    sprite->setCenterX(screenArea.w / 2.0f);

    const float margin = 20 * screenScale;
    sprite->setRight(screenArea.w - margin);
    sprite->setBottom(screenArea.h - margin);

    // Store sprite info for reuse
    const auto it = sprite->getTexture();
    textureID = std::get<0>(it);
    textureSize = std::get<1>(it);
    spriteSize = sprite->rect;
}

void CatAnimationManager::fireSingleAnimation(const CatSpriteLayers layer, const CatAnimation animationId,
                                              const CatAnimation nextAnimationId) {
    Sprite *sprite = getSpriteByLayer(layer);
    sprite->setAnimation(animationId);
    singleFireAnimations.push_back({layer, animationId, nextAnimationId});
}

bool CatAnimationManager::getShouldLoop(const Sprite *sprite) {
    switch (const auto anim = static_cast<CatAnimation>(sprite->getCurrentAnimation())) {
        case CAT_EYES_OPEN:
            return false;
        case CAT_EYES_CLOSE:
            return false;
        default:
            return true;
    }
}
