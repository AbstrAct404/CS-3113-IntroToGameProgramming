#pragma once
#include "Scene.h"

class LevelATitle : public Scene {
private:
    float mTimer = 0.0f;
    const float DISPLAY_TIME = 2.0f; // 显示2秒后自动进入LevelA

public:
    LevelATitle();
    LevelATitle(Vector2 origin, const char *bgHexCode);
    ~LevelATitle();
    
    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void renderUI() override;
    void shutdown() override;
};

