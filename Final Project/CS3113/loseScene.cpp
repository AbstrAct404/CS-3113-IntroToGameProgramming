#include "loseScene.h"
#include "LevelC.h"
#include <iostream>
#include <cstdio>

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
   // Press R to restart game (jump to scene -2, handled by main.cpp)
   if (IsKeyPressed(KEY_R))
   {
      // Use special value -2 to restart game
      mGameState.nextSceneID = -2;
   }
}

void LoseScene::render()
{
   ClearBackground(ColorFromHex(mBGColourHexCode));
}

void LoseScene::renderUI() {
   int screenWidth = GetScreenWidth();
   int screenHeight = GetScreenHeight();
   
   // Title - YOU LOST
   const char* title = "YOU LOST";
   int titleSize = 60;
   int titleWidth = MeasureText(title, titleSize);
   DrawText(title, screenWidth/2 - titleWidth/2, screenHeight/2 - 150, titleSize, RED);
   
   // Kill Statistics
   const char* statsTitle = "- KILL STATISTICS -";
   int statsTitleSize = 30;
   int statsTitleWidth = MeasureText(statsTitle, statsTitleSize);
   DrawText(statsTitle, screenWidth/2 - statsTitleWidth/2, screenHeight/2 - 70, statsTitleSize, WHITE);
   
   // Total kills
   char totalText[64];
   snprintf(totalText, sizeof(totalText), "Total Kills: %d", gTotalKills);
   int totalSize = 28;
   int totalWidth = MeasureText(totalText, totalSize);
   DrawText(totalText, screenWidth/2 - totalWidth/2, screenHeight/2 - 30, totalSize, GOLD);
   
   // Kill breakdown
   char followerText[64];
   snprintf(followerText, sizeof(followerText), "Followers: %d", gFollowerKills);
   int breakdownSize = 22;
   int followerWidth = MeasureText(followerText, breakdownSize);
   DrawText(followerText, screenWidth/2 - followerWidth/2, screenHeight/2 + 10, breakdownSize, LIGHTGRAY);
   
   char wandererText[64];
   snprintf(wandererText, sizeof(wandererText), "Wanderers: %d", gWandererKills);
   int wandererWidth = MeasureText(wandererText, breakdownSize);
   DrawText(wandererText, screenWidth/2 - wandererWidth/2, screenHeight/2 + 40, breakdownSize, LIGHTGRAY);
   
   char flyerText[64];
   snprintf(flyerText, sizeof(flyerText), "Flyers: %d", gFlyerKills);
   int flyerWidth = MeasureText(flyerText, breakdownSize);
   DrawText(flyerText, screenWidth/2 - flyerWidth/2, screenHeight/2 + 70, breakdownSize, LIGHTGRAY);
   
   // Restart prompt
   const char* prompt = "Press R to Restart";
   int promptSize = 24;
   int promptWidth = MeasureText(prompt, promptSize);
   DrawText(prompt, screenWidth/2 - promptWidth/2, screenHeight/2 + 130, promptSize, GRAY);
}

void LoseScene::shutdown()
{
   // printf("LoseScene::shutdown()");
   Scene::shutdown();
}
