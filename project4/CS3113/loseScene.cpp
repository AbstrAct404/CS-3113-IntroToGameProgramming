#include "LoseScene.h"
#include <iostream>

LoseScene::LoseScene()                                      : Scene { {0.0f}, nullptr   } {}
LoseScene::LoseScene(Vector2 origin, const char *bgHexCode) : Scene { origin, bgHexCode } {}

LoseScene::~LoseScene() { }

void LoseScene::initialise()
{
   Scene::initialise();
   mGameState.nextSceneID = -1;
}

void LoseScene::update(float deltaTime)
{
   if (IsKeyPressed(KEY_R)) mGameState.nextSceneID = 3;
}

void LoseScene::render()
{
   ClearBackground(ColorFromHex(mBGColourHexCode));
   
}

void LoseScene::renderUI() {
   const char* text = "\t\t\t\t\tYOU LOST\n\nPress R to Restart";
   DrawText(text, 250, mOrigin.y-40, 40, WHITE);
   const char* text1 = "ps:\n\nYou may attack to kill some of the enemies and even traps,\n\nBUT NOT ALL OF THEM!";
   DrawText(text1, 180, mOrigin.y+100, 20, WHITE);
}

void LoseScene::shutdown()
{
   printf("lost::shutdown()");
   Scene::shutdown();
}