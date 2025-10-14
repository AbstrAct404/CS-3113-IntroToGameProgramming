/**
 * Author: Aaron Zhu
 * Assignment: Pong Clone
 * Date due: 2025-10-13, 11:59pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 **/

#include "raylib.h"
#include "CS3113/cs3113.h"

// ------------------------------------------------------------
// Config
// ------------------------------------------------------------

constexpr int   SCREEN_WIDTH   = 1200,
                SCREEN_HEIGHT   = 600,
                TARGET_FPS = 120,
                MAX_BALLS  = 3;

constexpr float PADDLE_W   = 20.0f,
                PADDLE_H   = 96.0f,
                BALL_SIZE  = 18.0f,
                WALL_THICK = 8.0f,
                PADDLE_SPEED = 640.0f,
                AI_SPEED     = 640.0f;

constexpr char  BG_COLOUR[] = "#39C5BB";

AppStatus       gAppStatus = RUNNING;
float           BALL_SPEED = 360.0f,
                gPreviousTicks = 0.0f;

Texture2D       currentTexL, currentTexR;


void initialise();
void processInput(float dt);
void update();
void render();
void shutdown();

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------
struct Paddle {
    Rectangle rect;
    float     speed;
    bool      ai;
    int       aiDir;
    void moveUp(float dt)    { rect.y -= speed * dt; }
    void moveDown(float dt)  { rect.y += speed * dt; }
    void paddleControl(float minY, float maxY) {
        if (rect.y < minY) rect.y = minY;
        if (rect.y > maxY - rect.height) rect.y = maxY - rect.height;
    }
};

struct Ball {
    Rectangle rect;
    Vector2   vel;
    bool      active;
};

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
Texture2D texPaddleL1, texPaddleL2, texPaddleR1, texPaddleR2, texBall, texWall;
// change texture when hit
float paddleLTimer = 0.0f, paddleRTimer = 0.0f;
const float SWITCH_BACK_TIME = 0.3f;
Paddle leftPaddle{}, rightPaddle{};
Ball   balls[MAX_BALLS]{};
bool   gameOver = false;
int    winner   = 0; // -1 right wins, +1 left wins

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
void ResetPaddles()
{
    leftPaddle.rect  = { 32, (float)SCREEN_HEIGHT/2 - PADDLE_H/2, PADDLE_W, PADDLE_H };
    rightPaddle.rect = { (float)SCREEN_WIDTH - 32 - PADDLE_W, (float)SCREEN_HEIGHT/2 - PADDLE_H/2, PADDLE_W, PADDLE_H };
    leftPaddle.speed = PADDLE_SPEED;
    rightPaddle.speed= PADDLE_SPEED;
    rightPaddle.ai = false;
    rightPaddle.aiDir= +1;
}

void ResetBalls(int count)
{
    if (count < 1) count = 1;
    if (count > MAX_BALLS) count = MAX_BALLS;

    for (int i = 0; i < MAX_BALLS; i++)
    {
        balls[i].rect = { (float)SCREEN_WIDTH/2 - BALL_SIZE/2, (float)SCREEN_HEIGHT/2 - BALL_SIZE/2, BALL_SIZE, BALL_SIZE };
        balls[i].vel = { (i % 2 == 0 ? BALL_SPEED : -BALL_SPEED), (i == 0 ? 50.0f : (i == 1 ? -60.0f : 90.0f)) };
        balls[i].active = (i < count);
    }
}

void EndGame(int whoWon)
{
    gameOver = true;
    winner = whoWon;
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------
void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong Clone");
    SetTargetFPS(TARGET_FPS);

    texPaddleL1 = LoadTexture("assets/bocchi1.png");
    texPaddleR1 = LoadTexture("assets/kita1.png");
    texPaddleL2 = LoadTexture("assets/bocchi2.png");
    texPaddleR2 = LoadTexture("assets/kita2.png");
    texBall     = LoadTexture("assets/凉.png");
    texWall     = LoadTexture("assets/wall.png");

    currentTexL = texPaddleL1;
    currentTexR = texPaddleR1;

    ResetPaddles();
    ResetBalls(1);
    gameOver = false;
    winner = 0;
}

