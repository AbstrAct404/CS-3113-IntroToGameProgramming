#include "MenuScene.h"
#include <iostream>

MenuScene::MenuScene() : Scene{{0.0f}, nullptr} {}
MenuScene::MenuScene(Vector2 origin, const char *bgHexCode) : Scene{origin, bgHexCode} {}
MenuScene::~MenuScene() {}

void MenuScene::initialise()
{
    Scene::initialise();
    mGameState.nextSceneID = -1;
}

void MenuScene::update(float deltaTime)
{
    // Press Enter to start - goes to Level A Title (or Level A directly)
    if (IsKeyPressed(KEY_ENTER))
    {
        mGameState.nextSceneID = 3; // Go to Level A Title (or change to 0 for Level A directly)
    }
}

void MenuScene::render()
{
    ClearBackground(ColorFromHex(mBGColourHexCode));
}

void MenuScene::renderUI()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Game Title
    const char* gameTitle = "Ghost Survivors";
    int titleFontSize = 60;
    int titleWidth = MeasureText(gameTitle, titleFontSize);
    DrawText(
        gameTitle,
        (int)(screenWidth * 0.5f - titleWidth * 0.5f),
        (int)(screenHeight * 0.2f),
        titleFontSize,
        YELLOW
    );
    
    // Subtitle
    const char* subtitle = "Survive 2 Minutes!";
    int subtitleFontSize = 30;
    int subtitleWidth = MeasureText(subtitle, subtitleFontSize);
    DrawText(
        subtitle,
        (int)(screenWidth * 0.5f - subtitleWidth * 0.5f),
        (int)(screenHeight * 0.2f + 70),
        subtitleFontSize,
        WHITE
    );
    
    // Press Enter to Start
    const char* startText = "Press ENTER to Start";
    int startFontSize = 32;
    int startWidth = MeasureText(startText, startFontSize);
    
    // Blinking effect
    float time = GetTime();
    Color startColor = ((int)(time * 2) % 2 == 0) ? WHITE : GRAY;
    
    DrawText(
        startText,
        (int)(screenWidth * 0.5f - startWidth * 0.5f),
        (int)(screenHeight * 0.5f),
        startFontSize,
        startColor
    );
    
    // Instructions Section
    int instructionY = (int)(screenHeight * 0.65f);
    int instructionFontSize = 20;
    
    const char* instructionsTitle = "Instructions:";
    DrawText(
        instructionsTitle,
        (int)(screenWidth * 0.5f - MeasureText(instructionsTitle, instructionFontSize) * 0.5f),
        instructionY,
        instructionFontSize,
        LIGHTGRAY
    );
    
    // Instruction lines
    const char* instruction1 = "WASD - Move your character";
    const char* instruction2 = "Kill enemies to level up";
    const char* instruction3 = "Choose upgrades when you level up";
    const char* instruction4 = "Survive for 2 minutes to win!";
    const char* instruction5 = "Avoid enemies and their projectiles";
    
    int lineHeight = 30;
    int instructionX = (int)(screenWidth * 0.5f - 200);
    
    DrawText(instruction1, instructionX, instructionY + 35, instructionFontSize, WHITE);
    DrawText(instruction2, instructionX, instructionY + 35 + lineHeight, instructionFontSize, WHITE);
    DrawText(instruction3, instructionX, instructionY + 35 + lineHeight * 2, instructionFontSize, WHITE);
    DrawText(instruction4, instructionX, instructionY + 35 + lineHeight * 3, instructionFontSize, WHITE);
    DrawText(instruction5, instructionX, instructionY + 35 + lineHeight * 4, instructionFontSize, WHITE);
    
    // Controls hint at bottom
    const char* controlsHint = "Press Q to quit";
    DrawText(
        controlsHint,
        screenWidth - MeasureText(controlsHint, 18) - 20,
        screenHeight - 30,
        18,
        GRAY
    );
}

void MenuScene::shutdown()
{
    // printf("MenuScene::shutdown()\n");
    Scene::shutdown();
}

