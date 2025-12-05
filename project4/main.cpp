/**
* Author: Aaron Zhu
* Assignment: Rise of the AI
* Date due: 2025-11-08, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#include "CS3113/LevelC.h"
#include "CS3113/startScreen.h"
#include "CS3113/wonScene.h"
#include "CS3113/loseScene.h"

// Global Constants
constexpr int SCREEN_WIDTH     = 1000,
              SCREEN_HEIGHT    = 600,
              FPS              = 120,
              NUMBER_OF_LEVELS = 3;

constexpr Vector2 ORIGIN      = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
            
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
StartScreen *gStartScreen = nullptr;
LoseScene *gLostScene = nullptr;
WonScene *gWonScene = nullptr;

Music bgm;
Sound levelCompletedSound = LoadSound("assets/levelCompleted.wav");

// Function Declarations
void switchToScene(Scene *scene, bool levelSwitch);
void initialise();
void processInput();
void update();
void render();
void shutdown();

void switchToScene(Scene *scene, bool levelSwitch)
{
    if (gCurrentScene != nullptr) gCurrentScene->shutdown();
    
    gCurrentScene = scene;
    gCurrentScene->initialise();
}


void initialiseScene() {
    
    gLevelA = new LevelA(ORIGIN, "#39C5BB");
    gLevelB = new LevelB(ORIGIN, "#3e25ffff");
    gLevelC = new LevelC(ORIGIN, "#ff0000ff");
    gStartScreen = new StartScreen(ORIGIN, "#000000");
    gLostScene = new LoseScene(ORIGIN, "#000000");
    gWonScene = new WonScene(ORIGIN, "#000000");

    gLevels.push_back(gLevelA);
    gLevels.push_back(gLevelB);
    gLevels.push_back(gLevelC);
    gLevels.push_back(gStartScreen);
    gLevels.push_back(gLostScene);
    gLevels.push_back(gWonScene);

    switchToScene(gLevels[3], false);

    printf("initialized\n");


}

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Scenes");
    InitAudioDevice();
    initialiseScene();
    
    bgm = LoadMusicStream("assets/BGM01hero.wav");
    SetMusicVolume(bgm, 0.33f);
    PlayMusicStream(bgm);
    SetTargetFPS(FPS);
}


void processInput() 
{
    if (gCurrentScene->getState().xochitl != nullptr)
    {
        gCurrentScene->getState().xochitl->resetMovement();

        if      (IsKeyDown(KEY_A)) gCurrentScene->getState().xochitl->moveLeft();
        else if (IsKeyDown(KEY_D)) gCurrentScene->getState().xochitl->moveRight();

        if (IsKeyPressed(KEY_W) && 
            gCurrentScene->getState().xochitl->isCollidingBottom())
        {
            gCurrentScene->getState().xochitl->jump();
            PlaySound(gCurrentScene->getState().jumpSound);
        }

        if (GetLength(gCurrentScene->getState().xochitl->getMovement()) > 1.0f) 
            gCurrentScene->getState().xochitl->normaliseMovement();

        if (IsKeyPressed(KEY_J)) {
            gCurrentScene->getState().xochitl->setEntityState(ATTACK);
            PlaySound(gCurrentScene->getState().attackSound);
        }
    }

    if (IsKeyDown(KEY_ENTER)) gCurrentScene->input(KEY_ENTER);

    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) gAppStatus = TERMINATED;
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
        UpdateMusicStream(bgm);
        gCurrentScene->update(FIXED_TIMESTEP);
        deltaTime -= FIXED_TIMESTEP;
    }
}
void render()
{
    BeginDrawing();
    
    if (gCurrentScene->getState().xochitl != nullptr) {
        BeginMode2D(gCurrentScene->getState().camera);
    }

    gCurrentScene->render();  

    if (gCurrentScene->getState().xochitl != nullptr) {
        EndMode2D();
    }
    
    gCurrentScene->renderUI(); 
    
    EndDrawing();
}

void shutdown() 
{
    for (size_t i = 0; i < gLevels.size(); ++i) delete gLevels[i];
    gLevels.clear();
    UnloadMusicStream(bgm);
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

        if (gCurrentScene->getState().nextSceneID >= 0)
        {
            int id = gCurrentScene->getState().nextSceneID;
            if (gLevels[id] == gCurrentScene) PlaySound(levelCompletedSound);
            if (id == 3) {
                Scene::lives = 3; 
                gCurrentScene = nullptr;
                for (size_t i = 0; i < gLevels.size(); ++i) delete gLevels[i];
                gLevels.clear();
                initialiseScene();
                continue;
            }
            else if (id == 1 || id ==2) switchToScene(gLevels[id], true);
            else switchToScene(gLevels[id], false);
        }

        render();
    }

    shutdown();

    return 0;
}