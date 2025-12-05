#include "wonScene.h"
#include "LevelC.h"
#include <iostream>
#include <cstdio>

WonScene::WonScene()                                      : Scene { {0.0f}, nullptr   } {}
WonScene::WonScene(Vector2 origin, const char *bgHexCode) : Scene { origin, bgHexCode } {}

WonScene::~WonScene() { }

void WonScene::initialise()
{
   Scene::initialise();
   mGameState.nextSceneID = -1;
}

void WonScene::update(float deltaTime)
{
   if (IsKeyPressed(KEY_R)) mGameState.nextSceneID = 3;
}

void WonScene::render()
{
   ClearBackground(ColorFromHex(mBGColourHexCode));
}

void WonScene::renderUI() {
   int screenWidth = GetScreenWidth();
   int screenHeight = GetScreenHeight();
   
   // Title - YOU WON
   const char* title = "YOU WON!";
   int titleSize = 60;
   int titleWidth = MeasureText(title, titleSize);
   DrawText(title, screenWidth/2 - titleWidth/2, screenHeight/2 - 180, titleSize, GOLD);
   
   // Congratulations
   const char* congrats = "CONGRATULATIONS!";
   int congratsSize = 36;
   int congratsWidth = MeasureText(congrats, congratsSize);
   DrawText(congrats, screenWidth/2 - congratsWidth/2, screenHeight/2 - 110, congratsSize, GREEN);
   
   // Kill Statistics
   const char* statsTitle = "- KILL STATISTICS -";
   int statsTitleSize = 30;
   int statsTitleWidth = MeasureText(statsTitle, statsTitleSize);
   DrawText(statsTitle, screenWidth/2 - statsTitleWidth/2, screenHeight/2 - 50, statsTitleSize, WHITE);
   
   // Total kills
   char totalText[64];
   snprintf(totalText, sizeof(totalText), "Total Kills: %d", gTotalKills);
   int totalSize = 28;
   int totalWidth = MeasureText(totalText, totalSize);
   DrawText(totalText, screenWidth/2 - totalWidth/2, screenHeight/2 - 10, totalSize, GOLD);
   
   // Kill breakdown
   char followerText[64];
   snprintf(followerText, sizeof(followerText), "Followers: %d", gFollowerKills);
   int breakdownSize = 22;
   int followerWidth = MeasureText(followerText, breakdownSize);
   DrawText(followerText, screenWidth/2 - followerWidth/2, screenHeight/2 + 30, breakdownSize, LIGHTGRAY);
   
   char wandererText[64];
   snprintf(wandererText, sizeof(wandererText), "Wanderers: %d", gWandererKills);
   int wandererWidth = MeasureText(wandererText, breakdownSize);
   DrawText(wandererText, screenWidth/2 - wandererWidth/2, screenHeight/2 + 60, breakdownSize, LIGHTGRAY);
   
   char flyerText[64];
   snprintf(flyerText, sizeof(flyerText), "Flyers: %d", gFlyerKills);
   int flyerWidth = MeasureText(flyerText, breakdownSize);
   DrawText(flyerText, screenWidth/2 - flyerWidth/2, screenHeight/2 + 90, breakdownSize, LIGHTGRAY);
   
   // Play again prompt
   const char* prompt = "Press R to Play Again";
   int promptSize = 24;
   int promptWidth = MeasureText(prompt, promptSize);
   DrawText(prompt, screenWidth/2 - promptWidth/2, screenHeight/2 + 150, promptSize, GRAY);
}

void WonScene::shutdown()
{
   // printf("WonScene::shutdown()");
   Scene::shutdown();
}
