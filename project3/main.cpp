/**
* Author: Aaron Zhu
* Assignment: Lunar Lander
* Date due: 2025-10-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "CS3113/Entity.h"

enum gameStatus { INPROGRESS, WON, LOST };

// Global Constants
constexpr int SCREEN_WIDTH  = 1000,
              SCREEN_HEIGHT = 600,
              FPS           = 120;

constexpr char BG_COLOUR[]    = "#39C5BB";
constexpr Vector2 ORIGIN      = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

constexpr int   NUMBER_OF_TILES         = 20,
                NUMBER_OF_BLOCKS        = 3;
constexpr float TILE_DIMENSION          = 75.0f,
                // in m/ms², since delta time is in ms
                FIXED_TIMESTEP          = 1.0f / 60.0f;

float ACCELERATION_G = 9.81f; // in m/ms²

// Global Variables
AppStatus gAppStatus   = RUNNING;
float gPreviousTicks   = 0.0f,
      gTimeAccumulator = 0.0f;

gameStatus gGameStatus = INPROGRESS;

Entity *gXochitl = nullptr;
Entity *gTiles  = nullptr;
Entity gTarget = Entity();
int fuel = 1000;


// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LUNAR LANDER");

    /*
        ----------- PROTAGONIST -----------
    */
    std::map<Direction, std::vector<int>> animationAtlas = {
        {LEFT,  { 8, 9, 10, 11, 12, 13, 14, 15 }},
        {RIGHT, { 40, 41, 42, 43, 44, 45, 46, 47 }},
        {UP,    { 24, 25, 26, 27, 28, 29, 30, 31 }},
        {DOWN,  { 0, 1, 2, 3, 4, 5, 6, 7 }},
    };

    float sizeRatio  = 48.0f / 64.0f;

    gXochitl = new Entity(
        {ORIGIN.x, ORIGIN.y-100}, // position
        {70.0f, 70.0f},  // scale
        "assets/dash.png",        // texture file address
        ATLAS,                         // single image or atlas?
        { 6, 8 },                      // atlas dimensions
        animationAtlas,                 // actual atlas
        PLAYER
    );

    gXochitl->setEntityType(PLAYER);
    gXochitl->setColliderDimensions({
        gXochitl->getScale().x / 2.0f,
        gXochitl->getScale().y / 2.0f
    });

    gXochitl->setAcceleration({0.0f, ACCELERATION_G});

    /*
        ----------- TILES -----------
    */
    gTiles = new Entity[NUMBER_OF_TILES];

    // Compute the left‑most x coordinate so that the entire row is centred
    float leftMostX = ORIGIN.x - (NUMBER_OF_TILES * TILE_DIMENSION) / 2.0f;

    for (int i = 0; i < NUMBER_OF_TILES - 1; i++) 
    {
        gTiles[i].setTexture("assets/IndustrialTile_67.png");
        gTiles[i].setScale({TILE_DIMENSION, TILE_DIMENSION / 2});
        gTiles[i].setColliderDimensions({TILE_DIMENSION, TILE_DIMENSION / 2});
        gTiles[i].setPosition({
            leftMostX + i * TILE_DIMENSION, 
            ORIGIN.y + 100 + TILE_DIMENSION
        });
    }

    gTarget.setTexture("assets/tile_0000.png");
    gTarget.setScale({TILE_DIMENSION / 2, TILE_DIMENSION / 2});
    gTarget.setColliderDimensions({TILE_DIMENSION / 2, TILE_DIMENSION / 2});
    gTarget.setPosition({ORIGIN.x, ORIGIN.y + 50 + TILE_DIMENSION});
    gTarget.setEntityType(TARGET);

    SetTargetFPS(FPS);
}

void processInput() 
{
    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) gAppStatus = TERMINATED;
    if (gGameStatus != INPROGRESS) return;
    gXochitl->resetMovement();
    Vector2 acc = gXochitl->getAcceleration();

    // set acc for x directions
    if ((IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) && fuel > 0) {
        gXochitl->setAcceleration({-1500, acc.y});
        gXochitl->setDirection(LEFT);
        --fuel;
    }
    else if ((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) && fuel > 0) {
        gXochitl->setAcceleration({1500, acc.y});
        gXochitl->setDirection(RIGHT);
        --fuel;
    }

    // Get acceleration after modifying X
    acc = gXochitl->getAcceleration();
    
    if ((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) && fuel > 0) {
        gXochitl->setAcceleration({acc.x, -9.81f});
        gXochitl->setDirection(UP);
        --fuel;
    }
    else {
        gXochitl->setAcceleration({acc.x, ACCELERATION_G});
    }

    if (!(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))) {
        gXochitl->setDirection(DOWN);
    }


    // to avoid faster diagonal speed
    if (GetLength(gXochitl->getMovement()) > 1.0f) 
        gXochitl->normaliseMovement();

}

void update() 
{
    // Delta time
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    // Fixed timestep
    deltaTime += gTimeAccumulator;

    if (deltaTime < FIXED_TIMESTEP)
    {
        gTimeAccumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP)
    {
        gXochitl->update(FIXED_TIMESTEP, nullptr,nullptr, gTiles, NUMBER_OF_TILES);

        if (gXochitl->collidesWithType(&gTarget)) {
            gGameStatus = WON;
            gXochitl->deactivate();
            gTarget.deactivate();
        } else if (gXochitl->isCollidingBottom()){
            gGameStatus = LOST;
            gXochitl->deactivate();
            gTarget.deactivate();
        }

        // make sure character doesn't go off screen
        Vector2 pos = gXochitl->getPosition();
        if (pos.x < 0) pos.x = 0;
        if (pos.x > SCREEN_WIDTH) pos.x = SCREEN_WIDTH;
        if (pos.y < 0) pos.y = 0;
        if (pos.y > SCREEN_HEIGHT) pos.y = SCREEN_HEIGHT;
        gXochitl->setPosition(pos);

        deltaTime -= FIXED_TIMESTEP;
    }
}

void render()
{
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));
    
    if (gGameStatus == INPROGRESS) {
        // Text fuel left and basic instructions
        std::string fuelText = "Fuel left: " + std::to_string(fuel);
        DrawText(fuelText.c_str(), 10, 10, 20, RAYWHITE);
        DrawText("Use A/D or Left/Right to move horizontally", 10, 40, 20, RAYWHITE);
        DrawText("Use W to thrust upwards", 10, 70, 20, RAYWHITE);
        DrawText("Land on the platform safely!", 10, 100, 20, RAYWHITE);
        
        for (int i = 0; i < NUMBER_OF_TILES; i++) gTiles[i].render();
    }
    else if (gGameStatus == LOST) {
        const char* text = "Mission Failed";
        DrawText(text, 250, ORIGIN.y-35, 70, RAYWHITE);
    }
    else if (gGameStatus == WON) {
        const char* text = "Mission Accomplished";
        DrawText(text, 140, ORIGIN.y-35, 70, RAYWHITE);
    }
    gXochitl->render();
    gTarget.render();

    EndDrawing();
}

void shutdown() 
{ 
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