// ------------------------------------------------------------
// Input
// ------------------------------------------------------------
void processInput(float dt){
    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) {
        gAppStatus = TERMINATED;
        return;
    }

    if (gAppStatus == TERMINATED) EndGame(0);

    if (IsKeyPressed(KEY_ONE))   ResetBalls(1);
    if (IsKeyPressed(KEY_TWO))   ResetBalls(2);
    if (IsKeyPressed(KEY_THREE)) ResetBalls(3);

    if (!gameOver) {
        // Left paddle
        if (IsKeyDown(KEY_W)) leftPaddle.moveUp(dt);
        if (IsKeyDown(KEY_S)) leftPaddle.moveDown(dt);
        leftPaddle.paddleControl(WALL_THICK, SCREEN_HEIGHT - WALL_THICK);

        // Right paddle
        if (IsKeyDown(KEY_UP)) rightPaddle.moveUp(dt);
        if (IsKeyDown(KEY_DOWN)) rightPaddle.moveDown(dt);
        rightPaddle.paddleControl(WALL_THICK, SCREEN_HEIGHT - WALL_THICK);

        // Toggle AI
        if (IsKeyPressed(KEY_T)) {
            rightPaddle.ai = !rightPaddle.ai;
        }

        // Right paddle AI
        if (rightPaddle.ai) {
            if (rightPaddle.aiDir < 0) {
                rightPaddle.moveUp(dt * (AI_SPEED / rightPaddle.speed));
            } else {
                rightPaddle.moveDown(dt * (AI_SPEED / rightPaddle.speed));
            }

            if (rightPaddle.rect.y <= WALL_THICK) {
                rightPaddle.rect.y = WALL_THICK;
                rightPaddle.aiDir = +1;
            }
            
            if (rightPaddle.rect.y >= SCREEN_HEIGHT - WALL_THICK - rightPaddle.rect.height) {
                rightPaddle.rect.y = SCREEN_HEIGHT - WALL_THICK - rightPaddle.rect.height;
                rightPaddle.aiDir = -1;
            }
        }

    }
    else{
        if (IsKeyPressed(KEY_R)) {
            ResetPaddles();
            ResetBalls(1);
            gameOver = false;
            winner = 0;
        }
    }
}

// ------------------------------------------------------------
// update Balls
// ------------------------------------------------------------
void update()
{
    //delta time
    float ticks = (float) GetTime();
    float dt = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    if (gameOver) return;

    for (int i = 0; i < MAX_BALLS; i++)
    {
        if (!balls[i].active) continue;
        Ball& b = balls[i];

        // move
        b.rect.x += b.vel.x * dt;
        b.rect.y += b.vel.y * dt;

        // top/bottom bounce
        if (b.rect.y <= WALL_THICK) { b.rect.y = WALL_THICK; b.vel.y *= -1; }
        if (b.rect.y + b.rect.height >= SCREEN_HEIGHT - WALL_THICK) {
            b.rect.y = SCREEN_HEIGHT - WALL_THICK - b.rect.height;
            b.vel.y *= -1;
        }

        // left/right out of screen
        if (b.rect.x + b.rect.width < 0)  { EndGame(-1); return; }
        if (b.rect.x > SCREEN_WIDTH)      { EndGame(+1); return; }

        // paddle collision
        if (b.vel.x < 0) {
            if (b.rect.x < leftPaddle.rect.x + leftPaddle.rect.width &&
                b.rect.x + b.rect.width > leftPaddle.rect.x &&
                b.rect.y < leftPaddle.rect.y + leftPaddle.rect.height &&
                b.rect.y + b.rect.height > leftPaddle.rect.y) {
                b.rect.x = leftPaddle.rect.x + leftPaddle.rect.width;
                b.vel.x *= -1.1;
                b.vel.y += (leftPaddle.speed == 0 ? -0.3 : 0.3) * (IsKeyDown(KEY_W) ? -1 : 1) * leftPaddle.speed;
                currentTexL = texPaddleL2;
                paddleLTimer = SWITCH_BACK_TIME;
            }
        }
        if (b.vel.x > 0) {
            if (b.rect.x < rightPaddle.rect.x + rightPaddle.rect.width &&
                b.rect.x + b.rect.width > rightPaddle.rect.x &&
                b.rect.y < rightPaddle.rect.y + rightPaddle.rect.height &&
                b.rect.y + b.rect.height > rightPaddle.rect.y) {
                b.rect.x = rightPaddle.rect.x - b.rect.width;
                b.vel.x *= -1.1;
                b.vel.y -= (rightPaddle.speed == 0 ? -0.3 : 0.3) * (IsKeyDown(KEY_UP) ? -1 : 1) * rightPaddle.speed;
                currentTexR = texPaddleR2;
                paddleRTimer = SWITCH_BACK_TIME;
            }
        }
        if (paddleLTimer > 0) {
            paddleLTimer -= dt;
            if (paddleLTimer <= 0) currentTexL = texPaddleL1;
        }
        if (paddleRTimer > 0) {
            paddleRTimer -= dt;
            if (paddleRTimer <= 0) currentTexR = texPaddleR1;
        }

    }
}

