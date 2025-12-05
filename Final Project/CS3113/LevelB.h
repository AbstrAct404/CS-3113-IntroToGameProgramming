#pragma once
#include "Scene.h"

// Tutorial Level B: Combat Training  
// Introduces different enemy AI types

constexpr int LEVELB_WIDTH  = 20;
constexpr int LEVELB_HEIGHT = 14;

class LevelB : public Scene {
private:
    // Tutorial map using new tileset: 0,1,17,23,33=泥土小路, 25=上下朝向泥土小路, 49=绿地, 50,51=草地, 53=横向泥土地
    unsigned int mLevelData[LEVELB_WIDTH * LEVELB_HEIGHT] = {
        49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
        49,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,49,
        49,50, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,50,49,
        49,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,49,
        49,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,49,
        49,50, 0,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53, 1,49,
        49,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,49,
        49,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,49,
        49,50, 0,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53, 1,49,
        49,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,49,
        49,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,49,
        49,50, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,50,49,
        49,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,49,
        49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
    };

public:
    static constexpr float TILE_DIMENSION = 30.0f;

    LevelB();
    LevelB(Vector2 origin, const char *bgHexCode);
    ~LevelB();
    
    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void renderUI() override;
    void shutdown() override;
    
    // Combat training
    int mEnemiesKilled = 0;
    int mTotalEnemies = 0;
    bool mTutorialComplete = false;
    int mWaveNumber = 0;
    float mWaveTimer = 0.0f;
    
    // Auto attacks
    std::vector<Entity*> mAutoAttacks;
    std::map<Direction, std::vector<int>> mProjectileAtlas;
    std::map<Direction, std::vector<int>> mFlyerProjectileAtlas;

    std::vector<Entity*> mOrbitSwords;
    std::vector<float>   mSwordAngles;
    std::vector<std::vector<Entity*>> mSwordOverlaps;

    std::vector<Entity*> mOrbitShields;
    std::vector<float>   mShieldAngles;

    std::map<Entity*, int> mArrowPierceCount;

    bool    mBowPrevAttacking = false;
    Vector2 mBowLastShotDir   = {0.0f, 0.0f};
    bool    mBowHadTargetWhenAttackStarted = false;

    std::vector<Entity*> mSwordHitList;
    bool    mSwordAttackThisFrame = false;

    void openLevelUpMenu();
    void handleLevelUpInput();
    void renderLevelUpOverlay();
    void applyUpgradeChoice(int index);

    struct LevelUpOption {
        const char* title;
        const char* description;
        int upgradeId;
    };

    static const int MAX_LEVELUP_OPTIONS = 3;

    bool mLevelUpMenuOpen = false;
    int  mLevelUpOptionCount = 0;
    int  mLevelUpSelectedIndex = -1;
    LevelUpOption mLevelUpOptions[MAX_LEVELUP_OPTIONS];

    struct WeaponUpgradeStats {
        int swordCount;
        float swordSize;
        int swordMaterialLevel;
        float shieldOrbitRadius;
        float shieldKnockback;
        int shieldCount;
        int shieldMaterialLevel;
        float bowRange;
        float bowCooldown;
        float bowFrameSpeed;
        float arrowSpeed;
        int arrowPierce;
        int bowMaterialLevel;
        float bloodBulletCD;
        int bloodBulletDamage;
        float auraRadius;
        int auraDamage;
        float playerSpeed;
        int playerMaxHP;
    };
    WeaponUpgradeStats mWeaponUpgrades;
    
    Entity *mBowEmitter = nullptr;
    bool gHasSword = false;
    bool gHasShield = false;
    bool gHasBow = false;
    bool gHasAura = false;
    
    Entity* findClosestEnemy(Vector2 pos);
    void spawnWave(int waveNum);
};
