#include "LevelBTitle.h"
#include <iostream>

LevelBTitle::LevelBTitle() : Scene{{0.0f}, nullptr} {}
LevelBTitle::LevelBTitle(Vector2 origin, const char *bgHexCode) : Scene{origin, bgHexCode} {}
LevelBTitle::~LevelBTitle() {}

void LevelBTitle::initialise()
{
    Scene::initialise();
    mGameState.nextSceneID = -1;
    mTimer = 0.0f;
}

void LevelBTitle::update(float deltaTime)
{
    // 跳关功能：按P键跳过LevelB直接进入LevelC
    if (IsKeyPressed(KEY_P))
    {
        mGameState.nextSceneID = 5; // Go to LevelCTitle
        return;
    }
    
    if (IsKeyPressed(KEY_ENTER))
    {
        mGameState.nextSceneID = 1; // Go to LevelB
    }
}

void LevelBTitle::render()
{
    ClearBackground(BLACK);
    
    const char* title = "LEVEL B: ATTACKING TUTORIAL";
    int fontSize = 48;
    int titleWidth = MeasureText(title, fontSize);
    
    DrawText(title, GetScreenWidth()/2 - titleWidth/2, GetScreenHeight()/2 - 50, fontSize, WHITE);
    
    const char* prompt = "Press ENTER to continue";
    int promptSize = 24;
    int promptWidth = MeasureText(prompt, promptSize);
    DrawText(prompt, GetScreenWidth()/2 - promptWidth/2, GetScreenHeight()/2 + 50, promptSize, GRAY);
    
    const char* skip = "Press P to skip to Level C";
    int skipSize = 20;
    int skipWidth = MeasureText(skip, skipSize);
    DrawText(skip, GetScreenWidth()/2 - skipWidth/2, GetScreenHeight()/2 + 100, skipSize, YELLOW);
}

void LevelBTitle::renderUI()
{
    // Empty
}

void LevelBTitle::shutdown()
{
    Scene::shutdown();
}