// ------------------------------------------------------------
// Draw
// ------------------------------------------------------------
void render()
{
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));

    // Walls
    DrawRectangle(0, 0, SCREEN_WIDTH, WALL_THICK, GRAY);
    DrawRectangle(0, SCREEN_HEIGHT - WALL_THICK, SCREEN_WIDTH, WALL_THICK, GRAY);

    Rectangle srcPaddleL = { 0, 0, (float)texPaddleL1.width, (float)texPaddleL1.height };
    Rectangle srcPaddleR = { 0, 0, (float)texPaddleR1.width, (float)texPaddleR1.height };
    Rectangle srcBall    = { 0, 0, (float)texBall.width,     (float)texBall.height     };
    Rectangle dstPaddleL = { leftPaddle.rect.x,  leftPaddle.rect.y,  leftPaddle.rect.width,  leftPaddle.rect.height  };
    Rectangle dstPaddleR = { rightPaddle.rect.x, rightPaddle.rect.y, rightPaddle.rect.width, rightPaddle.rect.height };

    // Origin
    Vector2 originTL = { 0.0f, 0.0f };

    // Paddles
    DrawTexturePro(currentTexL, srcPaddleL, dstPaddleL, originTL, 0.0f, WHITE);
    DrawTexturePro(currentTexR, srcPaddleR, dstPaddleR, originTL, 0.0f, WHITE);

    // Ball
    for (int i = 0; i < MAX_BALLS; i++) {
        if (!balls[i].active) continue;
        Rectangle dstBall = { balls[i].rect.x, balls[i].rect.y, balls[i].rect.width, balls[i].rect.height };
        DrawTexturePro(texBall, srcBall, dstBall, originTL, 0.0f, WHITE);
    }

    DrawText("W/S Control Left", 16, 12, 20, WHITE);
    DrawText("UP/DOWN Control Right", 16, 36, 20, WHITE);
    DrawText("1/2/3 Number of Balls", 16, 60, 20, WHITE);
    DrawText("Press T to toggle player control", 16, 84, 20, WHITE);

    if (gameOver) {
        if (winner > 0) DrawText("Left Player Wins!", 400, 200, 40, YELLOW);
        else if (winner < 0) DrawText("Right Player Wins!", 400, 200, 40, YELLOW);
        else DrawText("Game Over", 400, 200, 40, WHITE);
        DrawText("Press R to Restart", 420, 260, 20, RAYWHITE);
    }

    EndDrawing();
}

void shutdown()
{
    CloseWindow();
    UnloadTexture(texPaddleL1);
    UnloadTexture(texPaddleL2);
    UnloadTexture(texPaddleR1);
    UnloadTexture(texPaddleR2);
    UnloadTexture(texBall);
    UnloadTexture(texWall);
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput(float (GetTime() - gPreviousTicks));
        update();
        render();
    }

    shutdown();
    return 0;
}

