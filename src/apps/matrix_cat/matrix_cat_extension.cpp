#include "apps/matrix_cat_extension.h"
#include <fstream>

#include "letter_atlas_texture.h"
#include "ui_atlas_texture.h"
#include "helper.h"
#include "sprite_glow_shader.h"

#include "../../../assets/sprites/include/letterAnimationsInfo.h"
#include "../../../assets/sprites/include/uiAnimationsInfo.h"
#include "apps/matrix.h"

void CatData::loadFile(const std::string& path) {
    const File file;
    if (!file.open(path, File::Mode::Read)) {
        saveFile(path);
        return;
    }

    // Read data

    if (!file.readBytes(&u_hunger, sizeof(u_hunger))) return;
    if (!file.readBytes(&u_energy, sizeof(u_energy))) return;
    if (!file.readBytes(&u_cleanliness, sizeof(u_cleanliness))) return;
    if (!file.readBytes(&u_affection, sizeof(u_affection))) return;
    if (!file.readBytes(&u_mood, sizeof(u_mood))) return;
    if (!file.readBytes(&u_playful, sizeof(u_playful))) return;
    if (!file.readBytes(&u_clingy, sizeof(u_clingy))) return;
    if (!file.readBytes(&u_lazy, sizeof(u_lazy))) return;
    if (!file.readBytes(&u_last_update, sizeof(u_last_update))) return;
}

void CatData::saveFile(const std::string &path) {
    const File file;
    if (!file.open(path, File::Mode::Write)) return;

    lastSaveTime = std::time(nullptr);

    // Write data
    if (!file.writeBytes(&u_hunger, sizeof(u_hunger))) return;
    if (!file.writeBytes(&u_energy, sizeof(u_energy))) return;
    if (!file.writeBytes(&u_cleanliness, sizeof(u_cleanliness))) return;
    if (!file.writeBytes(&u_affection, sizeof(u_affection))) return;
    if (!file.writeBytes(&u_mood, sizeof(u_mood))) return;
    if (!file.writeBytes(&u_playful, sizeof(u_playful))) return;
    if (!file.writeBytes(&u_clingy, sizeof(u_clingy))) return;
    if (!file.writeBytes(&u_lazy, sizeof(u_lazy))) return;
    if (!file.writeBytes(&u_last_update, sizeof(u_last_update))) return;
}

void CatData::initialize(const int width, const int height) {
    const float screenScale = static_cast<float>(height) / 1080.0f;
    const Rect screenSize = {0, 0, static_cast<float>(width), static_cast<float>(height)};

    catAnimation = new CatAnimationManager(*this, screenScale, screenSize);

    charSprite = new Sprite(
        letterAnimations,
        screenArea,
        screenSize,
        screenScale,
        0.5f
    );
    charSprite->loadTexture(letterAtlasTexture, letterAtlasTextureSize);
}

bool CatData::checkSaveRequired() const {
    return std::difftime(std::time(nullptr), lastSaveTime) >= CAT_SAVE_THRESHOLD;
}

void MatrixApp::catSetup() {
    cat.loadFile(rnd->opts->catDataPath.value_or("matrix_cat.data"));

    // Define cat screen area (20% of screen size in bottom-right corner)
    cat.screenArea.w = rnd->opts->width * 0.20f;
    cat.screenArea.h = cat.screenArea.w;

    cat.screenArea.x = rnd->opts->width - cat.screenArea.w;
    cat.screenArea.y = rnd->opts->height - cat.screenArea.h;

    cat.initialize(rnd->opts->width, rnd->opts->height);
}

void MatrixApp::catSaveData() {
    cat.saveFile(rnd->opts->catDataPath.value_or("matrix_cat.data"));
}

void MatrixApp::catLoop() {
    // Periodically save cat data
    if (cat.checkSaveRequired()) {
        catSaveData();
    }

    // Render the cat
    cat.catAnimation->render(rnd->clock->deltaTime);
    cat.catAnimation->logic(rnd->clock->deltaTime, *rnd->events);
}