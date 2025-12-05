#include "StartScreen.h"
#include <iostream>

StartScreen::StartScreen()                                      : Scene { {0.0f}, nullptr   } {}
StartScreen::StartScreen(Vector2 origin, const char *bgHexCode) : Scene { origin, bgHexCode } {}

StartScreen::~StartScreen() {  }

void StartScreen::initialise()
{
   Scene::initialise();
   mGameState.nextSceneID = -1;
}

void StartScreen::update(float deltaTime)
{
   if (IsKeyPressed(KEY_ENTER)) mGameState.nextSceneID = 0;
}

void StartScreen::render()
{
   ClearBackground(ColorFromHex(mBGColourHexCode));
   
}

void StartScreen::renderUI() {
   const char* text = "STAY ALIVE!!\n\nPress Enter to Start";
   DrawText(text, 100, mOrigin.y-100, 40, WHITE);
   const char* text1 = "ps:\n\nWASD for moving, J for attacking";
   DrawText(text1, 100, mOrigin.y+70, 20, WHITE);
   const char* text2 = "pss:\n\nAPPLE IS POISONOUS!!!!";
   DrawText(text2, 100, mOrigin.y+130, 20, RED);
}

void StartScreen::shutdown()
{ 
   printf("start::shutdown()");
   Scene::shutdown();
}