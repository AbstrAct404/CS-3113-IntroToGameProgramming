#include "Scene.h"

Scene::Scene() : mOrigin{{}} {
    ClearBackground(WHITE);
}

int Scene::lives = 999;

Scene::Scene(Vector2 origin, const char *bgHexCode) : mOrigin{origin}, mBGColourHexCode {bgHexCode}
{
    ClearBackground(ColorFromHex(bgHexCode));
}

void Scene::initialise() {  
   mGameState.jumpSound = LoadSound("assets/jump.wav");
   SetSoundVolume( mGameState.jumpSound, 0.5f);
   mGameState.attackSound = LoadSound("assets/attack.wav");
}

void Scene::input(KeyboardKey key) {
    mGameState.key = key;
} 

Scene::~Scene() { }

void Scene::shutdown() {
    delete mGameState.xochitl;
    mGameState.xochitl = nullptr;
    delete mGameState.map;
    mGameState.map = nullptr;
    for (size_t i = 0; i < mGameState.collidableEntities.size(); ++i) {
        delete mGameState.collidableEntities[i];
    }
    mGameState.collidableEntities.clear();  

    for (size_t i = 0; i < mGameState.hearts.size(); ++i) {
        delete mGameState.hearts[i];
    }
    mGameState.hearts.clear();
    
    if (mGameState.jumpSound.frameCount > 0) {
        StopSound(mGameState.jumpSound);
        UnloadSound(mGameState.jumpSound);
    }
    if (mGameState.attackSound.frameCount > 0) {
        StopSound(mGameState.attackSound);
        UnloadSound(mGameState.attackSound);
    }
}

