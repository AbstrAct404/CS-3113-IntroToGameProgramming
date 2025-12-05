#include "LevelCTitle.h"
#include <iostream>

LevelCTitle::LevelCTitle() : Scene{{0.0f}, nullptr} {}
LevelCTitle::LevelCTitle(Vector2 origin, const char *bgHexCode) : Scene{origin, bgHexCode} {}
LevelCTitle::~LevelCTitle() {}

void LevelCTitle::initialise()
{
    Scene::initialise();
    mGameState.nextSceneID = -1;
    mTimer = 0.0f;
}

void LevelCTitle::update(float deltaTime)
{
    if (IsKeyPressed(KEY_ENTER))
    {
        mGameState.nextSceneID = 2; // Go to LevelC
    }
}

void LevelCTitle::render()
{
    ClearBackground(BLACK);
    
    const char* title = "LEVEL C: STAY ALIVE";
    int fontSize = 48;
    int titleWidth = MeasureText(title, fontSize);
    
    DrawText(title, GetScreenWidth()/2 - titleWidth/2, GetScreenHeight()/2 - 50, fontSize, WHITE);
    
    const char* prompt = "Press ENTER to continue";
    int promptSize = 24;
    int promptWidth = MeasureText(prompt, promptSize);
    DrawText(prompt, GetScreenWidth()/2 - promptWidth/2, GetScreenHeight()/2 + 50, promptSize, GRAY);
}

void LevelCTitle::renderUI()
{
    // Empty
}

void LevelCTitle::shutdown()
{
    Scene::shutdown();
}

