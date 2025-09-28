/**
* Author: [Aaron Zhu]
* Assignment: Simple 2D Scene
* Date due: 2025-09-27, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "raylib.h"
#include <array>
#include <string>
#include <math.h>

enum AppStatus { TERMINATED, RUNNING };

struct Sprite {
    Texture2D texture;
    Vector2 position;
    Vector2 scale;
    float angle;
    float pulseTime;
};

// Declare arrays to hold textures and sprites
Sprite members[6];
Texture2D textures[6];

// Global Constants
constexpr int   SCREEN_WIDTH  = 1600,
                SCREEN_HEIGHT = 900,
                FPS           = 60,
                SIZE          = 100,
                FRAME_LIMIT   = 100;
constexpr float MAX_AMP       = 10.0f;

constexpr char    BG_COLOUR[] = "39C5BB";
constexpr Vector2 ORIGIN      = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
constexpr Vector2 BASE_SIZE   = { (float) SIZE, (float) SIZE };

constexpr char GBC[]  = "assets/gbc.jpeg";
constexpr char NINA[] = "assets/nina.jpeg";
constexpr char MOMOKA[] = "assets/momoka.jpeg";
constexpr char SUBARU[]  = "assets/subaru.jpeg";
constexpr char RUPA[]    = "assets/rupa.jpeg";
constexpr char TOMO[]  = "assets/tomo.jpeg";

// Global Variables
AppStatus gAppStatus     = RUNNING;
float     gScaleFactor   = SIZE;
Vector2   gPosition      = ORIGIN;
Vector2   gScale         = BASE_SIZE;
float     gPreviousTicks = 0.0f;

// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();

// Function Definitions
void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Textures");

    textures[0] = LoadTexture(GBC);
    textures[1] = LoadTexture(NINA);
    textures[2] = LoadTexture(MOMOKA);
    textures[3] = LoadTexture(SUBARU);
    textures[4] = LoadTexture(RUPA);
    textures[5] = LoadTexture(TOMO);

    SetTargetFPS(FPS);

    // Initialize sprite properties
    for (int i = 0; i < 6; i++) {
        members[i].texture   = textures[i];
        members[i].position  = ORIGIN;
        members[i].scale     = {
            BASE_SIZE.x / 2,
            BASE_SIZE.y / 2
        };
        members[i].angle     = 0.0f;
        members[i].pulseTime = 0.0f;
    }
}

void processInput() 
{
    if (WindowShouldClose()) gAppStatus = TERMINATED;
}

void update() 
{
    // Delta time
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks = ticks;

    //GBC's Heartbeat
    members[0].pulseTime += deltaTime;

    members[0].scale = {
        1.5f * BASE_SIZE.x + MAX_AMP * 2 * cos(members[0].pulseTime),
        1.5f * BASE_SIZE.y + MAX_AMP * 2 * cos(members[0].pulseTime)
    };

    // NINA's Rotation
    members[1].pulseTime += deltaTime;
    members[1].angle += 60.0f * deltaTime;
    if (members[1].angle > 360.0f) members[1].angle -= 360.0f;
    members[1].position = {
        ORIGIN.x + 200.0f * cos(members[1].pulseTime),
        ORIGIN.y + 200.0f * sin(members[1].pulseTime)
    };

    // MOMOKA's movements based on NINA's rotation
    members[2].pulseTime += deltaTime;
    members[2].position = {
        members[1].position.x + 350.0f * cos(members[2].pulseTime),
        members[1].position.y - 100.0f + 250.0f * sin(2 * members[2].pulseTime)
    };

    //SUBARU's scaling and self-rotation
    members[3].position = {
        ORIGIN.x,
        ORIGIN.y - 250.0f
    };
    members[3].pulseTime += deltaTime;
    members[3].angle += 90.0f * deltaTime;
    if (members[3].angle > 360.0f) members[3].angle -= 360.0f;
    members[3].scale = {
        BASE_SIZE.x + MAX_AMP * sin(members[3].pulseTime),
        BASE_SIZE.y + MAX_AMP * sin(members[3].pulseTime)
    }; 

    //RUPA's movements
    members[4].pulseTime += deltaTime;
    members[4].position = {
        ORIGIN.x + 200.0f * sin(members[4].pulseTime / 2),
        ORIGIN.y + 220.0f * cos(members[4].pulseTime / 2)
    };

    //TOMO's movements based on SUBARU's movements
    members[5].pulseTime += deltaTime;
    members[5].position = {
        members[4].position.x + 200 * sin(members[5].pulseTime * 2),
        members[4].position.y - 200 * (cos(members[5].pulseTime * 2) * cos(members[5].pulseTime / 2))
    };
}

void render()
{
    BeginDrawing();
    // Change background color over time 
    // (searched online for how to code rgb seperately, did not use llms)
    float t = GetTime(); 
    unsigned char r = (unsigned char)(127 + 86 * sin(t));
    unsigned char g = (unsigned char)(127 + 86 * sin(t + 2.0f));
    unsigned char b = (unsigned char)(127 + 86 * sin(t + 1.0f));

    Color bgColor = { r, g, b, 255 };

    ClearBackground(bgColor);

    // Draw all members
    for (int i=0; i<6; i++) {
        Rectangle src = {0, 0, (float)members[i].texture.width, (float)members[i].texture.height};
        Rectangle dst = {members[i].position.x, members[i].position.y, members[i].scale.x, members[i].scale.y};
        Vector2 origin = {members[i].scale.x / 2, members[i].scale.y / 2};

        DrawTexturePro(
            members[i].texture,
            src, 
            dst, 
            origin, 
            members[i].angle, 
            WHITE
        );
    }

    EndDrawing();
}

void shutdown() { 
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
