#include "apps/matrix_cat_extension.h"
#include "cat_atlas_texture.h"

CatAnimationManager::CatAnimationManager(CatData &catData, float screenScale, Rect screenSize) : catData(catData),
    screenArea(catData.screenArea),
    screenScale(screenScale),
    screenSize(screenSize) {
    initializeSprite(eyesSprite, CAT_EYES_IDLE);
    initializeSprite(faceSprite, CAT_FACE_IDLE);
    initializeSprite(hairSprite, CAT_HAIR_IDLE);
    initializeSprite(clothesSprite, CAT_CLOTHES_IDLE);
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
            sprite->setFPS(12); // reset to default FPS
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
}

void CatAnimationManager::logic(const float deltaTime) {
    // Increment timers
    blinkTimer += deltaTime;
    sleepTimer += deltaTime;

    // Check for random events
    if (const auto anim = eyesSprite->getCurrentAnimation();
        (anim == CAT_EYES_IDLE) && blinkTimer > 3) {
        fireSingleAnimation(CAT_ALL_BLINK, CAT_ALL_IDLE);
        if (rand() % 100 < 10) {
            blinkTimer -= 1.0f; // shorter time to next blink
        } else {
            blinkTimer = 0.0f; // reset timer
        }
    }
    if (const auto anim = eyesSprite->getCurrentAnimation();
        (anim == CAT_EYES_IDLE) && sleepTimer > 15) {
        fireSingleAnimation(CAT_ALL_SLEEP_IN, CAT_ALL_SLEEP_LOOP);
    }
    if (const auto anim = eyesSprite->getCurrentAnimation();
        (anim == CAT_EYES_SLEEP_LOOP) && sleepTimer > 25) {
        fireSingleAnimation(CAT_ALL_SLEEP_OUT, CAT_ALL_IDLE);
        sleepTimer = 0.0f;
        blinkTimer = 0.0f;
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
        1.25f
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
                                              const CatAnimation nextAnimationId, const int fps) {
    Sprite *sprite = getSpriteByLayer(layer);
    sprite->setAnimation(animationId);
    sprite->setFPS(fps);
    singleFireAnimations.push_back({layer, animationId, nextAnimationId});
}

void CatAnimationManager::fireSingleAnimation(const CatAllAnimation &animation,
                                              const CatAllAnimation &nextAnimation) {
    fireSingleAnimation(LAYER_EYES, animation.eyesAnimation, nextAnimation.eyesAnimation, animation.fps);
    fireSingleAnimation(LAYER_FACE, animation.faceAnimation, nextAnimation.faceAnimation, animation.fps);
    fireSingleAnimation(LAYER_HAIR, animation.hairAnimation, nextAnimation.hairAnimation, animation.fps);
    fireSingleAnimation(LAYER_CLOTHES, animation.clothesAnimation, nextAnimation.clothesAnimation, animation.fps);
}


void CatAnimationManager::fireLoopAnimation(const CatSpriteLayers layer, const CatAnimation animationId, const int fps) {
    Sprite *sprite = getSpriteByLayer(layer);
    sprite->setAnimation(animationId);
    sprite->setFPS(fps);
}

void CatAnimationManager::fireLoopAnimation(const CatAllAnimation &animation) {
    fireLoopAnimation(LAYER_EYES, animation.eyesAnimation, animation.fps);
    fireLoopAnimation(LAYER_FACE, animation.faceAnimation, animation.fps);
    fireLoopAnimation(LAYER_HAIR, animation.hairAnimation, animation.fps);
    fireLoopAnimation(LAYER_CLOTHES, animation.clothesAnimation, animation.fps);
}

bool CatAnimationManager::getShouldLoop(const Sprite *sprite) {
    switch (const auto anim = static_cast<CatAnimation>(sprite->getCurrentAnimation())) {
        default:
            return true;
    }
}
