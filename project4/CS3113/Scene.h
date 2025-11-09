#include "Entity.h"

#ifndef SCENE_H
#define SCENE_H

struct GameState
{
    Entity *xochitl = nullptr;
    Map *map = nullptr;
    std::vector<Entity*> collidableEntities;
    std::vector<Entity*> hearts;
    std::vector<Entity*> projectiles;

    Music bgm;
    Sound jumpSound;
    Sound attackSound;
    Sound aquireSound;

    Camera2D camera;
    KeyboardKey key = KEY_F1;
    int nextSceneID;
};

class Scene 
{
protected:
    GameState mGameState;
    Vector2 mOrigin;
    const char *mBGColourHexCode = "#000000";
    

public:
    static int lives;
    Scene();
    Scene(Vector2 origin, const char *bgHexCode);

    virtual ~Scene();
    virtual void initialise() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    virtual void renderUI() = 0; // render elements that are fixed to screen
    virtual void shutdown() = 0;
    virtual void input(KeyboardKey key); 
    int getLives() { return lives; }
    
    GameState   getState()           const { return mGameState; }
    Vector2     getOrigin()          const { return mOrigin;    }
    const char* getBGColourHexCode() const { return mBGColourHexCode; }
};

#endif