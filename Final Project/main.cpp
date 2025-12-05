/**
* Author: Aaron Zhu
* Assignment: Ghost Survivors
* Date due: Dec 5, 2:00pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "CS3113/LevelA.h"
#include "CS3113/LevelB.h"
#include "CS3113/LevelC.h"
#include "CS3113/LevelATitle.h"
#include "CS3113/LevelBTitle.h"
#include "CS3113/LevelCTitle.h"
#include "CS3113/loseScene.h"
#include "CS3113/wonScene.h"
#include "CS3113/MenuScene.h"
#include "CS3113/ShaderProgram.h"

// Global Constants
constexpr int SCREEN_WIDTH     = 1600,
              SCREEN_HEIGHT    = 800,
              FPS              = 120,
              NUMBER_OF_LEVELS = 9;

constexpr Vector2 ORIGIN = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;

// Global Variables
AppStatus gAppStatus   = RUNNING;
float gPreviousTicks   = 0.0f,
      gTimeAccumulator = 0.0f;

Scene *gCurrentScene = nullptr;
std::vector<Scene*> gLevels = {};

LevelA *gLevelA = nullptr;
LevelB *gLevelB = nullptr;
LevelC *gLevelC = nullptr;
LevelATitle *gLevelATitle = nullptr;
LevelBTitle *gLevelBTitle = nullptr;
LevelCTitle *gLevelCTitle = nullptr;
LoseScene *gLoseScene = nullptr;
WonScene *gWonScene = nullptr;
MenuScene *gMenuScene = nullptr;

Music bgm;
ShaderProgram gShader;
Vector2 gLightPosition = { 0.0f, 0.0f };
Sound gNextLevelSound = {0};

// Function Declarations
void switchToScene(Scene *scene);
void initialise();
void processInput();
void update();
void render();
void shutdown();

void switchToScene(Scene *scene)
{
    if (gCurrentScene != nullptr) gCurrentScene->shutdown();
    
    bool isGoingToLoseScene = (scene == gLoseScene);
    if (gCurrentScene != nullptr && gNextLevelSound.frameCount > 0 && !isGoingToLoseScene)
    {
        PlaySound(gNextLevelSound);
    }
    
    gCurrentScene = scene;
    gCurrentScene->initialise();
}

void initialiseScene() {
    gLevelA = new LevelA(ORIGIN, "#000000");
    gLevelB = new LevelB(ORIGIN, "#000000");
    gLevelC = new LevelC(ORIGIN, "#000000");
    gLevelATitle = new LevelATitle(ORIGIN, "#000000");
    gLevelBTitle = new LevelBTitle(ORIGIN, "#000000");
    gLevelCTitle = new LevelCTitle(ORIGIN, "#000000");
    gLoseScene = new LoseScene(ORIGIN, "#000000");
    gWonScene = new WonScene(ORIGIN, "#000000");
    gMenuScene = new MenuScene(ORIGIN, "#1a1a2e");

    gLevels.push_back(gLevelA);
    gLevels.push_back(gLevelB);
    gLevels.push_back(gLevelC);
    gLevels.push_back(gLevelATitle);
    gLevels.push_back(gLevelBTitle);
    gLevels.push_back(gLevelCTitle);
    gLevels.push_back(gMenuScene);
    gLevels.push_back(gLoseScene);
    gLevels.push_back(gWonScene);

    switchToScene(gMenuScene);
    // printf("Game initialized - Starting Menu Scene\n");
}

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vampire Survivors Clone - Survive 2 Minutes!");
    InitAudioDevice();
    SetTraceLogLevel(LOG_WARNING);
    
    gShader.load("assets/lighting.vs", "assets/lighting.fs");
    gNextLevelSound = LoadSound("assets/nextLevel.wav");
    SetSoundVolume(gNextLevelSound, 0.5f);
    
    initialiseScene();
    
    bgm = LoadMusicStream("assets/bgm.wav");
    SetMusicVolume(bgm, 0.33f);
    PlayMusicStream(bgm);
    SetTargetFPS(FPS);
}

void processInput() 
{
    if (gCurrentScene->getState().xochitl != nullptr)
    {
        gCurrentScene->getState().xochitl->resetMovement();

        if (IsKeyDown(KEY_A)) gCurrentScene->getState().xochitl->moveLeft();
        if (IsKeyDown(KEY_D)) gCurrentScene->getState().xochitl->moveRight();
        if (IsKeyDown(KEY_W)) gCurrentScene->getState().xochitl->moveUp();
        if (IsKeyDown(KEY_S)) gCurrentScene->getState().xochitl->moveDown();

        if (GetLength(gCurrentScene->getState().xochitl->getMovement()) > 1.0f) 
            gCurrentScene->getState().xochitl->normaliseMovement();
    }

    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) gAppStatus = TERMINATED;
    
    if (IsKeyPressed(KEY_R)) {
        bool isLoseOrWinScene = (gCurrentScene == gLoseScene || gCurrentScene == gWonScene);
        if (!isLoseOrWinScene) {
            if (gCurrentScene != nullptr) {
                gCurrentScene->shutdown();
            }
            for (size_t i = 0; i < gLevels.size(); ++i) {
                if (gLevels[i] != nullptr) {
                    delete gLevels[i];
                    gLevels[i] = nullptr;
                }
            }
            gLevels.clear();
            gLevelA = nullptr;
            gLevelB = nullptr;
            gLevelC = nullptr;
            gLevelATitle = nullptr;
            gLevelBTitle = nullptr;
            gLevelCTitle = nullptr;
            gLoseScene = nullptr;
            gWonScene = nullptr;
            gMenuScene = nullptr;
            gCurrentScene = nullptr;
            initialiseScene();
        }
    }
}

void update() 
{
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    deltaTime += gTimeAccumulator; 

    if (deltaTime < FIXED_TIMESTEP)
    {
        gTimeAccumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP)
    {
        bool isLevelABC = (gCurrentScene == gLevelA || gCurrentScene == gLevelB || gCurrentScene == gLevelC);
        if (!isLevelABC)
        {
            UpdateMusicStream(bgm);
            if (IsMusicStreamPlaying(bgm) == false)
            {
                PlayMusicStream(bgm);
            }
        }
        gCurrentScene->update(FIXED_TIMESTEP);
        
        if (gCurrentScene->getState().xochitl != nullptr)
        {
            gLightPosition = gCurrentScene->getState().xochitl->getPosition();
        }
        
        deltaTime -= FIXED_TIMESTEP;
    }

    int nextID = gCurrentScene->getState().nextSceneID;
    
    if (nextID == -2)
    {
        // printf("Restarting game from LoseScene\n");
        if (gCurrentScene != nullptr)
        {
            gCurrentScene->shutdown();
        }
        for (size_t i = 0; i < gLevels.size(); ++i)
        {
            if (gLevels[i] != nullptr)
            {
                delete gLevels[i];
                gLevels[i] = nullptr;
            }
        }
        gLevels.clear();
        gLevelA = nullptr;
        gLevelB = nullptr;
        gLevelC = nullptr;
        gLevelATitle = nullptr;
        gLevelBTitle = nullptr;
        gLevelCTitle = nullptr;
        gLoseScene = nullptr;
        gWonScene = nullptr;
        gMenuScene = nullptr;
        gCurrentScene = nullptr;
        initialiseScene();
        return;
    }
    
    if (nextID >= 0 && nextID < (int)gLevels.size() && gLevels[nextID] != nullptr)
    {
        // printf("Switching to scene %d\n", nextID);
        switchToScene(gLevels[nextID]);
    }
}

void render()
{
    BeginDrawing();
    
    if (gCurrentScene->getState().xochitl != nullptr) {
        BeginMode2D(gCurrentScene->getState().camera);
    }
    
    gShader.begin();
    if (gShader.isLoaded())
    {
        gShader.setVector2("lightPosition", gLightPosition);
    }
    gCurrentScene->render();  
    gShader.end();
    
    if (gCurrentScene->getState().xochitl != nullptr) {
        EndMode2D();
    }
    
    gCurrentScene->renderUI(); 
    DrawText("Press R to restart | Press Q to quit", GetScreenWidth() - 350, GetScreenHeight() - 25, 14, GRAY);
    
    EndDrawing();
}

void shutdown() 
{
    for (size_t i = 0; i < gLevels.size(); ++i) delete gLevels[i];
    gLevels.clear();
    UnloadMusicStream(bgm);
    if (gNextLevelSound.frameCount > 0)
    {
        UnloadSound(gNextLevelSound);
    }
    gShader.unload();
    CloseAudioDevice();
    CloseWindow();
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();
    return 0;
}
