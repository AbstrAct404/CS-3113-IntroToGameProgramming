#pragma once
#include "Scene.h"

// Main Level C: 2-minute Survival
// Full game with upgrade system, dynamic difficulty, and enemy waves

// Kill counters (defined in LevelC.cpp, accessible from other scenes)
extern int gTotalKills;
extern int gFollowerKills;
extern int gWandererKills;
extern int gFlyerKills;

constexpr int LEVELC_WIDTH  = 36;
constexpr int LEVELC_HEIGHT = 20;

class LevelC : public Scene {
private:
    // Full game map using new tileset: 0,1,17,23,33=dirt path, 25=vertical dirt path, 49=green land, 50,51=grass, 53=horizontal dirt
    unsigned int mLevelData[LEVELC_WIDTH * LEVELC_HEIGHT] = {
        49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
        49,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,49,
        49,50,50, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,33,50,50,50,49,
        49,50,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,50,50,50,49,
        49,50,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,50,50,50,49,
        49,50,50, 0,53,53,53,53,53,53,53,53,53,53,53,33,50,50, 0,53,53,53,53,53,53,53,53,53,53,53,53,33,50,50,50,49,
        49,50,50,50,50,50,50,50,50,50,50,50,50,50,50,25,50,50,25,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,49,
        49,50,50,50,50,50,50,50,50,50,50,50,50,50,50,25,50,50,25,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,49,
        49,50, 0,53,53,53,53,53,53,53,53,53,53,53,53,33,50,50, 0,53,53,53,53,53,53,53,53,53,53,53,53,53,53,33,50,49,
        49,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,50,50,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,50,49,
        49,50,25,51,51,51,51,51,51,51,51,51,51,51,51,51,50,50,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,25,50,49,
        49,50, 0,53,53,53,53,53,53,53,53,33,50,50, 0,53,53,53,53,53,53,53,33,50,50, 0,53,53,53,53,53,53,53,33,50,49,
        49,50,50,50,50,50,50,50,50,50,50,25,50,50,25,50,50,50,50,50,50,50,25,50,50,25,50,50,50,50,50,50,50,50,50,49,
        49,50,50,50,50,50,50,50,50,50,50,25,50,50,25,50,50,50,50,50,50,50,25,50,50,25,50,50,50,50,50,50,50,50,50,49,
        49,50,50, 0,53,53,53,53,53,53,53,33,50,50, 0,53,53,53,53,53,53,53,33,50,50, 0,53,53,53,53,53,33,50,50,50,49,
        49,50,50,25,51,51,51,51,51,51,51,51,50,50,51,51,51,51,51,51,51,51,51,51,51,25,50,50,50,50,50,25,50,50,50,49,
        49,50,50,25,51,51,51,51,51,51,51,51,50,50,51,51,51,51,51,51,51,51,51,51,51,25,50,50,50,50,50,25,50,50,50,49,
        49,50,50, 0,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,53,33,50,50,50,50,50, 0,53,33,50,49,
        49,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,49,
        49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
    };

public:
    static constexpr float TILE_DIMENSION = 25.0f;
    static constexpr float SURVIVAL_TIME = 120.0f;  // 2 minutes
    static constexpr int MAX_FLYERS = 5;

    LevelC();
    LevelC(Vector2 origin, const char *bgHexCode);
    ~LevelC();
    
    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void renderUI() override;
    void shutdown() override;
    
    // Game systems
    Entity* findClosestEnemy(Vector2 pos);
    std::vector<Entity*> findClosestEnemies(Vector2 pos, int count); // For Heaven Laser targeting
    void openLevelUpMenu();
    void handleLevelUpInput();
    void renderLevelUpOverlay();
    void applyUpgradeChoice(int index);
    void spawnEnemy(int type, float difficulty = 1.0f);  // 0=wanderer, 1=flyer, 2=follower
    void spawnWave(int followerCount);
    int countActiveFlyers();
    int getTotalWeaponLevel();
    
    // Timer
    float mGameTimer = 0.0f;
    bool mGameWon = false;
    bool mGameOver = false;
    
    // Spawning
    float mSpawnTimer = 0.0f;
    float mBaseSpawnInterval = 2.0f;
    bool mWave60Spawned = false;
    bool mWave90Spawned = false;
    
    // Experience system
    int mPlayerLevel = 1;
    int mPlayerExp = 0;
    int mExpToNextLevel = 10;
    
    // Upgrade system
    struct WeaponUpgradeStats {
        // Sword
        int swordCount;
        float swordSize;
        int swordMaterialLevel;
        int swordUpgradeCount; // Track upgrade count
        // Shield
        float shieldOrbitRadius;
        float shieldKnockback;
        int shieldCount;
        int shieldMaterialLevel;
        int shieldUpgradeCount; // Track upgrade count
        // Bow / Arrow
        float bowRange;
        float bowCooldown;
        float bowFrameSpeed;
        float arrowSpeed;
        int arrowPierce;
        int bowMaterialLevel;
        int bowUpgradeCount; // Track upgrade count
        // blood bullet
        float bloodBulletCD;
        int bloodBulletDamage;
        int bloodBulletUpgradeCount; // Track upgrade count
        // Aura
        float auraRadius;
        int auraDamage;
        int auraUpgradeCount; // Track upgrade count
        // Player stats
        float playerSpeed;
        int playerMaxHP;
    };
    WeaponUpgradeStats mWeaponUpgrades;
    
    bool mHasSword = false;
    bool mHasShield = false;
    bool mHasBow = false;
    bool mHasAura = false;
    
    // Level up menu
    struct LevelUpOption {
        const char* title;
        const char* description;
        int upgradeId;
    };
    static const int MAX_LEVELUP_OPTIONS = 3;
    bool mLevelUpMenuOpen = false;
    int mLevelUpOptionCount = 0;
    int mLevelUpSelectedIndex = -1;
    LevelUpOption mLevelUpOptions[MAX_LEVELUP_OPTIONS];
    
    // Auto attacks
    std::vector<Entity*> mAutoAttacks;
    std::map<Direction, std::vector<int>> mProjectileAtlas;
    std::map<Direction, std::vector<int>> mFlyerProjectileAtlas;
    
    // Sword orbiting
    std::vector<Entity*> mOrbitSwords;
    std::vector<float> mSwordAngles;
    std::vector<std::vector<Entity*>> mSwordOverlaps;
    std::vector<float> mSwordAttackTimers; // Attack timer for each sword
    bool mSwordAttackThisFrame = false;
    
    // Shield orbiting
    std::vector<Entity*> mOrbitShields;
    std::vector<float> mShieldAngles;
    
    // Arrow pierce tracking
    std::map<Entity*, int> mArrowPierceCount;
    
    // Aura damage timer
    float mAuraDamageTimer = 0.0f;
};
