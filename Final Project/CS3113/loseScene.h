#include "Scene.h"

class LoseScene : public Scene {

public:
    static constexpr float TILE_DIMENSION       = 55.0f,
                        ACCELERATION_OF_GRAVITY = 981.0f,
                        END_GAME_THRESHOLD      = 800.0f;

    LoseScene();
    LoseScene(Vector2 origin, const char *bgHexCode);
    ~LoseScene();
    
    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void renderUI() override;
    void shutdown() override;
};