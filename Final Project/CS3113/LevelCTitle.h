#pragma once
#include "Scene.h"

class LevelCTitle : public Scene {
private:
    float mTimer = 0.0f;
    const float DISPLAY_TIME = 2.0f; // 显示2秒后自动进入LevelC

public:
    LevelCTitle();
    LevelCTitle(Vector2 origin, const char *bgHexCode);
    ~LevelCTitle();
    
    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void renderUI() override;
    void shutdown() override;
};

