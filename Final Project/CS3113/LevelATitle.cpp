#include "LevelATitle.h"
#include <iostream>

LevelATitle::LevelATitle() : Scene{{0.0f}, nullptr} {}
LevelATitle::LevelATitle(Vector2 origin, const char *bgHexCode) : Scene{origin, bgHexCode} {}
LevelATitle::~LevelATitle() {}

void LevelATitle::initialise()
{
    Scene::initialise();
    mGameState.nextSceneID = -1;
    mTimer = 0.0f;
}

void LevelATitle::update(float deltaTime)
{
    // 跳关功能：按P键跳过LevelA直接进入LevelB
    if (IsKeyPressed(KEY_P))
    {
        mGameState.nextSceneID = 4; // Go to LevelBTitle
        return;
    }
    
    if (IsKeyPressed(KEY_ENTER))
    {
        mGameState.nextSceneID = 0; // Go to LevelA
    }
}

void LevelATitle::render()
{
    ClearBackground(BLACK);
    
    const char* title = "LEVEL A: UPGRADE TUTORIAL";
    int fontSize = 48;
    int titleWidth = MeasureText(title, fontSize);
    
    DrawText(title, GetScreenWidth()/2 - titleWidth/2, GetScreenHeight()/2 - 50, fontSize, WHITE);
    
    const char* prompt = "Press ENTER to continue";
    int promptSize = 24;
    int promptWidth = MeasureText(prompt, promptSize);
    DrawText(prompt, GetScreenWidth()/2 - promptWidth/2, GetScreenHeight()/2 + 50, promptSize, GRAY);
    
    const char* skip = "Press P to skip to Level B";
    int skipSize = 20;
    int skipWidth = MeasureText(skip, skipSize);
    DrawText(skip, GetScreenWidth()/2 - skipWidth/2, GetScreenHeight()/2 + 100, skipSize, YELLOW);
}

void LevelATitle::renderUI()
{
    // Empty
}

void LevelATitle::shutdown()
{
    Scene::shutdown();
}

