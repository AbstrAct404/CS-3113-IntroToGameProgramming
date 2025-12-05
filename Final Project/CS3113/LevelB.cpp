#include "LevelB.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>

LevelB::LevelB() : Scene{{0.0f}, nullptr} {}
LevelB::LevelB(Vector2 origin, const char *bgHexCode) : Scene{origin, bgHexCode} {}

LevelB::~LevelB() {}

static Vector2 getRandomSpawnPosition(Vector2 playerPos, float safeRadius, Vector2 origin)
{
   float x, y;
   Vector2 pos;

   do
   {
      x = GetRandomValue(
          origin.x - (LEVELB_WIDTH * LevelB::TILE_DIMENSION) / 4,
          origin.x + (LEVELB_WIDTH * LevelB::TILE_DIMENSION) / 4);

      y = GetRandomValue(
          origin.y - (LEVELB_HEIGHT * LevelB::TILE_DIMENSION) / 4,
          origin.y + (LEVELB_HEIGHT * LevelB::TILE_DIMENSION) / 4);

      pos = {x, y};

   } while (Vector2Distance(pos, playerPos) < safeRadius);

   return pos;
}

// Get weapon material path functions
static const char* getSwordMaterialPath(int level)
{
   switch(level)
   {
      case 1: return "assets/weapons/001.png"; // Level 1 material
      case 2: return "assets/weapons/002.png"; // Level 2 material
      case 3: return "assets/weapons/003.png"; // Level 3 material
      case 4: return "assets/weapons/004.png"; // Level 4 material
      default: return "assets/weapons/001.png";
   }
}

static const char* getShieldMaterialPath(int level)
{
   switch(level)
   {
      case 1: return "assets/weapons/037.png"; // Level 1 material
      case 2: return "assets/weapons/039.png"; // Level 2 material
      case 3: return "assets/weapons/040.png"; // Level 3 material
      default: return "assets/weapons/037.png";
   }
}

static const char* getBowMaterialPath(int level)
{
   switch(level)
   {
      case 1: return "assets/weapons/weaponSheet.png"; // Level 1 material
      case 2: return "assets/weapons/weaponSheet.png"; // Level 2 material
      case 3: return "assets/weapons/weaponSheet.png"; // Level 3 material
      default: return "assets/weapons/weaponSheet.png";
   }
}

// Update weapon material level (based on upgrade progress)
static void updateWeaponMaterialLevels(LevelB::WeaponUpgradeStats& upgrades)
{
   // Sword material upgrade: based on upgrade count
   // Lv1->Lv2: count>=2 or size>=32
   // Lv2->Lv3: count>=3 or size>=40
   // Lv3->Lv4: count>=4 or size>=48
   if (upgrades.swordMaterialLevel < 2 && (upgrades.swordCount >= 2 || upgrades.swordSize >= 32.0f))
   {
      upgrades.swordMaterialLevel = 2;
   }
   if (upgrades.swordMaterialLevel < 3 && (upgrades.swordCount >= 3 || upgrades.swordSize >= 40.0f))
   {
      upgrades.swordMaterialLevel = 3;
   }
   if (upgrades.swordMaterialLevel < 4 && (upgrades.swordCount >= 4 || upgrades.swordSize >= 48.0f))
   {
      upgrades.swordMaterialLevel = 4;
   }

   // Shield material upgrade:
   // Lv1->Lv2: count>=2 or knockback>=40
   // Lv2->Lv3: count>=3 or knockback>=60
   if (upgrades.shieldMaterialLevel < 2 && (upgrades.shieldCount >= 2 || upgrades.shieldKnockback >= 40.0f))
   {
      upgrades.shieldMaterialLevel = 2;
   }
   if (upgrades.shieldMaterialLevel < 3 && (upgrades.shieldCount >= 3 || upgrades.shieldKnockback >= 60.0f))
   {
      upgrades.shieldMaterialLevel = 3;
   }

   // Bow material upgrade:
   // Lv1->Lv2: pierce>=2 or CD<=1.5
   // Lv2->Lv3: pierce>=4 or CD<=1.0
   if (upgrades.bowMaterialLevel < 2 && (upgrades.arrowPierce >= 2 || upgrades.bowCooldown <= 1.5f))
   {
      upgrades.bowMaterialLevel = 2;
   }
   if (upgrades.bowMaterialLevel < 3 && (upgrades.arrowPierce >= 4 || upgrades.bowCooldown <= 1.0f))
   {
      upgrades.bowMaterialLevel = 3;
   }
}

// Unlock status
static bool gHasBloodBullet = true; // Available from start
static bool gHasSword = false;
static bool gHasShield = false;
static bool gHasBow = false;
static bool gHasAura = false;

// Key weapon entity pointers (for easy parameter modification during upgrades)
static Entity *gBloodEmitter = nullptr;

static Entity *mBowEmitter = nullptr;
static bool mBowWasAttackingLastFrame = false;

static Entity *mShield = nullptr;
static float mShieldAngle = 0.0f;
static float mShieldOrbitSpeed = 2.0f;
static float mSwordOrbitRadius = 30.0f;
static float mSwordOrbitSpeed = 1.5f;
static int PLAYER_PROJECTILE_DAMAGE = 1;
static int FLYER_PROJECTILE_DAMAGE = 1;

// Sound effects
static Sound gBloodBulletSound = {0};
static Sound gUpgradeSound = {0};        // Upgrade sound effect
static Sound gChooseUpgradeSound = {0};  // Choose upgrade sound effect
static Sound gPlayerDeadSound = {0};     // Player death sound

// Aura damage cooldown
static float gAuraDamageTimer = 0.0f;
static const float AURA_DAMAGE_INTERVAL = 0.1f; // Deal damage every 0.1 sec

// Experience and level system
static int gPlayerLevel = 1;
static int gPlayerExp = 0;
static int gExpToNextLevel = 10; // EXP needed for next level (increases with level)

// Enemy EXP drop values
static const int EXP_FOLLOWER = 10;
static const int EXP_WANDERER = 10;
static const int EXP_FLYER = 10;

// Set of enemies that have given EXP (to avoid duplicate EXP)
static std::set<Entity*> gExpGivenEnemies;

// Calculate EXP needed for level up
static int getExpForLevel(int level)
{
   return 10 + (level - 1) * 5; // Lv1 needs 10, Lv2 needs 15, Lv3 needs 20...
}

// Add EXP to player
static void addPlayerExp(int amount)
{
   gPlayerExp += amount;
}

// Check if can level up
static bool canLevelUp()
{
   return gPlayerExp >= gExpToNextLevel;
}

// Execute level up
static void doLevelUp()
{
   gPlayerExp -= gExpToNextLevel;
   gPlayerLevel++;
   gExpToNextLevel = getExpForLevel(gPlayerLevel);
}

// Get EXP value based on enemy type
static int getExpFromEnemy(AIType aiType)
{
   switch (aiType)
   {
      case FOLLOWER: return EXP_FOLLOWER;
      case WANDERER: return EXP_WANDERER;
      case FLYER:    return EXP_FLYER;
      default:       return 1;
   }
}

// Check if enemy just died, give EXP if so (only once)
static void checkEnemyDeathAndGiveExp(Entity* enemy)
{
   if (!enemy) return;
   if (!enemy->isDead()) return;
   if (enemy->getEntityType() != NPC) return;
   
   // Check if EXP already given
   if (gExpGivenEnemies.find(enemy) != gExpGivenEnemies.end()) return;
   
   // Mark as EXP given and add EXP
   gExpGivenEnemies.insert(enemy);
   int exp = getExpFromEnemy(enemy->getAIType());
   addPlayerExp(exp);
}

static float computeAngleDegFromDir(Vector2 dir, float offsetDeg)
{
   float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
   if (len < 0.0001f)
      return 0.0f;

   float nx = dir.x / len;
   float ny = dir.y / len;

   float angleRad = atan2f(ny, nx);
   float angleDeg = angleRad * 180.0f / PI;
   return angleDeg + offsetDeg;
}

Entity* LevelB::findClosestEnemy(Vector2 pos)
{
   Entity *closest = nullptr;
   float minDist = 400.0f;
   for (Entity *e : mGameState.collidableEntities) {
      if (!e || e->getEntityType() != NPC || e->isDead()) continue;
      float d = Vector2Distance(pos, e->getPosition());
      if (d < minDist) { minDist = d; closest = e; }
   }
   return closest;
}

void LevelB::spawnWave(int waveNum)
{
   std::map<Direction, std::vector<int>> wandererAtlas = {
      {RIGHT, {0,1,2,3,4,5,6}}, {LEFT, {0,1,2,3,4,5,6}},
      {UP, {0,1,2,3,4,5,6}}, {DOWN, {0,1,2,3,4,5,6}},
   };
   std::map<Direction, std::vector<int>> flyerAtlas = {
      {RIGHT, {0,1,2,3}}, {LEFT, {0,1,2,3}},
      {UP, {0,1,2,3}}, {DOWN, {0,1,2,3}},
   };
   std::map<Direction, std::vector<int>> followerAtlas = {
      {LEFT, {0,1,2,3,4,5,6,7,8,9}}, {RIGHT, {0,1,2,3,4,5,6,7,8,9}},
   };

   float cx = mOrigin.x;
   float cy = mOrigin.y;

   if (waveNum == 1) {
      // Wave 1: 3 Wanderers
      for (int i = 0; i < 3; i++) {
         Entity *e = new Entity({cx - 200 + i*100, cy - 150}, {40,40},
            "assets/Enemy1/Walk.png", ATLAS, {7,1}, wandererAtlas, NPC);
         e->setAIType(WANDERER);
         e->setSpeed(60);
         e->setMaxHP(50);
         e->setColliderDimensions({20,20});
         e->setOriginalPos({cx - 200 + i*100, cy - 150}); // Set original position for Wanderer AI
         mGameState.collidableEntities.push_back(e);
         mTotalEnemies++;
      }
   } else if (waveNum == 2) {
      // Wave 2: 2 Flyers
      for (int i = 0; i < 2; i++) {
         Entity *e = new Entity({cx + 150 - i*80, cy - 100}, {35,35},
            "assets/Enemy 2/Idle Enemy2.png", ATLAS, {4,1}, flyerAtlas, NPC);
         e->setAIType(FLYER);
         e->setSpeed(70);
         e->setMaxHP(40);
         e->setAttackInterval(3.0f);
         e->setColliderDimensions({17,17});
         mGameState.collidableEntities.push_back(e);
         mTotalEnemies++;
      }
   } else if (waveNum == 3) {
      // Wave 3: 3 Followers
      for (int i = 0; i < 3; i++) {
         Entity *e = new Entity({cx - 100 + i*100, cy + 150}, {45,45},
            "assets/Enemy 3/Walk.png", ATLAS, {2,1}, followerAtlas, NPC);
         e->setAIType(FOLLOWER);
         e->setAIState(FOLLOWING);
         e->setSpeed(50);
         e->setMaxHP(60);
         e->setColliderDimensions({22,22});
         mGameState.collidableEntities.push_back(e);
         mTotalEnemies++;
      }
   }
}

void LevelB::initialise()
{
   Scene::initialise();
   
   // Load and play BGM (using main.cpp's bgm)
   mGameState.bgm = LoadMusicStream("assets/bgm.mp3");
   SetMusicVolume(mGameState.bgm, 0.33f);
   PlayMusicStream(mGameState.bgm);
   mGameState.nextSceneID = -1;

   mWeaponUpgrades.swordCount = 2;
   mWeaponUpgrades.swordSize = 24.0f; // Initial sword size
   mWeaponUpgrades.swordMaterialLevel = 1; // Initial level 1 material

   mWeaponUpgrades.shieldOrbitRadius = 15.0f;
   mWeaponUpgrades.shieldKnockback = 20.0f;
   mWeaponUpgrades.shieldCount = 1;
   mWeaponUpgrades.shieldMaterialLevel = 1; // Initial level 1 material

   mWeaponUpgrades.bowRange = 180.0f;
   mWeaponUpgrades.bowCooldown = 2.0f;
   mWeaponUpgrades.bowFrameSpeed = 0.12f;
   mWeaponUpgrades.arrowSpeed = 200.0f;
   mWeaponUpgrades.arrowPierce = 1;
   mWeaponUpgrades.bowMaterialLevel = 1; // Initial level 1 material
   mWeaponUpgrades.bloodBulletCD = 0.50f;
   mWeaponUpgrades.bloodBulletDamage = 5; // Increased 5x

   mWeaponUpgrades.auraRadius = 60.0f;
   mWeaponUpgrades.auraDamage = 5; // Increased 5x

   // Player attributes initialization
   mWeaponUpgrades.playerSpeed = 1.0f; // Speed multiplier (1.0 = default speed)
   mWeaponUpgrades.playerMaxHP = 100; // Max HP (increased 5x)

   // Load sound effects
   gBloodBulletSound = LoadSound("assets/bloodBulletShoot.wav");
   SetSoundVolume(gBloodBulletSound, 0.2f);
   gUpgradeSound = LoadSound("assets/upgrade.wav");
   SetSoundVolume(gUpgradeSound, 0.5f);
   gChooseUpgradeSound = LoadSound("assets/chooseUpgrade.wav");
   SetSoundVolume(gChooseUpgradeSound, 0.5f);
   gPlayerDeadSound = LoadSound("assets/playerDead.wav");
   SetSoundVolume(gPlayerDeadSound, 0.6f);
   

   /*
      ----------- MAP -----------
   */
   // Tutorial initialization
   mEnemiesKilled = 0;
   mTotalEnemies = 0;
   mTutorialComplete = false;
   mWaveNumber = 0;
   mWaveTimer = 0.0f;

   mGameState.map = new Map(
      LEVELB_WIDTH, LEVELB_HEIGHT,  // map grid cols & rows
      (unsigned int *)mLevelData,    // grid data
      "assets/Tileset.png",          // texture filepath
      TILE_DIMENSION,                // tile size
      16, 4,                         // texture cols & rows (4x16)
      mOrigin                        // in-game origin
   );

   /*
      ----------- PROTAGONIST -----------
   */
   std::map<Direction, std::vector<int>> xochitlWalkAtlas = {
       {RIGHT, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
       {LEFT, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
       {UP, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
       {DOWN, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}},
   };

   std::vector<int> flameFrames;
   for (int i = 0; i < 49; i++)
      flameFrames.push_back(i);

   std::map<Direction, std::vector<int>> flameAtlas = {
       {UP, flameFrames},
       {DOWN, flameFrames},
       {RIGHT, flameFrames},
       {LEFT, flameFrames},
   };

   std::vector<int> projFrames;
   for (int i = 0; i < 60; i++)
      projFrames.push_back(i);

   std::map<Direction, std::vector<int>> projectileAtlas = {
       {UP, projFrames},
       {DOWN, projFrames},
       {LEFT, projFrames},
       {RIGHT, projFrames}};

   mProjectileAtlas = projectileAtlas;

   std::vector<int> flyerProjFrames;
   for (int i = 0; i < 64; ++i)
      flyerProjFrames.push_back(i);

   mFlyerProjectileAtlas = {
       {UP, flyerProjFrames},
       {DOWN, flyerProjFrames},
       {LEFT, flyerProjFrames},
       {RIGHT, flyerProjFrames}};

   /*
   std::map<EntityState, std::vector<int>> xochitlAttackAtlas = {
      {ATTACK,  {  1, 2, 3, 4, 5, 6, 7, 8, 9}},
   };
   */
   float sizeRatio = 48.0f / 64.0f;

   mGameState.xochitl = new Entity(
      {mOrigin.x - 650.0f, mOrigin.y}, // position
       {40.0f, 40.0f},                  // scale
       "assets/hero/Player_run.png",    // texture file address
       ATLAS,                           // single image or atlas?
       {1, 14},                         // atlas dimensions
       xochitlWalkAtlas,                // actual atlas
       PLAYER                           // entity type
   );

   // mGameState.xochitl->setAttackAnimations(xochitlAttackAtlas);
   mGameState.xochitl->setEntityState(WALK);
   
   // Apply player attributes
   mGameState.xochitl->setColliderDimensions({mGameState.xochitl->getScale().x / 2.5f,
                                              mGameState.xochitl->getScale().y / 1.5f});
   mGameState.xochitl->setSpeed((int)(Entity::DEFAULT_SPEED * mWeaponUpgrades.playerSpeed));
   mGameState.xochitl->setMaxHP(mWeaponUpgrades.playerMaxHP);
   mGameState.xochitl->setAcceleration({0.0f, 0.0f});

   // set attack properties

   /*
      ----------- AUTO ATTACKS -----------
   */

   /*
      ----------- FLAME AURA ----------
   */
   if (gHasAura)
   {
      Entity *flameAura = new Entity(
             {mGameState.xochitl->getPosition().x, mGameState.xochitl->getPosition().y}, // position
             {mWeaponUpgrades.auraRadius * 2.0f, mWeaponUpgrades.auraRadius * 2.0f}, // Size matches attack range (radius*2)
             "assets/Effects/17_felspell_spritesheet.png",
          ATLAS,
             {10, 10},
          flameAtlas,
          EFFECT);

      flameAura->setIsEffect(true);
      flameAura->setDirection(DOWN);
      flameAura->setAttackType(AURA);
      flameAura->setAttackRadius(mWeaponUpgrades.auraRadius);
      flameAura->setEntityState(WALK);
      flameAura->setFrameSpeed(0.1f); // Animation speed: 0.1 seconds per frame (10fps)
      flameAura->setWalkAnimations(flameAtlas); // Set walk animations
      Vector2 pos = {mGameState.xochitl->getPosition().x, mGameState.xochitl->getPosition().y - 5};
      flameAura->setPosition(pos);
      flameAura->setCheckCollision(false);
      mAutoAttacks.push_back(flameAura);
      mGameState.collidableEntities.push_back(flameAura);
   }

   /*
      ----------- PROJECTILE EMITTER -----------
   */
   if (gHasBloodBullet)
   {
      std::map<Direction, std::vector<int>> dummyEmitterAtlas = {
          {RIGHT, {0}}};

      Entity *projectileEmitter1 = new Entity(
          {mGameState.xochitl->getPosition().x, mGameState.xochitl->getPosition().y},
          {0, 0},
          "",
          SINGLE,
          {1, 1},
          dummyEmitterAtlas,
          EFFECT);

      projectileEmitter1->setAttackType(PROJECTILE);
      projectileEmitter1->setAttackInterval(mWeaponUpgrades.bloodBulletCD);
      projectileEmitter1->setCheckCollision(false);

      mAutoAttacks.push_back(projectileEmitter1);
      gBloodEmitter = projectileEmitter1;
   }

   /*
      ----------- ORBIT SWORDS -----------
   */
   if (gHasSword)
   {
      std::map<Direction, std::vector<int>> dummySwordAtlas = {
          {RIGHT, {0}}};

      int swordCount = mWeaponUpgrades.swordCount;

      for (int i = 0; i < swordCount; ++i)
      {
         Entity *sword = new Entity(
             mGameState.xochitl->getPosition(),
             {mWeaponUpgrades.swordSize, mWeaponUpgrades.swordSize},
             getSwordMaterialPath(mWeaponUpgrades.swordMaterialLevel),
             SINGLE,
             {1, 1},
             dummySwordAtlas,
             EFFECT);

         sword->setIsEffect(true);
         sword->setAttackType(MELEE);
         sword->setEntityState(WALK);
         sword->setCheckCollision(false);
         sword->setDirection(RIGHT);
         sword->setFrameSpeed(0.08f);

         sword->setAttackInterval(0.25f);

         mOrbitSwords.push_back(sword);

         float baseAngle = (2.0f * PI / swordCount) * i;
         mSwordAngles.push_back(baseAngle);
         std::vector<Entity *> emptyList;
         mSwordOverlaps.push_back(emptyList);

         mGameState.collidableEntities.push_back(sword);
         mAutoAttacks.push_back(sword);
      }
   }

   /*
      ----------- ORBIT SHIELD -----------
   */
   if (gHasShield)
   {
      std::map<Direction, std::vector<int>> shieldAtlas = {
          {RIGHT, {0}},
          {LEFT, {0}},
          {UP, {0}},
          {DOWN, {0}}};

      int shieldCount = mWeaponUpgrades.shieldCount;

      for (int i = 0; i < shieldCount; ++i)
      {
      Vector2 shieldPos = mGameState.xochitl->getPosition();
         float baseAngle = (2.0f * PI / shieldCount) * i;
         shieldPos.x += cosf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;
         shieldPos.y += sinf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;

         // Shield size based on material level: Lv1=20, Lv2=24, Lv3=28
         float shieldSize = 20.0f + (mWeaponUpgrades.shieldMaterialLevel - 1) * 4.0f;
         Entity *shield = new Entity(
          shieldPos,
             {shieldSize, shieldSize},
             getShieldMaterialPath(mWeaponUpgrades.shieldMaterialLevel),
          SINGLE,
          {1, 1},
          shieldAtlas,
          EFFECT);
         shield->setIsEffect(true);
         shield->setAttackType(SHIELD);
         shield->setEntityState(WALK);
         shield->setCheckCollision(false);

         mOrbitShields.push_back(shield);
         mShieldAngles.push_back(baseAngle);

         mAutoAttacks.push_back(shield);
         mGameState.collidableEntities.push_back(shield);
      }
   }

   /*
      ----------- BOW EMITTER -----------
   */
   if (gHasBow)
   {
      std::vector<int> bowIdleFrames;
      bowIdleFrames.push_back(104);

      std::vector<int> bowAttackFrames;
      bowAttackFrames.push_back(104);
      bowAttackFrames.push_back(106);
      bowAttackFrames.push_back(107);
      bowAttackFrames.push_back(108);
      bowAttackFrames.push_back(108);

      std::map<Direction, std::vector<int>> bowWalkAtlas = {
          {RIGHT, bowIdleFrames},
          {LEFT, bowIdleFrames},
          {UP, bowIdleFrames},
          {DOWN, bowIdleFrames}};

      std::map<EntityState, std::vector<int>> bowAttackAtlas = {
          {ATTACK, bowAttackFrames}};

      Vector2 bowPos = mGameState.xochitl->getPosition();
      bowPos.y += 2.0f;

      mBowEmitter = new Entity(
          bowPos,
          {10.0f, 10.0f},
          getBowMaterialPath(mWeaponUpgrades.bowMaterialLevel),
          ATLAS,
          {24, 5},
          bowWalkAtlas,
          EFFECT);

      Entity *target = findClosestEnemy(bowPos);
      float bowFacingOffset = 45.0f;

      if (target != nullptr && mGameState.xochitl != nullptr)
      {
         Vector2 to = target->getPosition();
         Vector2 from = mGameState.xochitl->getPosition();
         Vector2 dir = {to.x - from.x, to.y - from.y};
         float bowAngle = computeAngleDegFromDir(dir, bowFacingOffset);
         mBowEmitter->setAngle(bowAngle);
      }
      else
      {
         mBowEmitter->setAngle(0.0f);
      }

      mBowEmitter->setIsEffect(true);
      mBowEmitter->setAttackType(BOW);
      mBowEmitter->setAttackAnimations(bowAttackAtlas);
      mBowEmitter->setEntityState(WALK);
      mBowEmitter->setFrameSpeed(mWeaponUpgrades.bowFrameSpeed);
      mBowEmitter->setCheckCollision(false);
      mBowEmitter->setAttackInterval(mWeaponUpgrades.bowCooldown);

      mAutoAttacks.push_back(mBowEmitter);
      mGameState.collidableEntities.push_back(mBowEmitter);
   }

   /*
      ----------- TUTORIAL: Wave-based enemy spawning -----------
   */
   // Start with wave 1
   mWaveNumber = 1;
   spawnWave(1);

   /*
      ----------- CAMERA -----------
   */
   mGameState.camera = {0};                                      // zero initialize
   mGameState.camera.target = mGameState.xochitl->getPosition(); // camera follows player
   mGameState.camera.offset = mOrigin;                           // camera offset to center of screen
   mGameState.camera.rotation = 0.0f;                            // no rotation
   mGameState.camera.zoom = 2.5f;                                // default zoom

   mLevelUpMenuOpen = false;
   mLevelUpOptionCount = 0;
   mLevelUpSelectedIndex = -1;

   mLevelUpMenuOpen = false;
   mLevelUpOptionCount = 0;
   mLevelUpSelectedIndex = -1;
}

void LevelB::update(float deltaTime)
{
   // Check if player is dead
   if (mGameState.xochitl != nullptr && mGameState.xochitl->isDead())
   {
      PlaySound(gPlayerDeadSound); // Play death sound
      mGameState.nextSceneID = 7; // Lose Scene
      return;
   }
   
   // Update BGM (loop playback)
   UpdateMusicStream(mGameState.bgm);
   if (IsMusicStreamPlaying(mGameState.bgm) == false)
   {
      // If music finished, replay (loop implementation)
      PlayMusicStream(mGameState.bgm);
   }
   // upgrade menu open: game stalls
   if (mLevelUpMenuOpen)
   {
      handleLevelUpInput();
      return;
   }

   // Check if can level up
   if (canLevelUp())
   {
      doLevelUp();
      PlaySound(gUpgradeSound); // Play upgrade sound effect
      openLevelUpMenu();
   }

   // Clean up EXP records of deleted enemies
   for (auto it = gExpGivenEnemies.begin(); it != gExpGivenEnemies.end();)
   {
      Entity* enemy = *it;
      if (!enemy) // Check if pointer is valid
      {
         it = gExpGivenEnemies.erase(it);
         continue;
      }
      bool found = false;
      for (Entity* e : mGameState.collidableEntities)
      {
         if (e && e == enemy && e->isActive()) // Check if Entity is valid and active
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         it = gExpGivenEnemies.erase(it);
      }
      else
      {
         ++it;
      }
   }

   mGameState.xochitl->update(
       deltaTime,                    // delta time / fixed timestep
       nullptr,                      // player
       nullptr,                      // map
       mGameState.collidableEntities // col. entity count
   );

   mSwordAttackThisFrame = false;

   for (Entity *entity : mGameState.collidableEntities)
   {
      if (!entity)
         continue;
      if (entity == mGameState.xochitl)
         continue;

      entity->update(
          deltaTime,
          mGameState.xochitl, // Enemy AI uses this to track player
          mGameState.map,     // Enemy/bullet collision with map
          mGameState.collidableEntities);
   }

   // 3. Flyer ranged attack (decide whether to shoot after they move)
   for (Entity *entity : mGameState.collidableEntities)
   {
      if (!entity || !entity->isActive() || entity->isDead())
         continue;
      if (entity->getEntityType() != NPC)
         continue;
      if (entity->getAIType() != FLYER)
         continue;

      // Update attack cooldown
      entity->updateAttackCooldown(deltaTime);
      if (!entity->canTriggerAttack())
         continue;

      // Always fire at player
      Vector2 from = entity->getPosition();
      Vector2 to = mGameState.xochitl->getPosition();

      float dx = to.x - from.x;
      float dy = to.y - from.y;
      float len2 = dx * dx + dy * dy;
      if (len2 < 0.0001f)
         continue;

      float len = sqrtf(len2);
      Vector2 dir = {dx / len, dy / len};

      // -- Create Flyer's bullet (8x8 sprite sheet) --
      Entity *proj = new Entity(
          from,
          {32.0f, 32.0f}, // Texture size doubled
          "assets/Effects/9_brightfire_spritesheet.png",
          ATLAS,
          {8, 8},
          mFlyerProjectileAtlas,
          EFFECT);

      proj->setIsEffect(true);
      proj->setAttackType(PROJECTILE);
      proj->setColliderDimensions({32.0f * 0.2f, 32.0f * 0.2f}); // Collision box 0.2x size
      proj->setMovement(dir);

      float angleRad = atan2f(dir.y, dir.x);
      float angleDeg = angleRad * 180.0f / PI;
      proj->setAngle(angleDeg - 90.0f);

      if (dir.x > 0)
         proj->setDirection(RIGHT);
      else if (dir.x < 0)
         proj->setDirection(LEFT);
      else if (dir.y > 0)
         proj->setDirection(DOWN);
      else if (dir.y < 0)
         proj->setDirection(UP);

      proj->setEntityState(WALK);
      proj->setFrameSpeed(0.03f);
      proj->setSpeed(200.0f);
      proj->setColliderDimensions({32.0f * 0.6f, 32.0f * 0.6f}); // Increased collision box from 0.2 to 0.6
      proj->setCheckCollision(false);
      proj->setLifetime(3.0f);
      proj->setSpawnInvincible(0.0f);
      proj->setAIType(BULLET);
      proj->setOwner(entity);

      mGameState.collidableEntities.push_back(proj);
   }

   Vector2 currentPlayerPosition = mGameState.xochitl->getPosition();
   Vector2 pos = currentPlayerPosition;

   float minX = mOrigin.x - (LEVELB_WIDTH * TILE_DIMENSION) / 2;
   float maxX = mOrigin.x + (LEVELB_WIDTH * TILE_DIMENSION) / 2;
   float minY = mOrigin.y - (LEVELB_HEIGHT * TILE_DIMENSION) / 2;
   float maxY = mOrigin.y + (LEVELB_HEIGHT * TILE_DIMENSION) / 2;

   if (pos.x < minX)
      pos.x = minX;
   if (pos.x > maxX)
      pos.x = maxX;
   if (pos.y < minY)
      pos.y = minY;
   if (pos.y > maxY)
      pos.y = maxY;

   mGameState.xochitl->setPosition(pos);
   currentPlayerPosition = pos;

   // ---------- Player collision damage from enemies ----------
   const int MELEE_TOUCH_DAMAGE = 10; // Melee enemy touch damage (5x)
   const int FLYER_TOUCH_DAMAGE = 5;  // Flyer body touch damage (5x)

   for (Entity *enemy : mGameState.collidableEntities)
   {
      if (!enemy || !enemy->isActive() || enemy->isDead())
         continue;
      if (enemy->getEntityType() != NPC)
         continue;

      if (!enemy->overlaps(mGameState.xochitl))
         continue;

      if (enemy->getAIType() == FLYER)
      {
         // Ranged enemy body touches player: deal damage once
         mGameState.xochitl->takeDamage(FLYER_TOUCH_DAMAGE);
      }
      else
      {
         // Wanderer / Follower melee enemy touches player: deal damage once
         mGameState.xochitl->takeDamage(MELEE_TOUCH_DAMAGE);
      }
   }

   for (Entity *entity : mAutoAttacks)
   {
      if (!entity)
         continue;
      if (entity->getAttackType() == AURA)
      {
         Vector2 pos = mGameState.xochitl->getPosition();
         pos.y -= 30;
         entity->setPosition(pos);
      }
   }

   // Aura damage: deal damage every 0.1 sec
   gAuraDamageTimer += deltaTime;
   if (gAuraDamageTimer >= AURA_DAMAGE_INTERVAL)
   {
      gAuraDamageTimer = 0.0f; // Reset timer
      
      float auraRadius = mWeaponUpgrades.auraRadius;
      int auraDamage = mWeaponUpgrades.auraDamage;
   for (Entity *enemy : mGameState.collidableEntities)
   {
         if (!enemy || !enemy->isActive() || enemy->isDead())
            continue;
      if (enemy->getEntityType() != NPC)
         continue;

      float dist = Vector2Distance(
          mGameState.xochitl->getPosition(),
          enemy->getPosition());

      if (dist <= auraRadius)
      {
            enemy->takeDamage(auraDamage);
            checkEnemyDeathAndGiveExp(enemy);
            if (enemy->isDead() && enemy->getEntityType() == NPC)
            {
               mEnemiesKilled++;
            }
         }
      }
   }

   // --- Effect positioning & cooldown update ---
   for (Entity *entity : mAutoAttacks)
   {
      if (!entity)
         continue;
      
      // Aura
      if (entity->getAttackType() == AURA)
      {
         Vector2 pos = mGameState.xochitl->getPosition();
         entity->setPosition(pos);
         // Update size to match attack range
         float auraSize = mWeaponUpgrades.auraRadius * 2.0f;
         entity->setScale({auraSize, auraSize});
         continue;
      }

      // bow
      if (entity->getAttackType() == BOW)
      {
         // 1. Bow follows player position
         Vector2 playerPos = mGameState.xochitl->getPosition();
         entity->setPosition(playerPos);

         // 2. Find closest enemy to player and check if in range
         Entity *target = findClosestEnemy(playerPos);
         bool hasTargetInRange = false;
         Vector2 shotDir = {0.0f, 0.0f};

         if (target != NULL)
         {
            Vector2 to = target->getPosition();
            float dx = to.x - playerPos.x;
            float dy = to.y - playerPos.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist > 0.001f && dist <= mWeaponUpgrades.bowRange)
            {
               hasTargetInRange = true;
               shotDir.x = dx / dist;
               shotDir.y = dy / dist;

               // Make bow face enemy (visual only)
               float angleRad = atan2f(shotDir.y, shotDir.x);
               float angleDeg = angleRad * 180.0f / PI;
               entity->setAngle(angleDeg + 45.0f);
            }
         }

         // Update cooldown
         entity->updateAttackCooldown(deltaTime);

         // No suitable target, or target not in range -> don't shoot
         if (!hasTargetInRange)
         {
            continue;
         }

         // Cooldown not ready -> don't shoot
         if (!entity->canTriggerAttack())
         {
            continue;
         }

         // 3. Cooldown done + has target -> fire arrow immediately
         // Play ATTACK animation (visual only)
         entity->attack();
         entity->forceAnimationStart();

         // Arrow start position: offset from player position along shotDir
         Vector2 from = playerPos;
         from.x += shotDir.x * 16.0f;
         from.y += shotDir.y * 16.0f;

         // Create arrow entity (using SINGLE type as 105.png is single image)
         std::map<Direction, std::vector<int>> arrowAtlas = {
             {UP, {0}}, {DOWN, {0}}, {LEFT, {0}}, {RIGHT, {0}}
         };
         Entity *arrow = new Entity(
             from,
             {10, 10},
             "assets/weapons/105.png",
             ATLAS,
             {1, 1},
             arrowAtlas,
             EFFECT);

         arrow->setIsEffect(true);
         arrow->setAttackType(ARROW); // Use ARROW type to distinguish from blood bullet
         arrow->setEntityState(WALK);
         arrow->setFrameSpeed(0.03f);

         arrow->setSpeed((int)mWeaponUpgrades.arrowSpeed);
         arrow->setMovement(shotDir);

         float angleRad = atan2f(shotDir.y, shotDir.x);
         float angleDeg = angleRad * 180.0f / PI;
         if (angleRad <= PI / 2 && angleRad >= -PI / 2)
            arrow->setAngle(angleDeg + 45.0f);
         else
         {
            arrow->setAngle(angleDeg + 135.0f);
         }

         arrow->setDirection(RIGHT);
         arrow->setCheckCollision(false);
         arrow->setLifetime(3.0f);
         arrow->setSpawnInvincible(0.0f);
         arrow->setAIType(BULLET);
         arrow->setOwner(mGameState.xochitl);

         // Initialize pierce count
         mArrowPierceCount[arrow] = mWeaponUpgrades.arrowPierce;

         mGameState.collidableEntities.push_back(arrow);
         continue;
      }

      // shield orbit
      if (entity->getAttackType() == SHIELD)
      {
         int idx = -1;
         for (size_t i = 0; i < mOrbitShields.size(); ++i)
         {
            if (mOrbitShields[i] == entity)
            {
               idx = static_cast<int>(i);
               break;
            }
         }
         if (idx < 0 || idx >= (int)mShieldAngles.size())
         {
            continue;
         }

         mShieldAngles[idx] += mShieldOrbitSpeed * deltaTime;
         float ang = mShieldAngles[idx];

         Vector2 center = mGameState.xochitl->getPosition();
         Vector2 shieldPos;
         shieldPos.x = center.x + cosf(ang) * mWeaponUpgrades.shieldOrbitRadius;
         shieldPos.y = center.y + sinf(ang) * mWeaponUpgrades.shieldOrbitRadius;
         entity->setPosition(shieldPos);

         float angleRad = atan2f(shieldPos.y - center.y, shieldPos.x - center.x);
         float angleDeg = angleRad * 180.0f / PI;
         entity->setAngle(angleDeg);

         // Manual collision detection for shield vs enemy (since setCheckCollision(false))
         for (size_t i = 0; i < mGameState.collidableEntities.size(); ++i)
         {
            Entity *enemy = mGameState.collidableEntities[i];
            if (!enemy || !enemy->isActive() || enemy->isDead())
               continue;
            if (enemy->getEntityType() != NPC)
               continue;

            // Use overlaps for manual collision detection
            if (!entity->overlaps(enemy))
               continue;

            Vector2 enemyPos = enemy->getPosition();
            Vector2 knockDir;
            knockDir.x = enemyPos.x - center.x;
            knockDir.y = enemyPos.y - center.y;
            float len = sqrtf(knockDir.x * knockDir.x + knockDir.y * knockDir.y);
            if (len > 0.001f)
            {
               knockDir.x /= len;
               knockDir.y /= len;

               float d = mWeaponUpgrades.shieldKnockback * deltaTime;
               Vector2 newPos;
               newPos.x = enemyPos.x + knockDir.x * d;
               newPos.y = enemyPos.y + knockDir.y * d;
               enemy->setPosition(newPos);
            }
         }

         continue;
      }

      // sword orbit
      if (entity->getAttackType() == MELEE)
      {

         int idx = -1;
         for (size_t i = 0; i < mOrbitSwords.size(); ++i)
         {
            if (mOrbitSwords[i] == entity)
            {
               idx = static_cast<int>(i);
               break;
            }
         }
         if (idx < 0 || idx >= (int)mSwordAngles.size() || idx >= (int)mSwordOverlaps.size())
         {
            continue;
         }

         mSwordAngles[idx] += mSwordOrbitSpeed * deltaTime;

         float ang = mSwordAngles[idx];
         Vector2 center = mGameState.xochitl->getPosition();

         Vector2 swordPos;
         swordPos.x = center.x + cosf(ang) * mSwordOrbitRadius;
         swordPos.y = center.y + sinf(ang) * mSwordOrbitRadius;
         entity->setPosition(swordPos);

         float angleRad = atan2f(swordPos.y - center.y, swordPos.x - center.x);
         float angleDeg = angleRad * 180.0f / PI;
         entity->setAngle(angleDeg + 45.0f);

         std::vector<Entity *> &overlapList = mSwordOverlaps[idx];

         // Overlap radius scales with sword size (base 14.0f for size 24.0f)
         const float baseSwordSize = 24.0f;
         const float baseHitRadius = 14.0f;
         float swordHitRadius = baseHitRadius * (mWeaponUpgrades.swordSize / baseSwordSize);

         for (size_t e = 0; e < mGameState.collidableEntities.size(); ++e)
         {
            Entity *enemy = mGameState.collidableEntities[e];
            if (!enemy || !enemy->isActive() || enemy->isDead())
               continue;
            if (enemy->getEntityType() != NPC)
               continue;

            float dist = Vector2Distance(swordPos, enemy->getPosition());
            bool inRange = (dist <= swordHitRadius);

            bool wasInRange = false;
            int foundIndex = -1;
            for (size_t j = 0; j < overlapList.size(); ++j)
            {
               if (overlapList[j] == enemy)
               {
                  wasInRange = true;
                  foundIndex = (int)j;
                  break;
               }
            }

            if (inRange && !wasInRange)
            {
               enemy->takeDamage(3);
               checkEnemyDeathAndGiveExp(enemy);
               if (enemy->isDead() && enemy->getEntityType() == NPC)
               {
                  mEnemiesKilled++;
               }

               overlapList.push_back(enemy);
            }
            else if (!inRange && wasInRange && foundIndex >= 0 && foundIndex < (int)overlapList.size())
            {
               int lastIndex = (int)overlapList.size() - 1;
               if (lastIndex >= 0 && foundIndex < lastIndex)
               {
                  overlapList[foundIndex] = overlapList[lastIndex];
               }
               overlapList.pop_back();
            }
         }

         continue;
      }

      // projectile
      if (entity->getAttackType() != PROJECTILE)
         continue;

      // update emitter cooldown
      entity->updateAttackCooldown(deltaTime);

      if (!entity->canTriggerAttack())
         continue;

      // find nearest enemy to player
      Entity *target = findClosestEnemy(mGameState.xochitl->getPosition());
      if (!target)
         continue;

      // compute relative vector (no normalize): use simple dx/dy partition
      Vector2 from = mGameState.xochitl->getPosition();
      Vector2 to = target->getPosition();
      float dx = to.x - from.x;
      float dy = to.y - from.y;

      Vector2 shotDir = {0.0f, 0.0f};
      float len = sqrtf(dx * dx + dy * dy);
      if (len > 0.001f)
      {
         shotDir.x = dx / len;
         shotDir.y = dy / len;
      }

      // create bullet entity (single-sprite version to avoid atlas scoping issues)
      Entity *bullet = new Entity(
          from,
          {15, 15},
          "assets/Projectiles/BloodBullet7.png",
          ATLAS,
          {6, 10},
          mProjectileAtlas,
          EFFECT);

      bullet->setIsEffect(true);
      bullet->setAttackType(PROJECTILE);

      bullet->setMovement(shotDir);

      float angleRad = atan2f(shotDir.y, shotDir.x);
      float angleDeg = angleRad * 180.0f / PI;
      bullet->setAngle(angleDeg + 90.0f);
   

      if (shotDir.x > 0)
         bullet->setDirection(RIGHT);
      else if (shotDir.x < 0)
         bullet->setDirection(LEFT);
      else if (shotDir.y > 0)
         bullet->setDirection(DOWN);
      else if (shotDir.y < 0)
         bullet->setDirection(UP);

      bullet->setEntityState(WALK);
      bullet->setFrameSpeed(0.03f);
      bullet->setSpeed(250.0f);
      bullet->setColliderDimensions({15.0f * 0.6f, 15.0f * 0.6f}); // Set collision box, increase hit range
      bullet->setCheckCollision(false);
      bullet->setLifetime(3.0f);
      bullet->setSpawnInvincible(0.0f);
      bullet->setAIType(BULLET);
      bullet->setOwner(mGameState.xochitl);
      PlaySound(gBloodBulletSound);
      mGameState.collidableEntities.push_back(bullet);
   }

   // ------------ Unified handling of all flying projectiles (blood bullet + arrow + enemy bullets) ------------
   // Clean up pierce count of expired arrows
   for (auto it = mArrowPierceCount.begin(); it != mArrowPierceCount.end();)
   {
      Entity *arrow = it->first;
      bool found = false;
      for (Entity *e : mGameState.collidableEntities)
      {
         if (e == arrow)
         {
            found = true;
            break;
         }
      }
      if (!found || !arrow || !arrow->isActive())
      {
         it = mArrowPierceCount.erase(it);
      }
      else
      {
         ++it;
      }
   }

   const int PLAYER_PROJECTILE_DAMAGE = 5;  // Blood bullet base damage (5x)
   const int FLYER_PROJECTILE_DAMAGE = 10; // Enemy bullet damage (5x)
   const int ARROW_DAMAGE = 20;            // Arrow damage (5x)

   for (Entity *proj : mGameState.collidableEntities)
   {
      if (!proj || !proj->isActive())
         continue;

      // Only process effect type entities
      if (proj->getEntityType() != EFFECT)
         continue;

      // Only process actual "flying projectile" AI (blood bullet / arrow / Flyer bullet)
      if (proj->getAIType() != BULLET)
         continue;

      AutoAttackType atkType = proj->getAttackType();
      if (atkType != PROJECTILE && atkType != ARROW)
         continue;

      Entity *owner = proj->getOwner();
      Entity *hit = proj->getCollidedObject();

      bool ownerIsPlayer = (owner == mGameState.xochitl);
      
      // For enemy bullets, ignore collision with enemies or other enemy bullets
      if (!ownerIsPlayer && hit)
      {
         // If hit is enemy or other enemy bullet, ignore
         if (hit->getEntityType() == NPC || 
             (hit->getEntityType() == EFFECT && hit->getAIType() == BULLET))
         {
            Entity *hitOwner = hit->getOwner();
            if (!hitOwner || hitOwner != mGameState.xochitl)
            {
               hit = nullptr; // Clear collision object
            }
         }
      }

      bool hitIsPlayer = false;
      bool hitIsEnemy = false;

      if (hit && hit->isActive())
      {
         hitIsPlayer = (hit == mGameState.xochitl);
         hitIsEnemy = (hit->getEntityType() == NPC);
      }

      // Avoid bullet hitting itself (e.g. Flyer's bullet)
      if (hit == owner)
      {
         hit = nullptr;
         hitIsPlayer = false;
         hitIsEnemy = false;
      }

      // ---------- Player bullet fallback: if colliders didn't record, manually check overlap with enemies ----------
      if (ownerIsPlayer && !hitIsEnemy)
      {
         // Manually check player bullet collision with all enemies
         for (Entity *enemy : mGameState.collidableEntities)
         {
            if (!enemy || !enemy->isActive() || enemy->isDead())
               continue;
            if (enemy->getEntityType() != NPC)
               continue;
            
            // Check if bullet overlaps with enemy
            if (proj->overlaps(enemy))
            {
               hit = enemy;
               hitIsEnemy = true;
               break; // Stop after hitting first enemy
            }
         }
      }

      // ---------- Enemy bullet first check if colliding with shield (block) ----------
      if (!ownerIsPlayer && !hitIsPlayer)
      {
         bool blockedByShield = false;
         // Check if colliding with any shield
         for (Entity *shield : mOrbitShields)
         {
            if (shield && shield->isActive() && proj->overlaps(shield))
            {
               // Blocked by shield, no damage dealt
               proj->deactivate();
               blockedByShield = true;
               break;
            }
         }
         if (blockedByShield)
         {
            continue; // Blocked, skip subsequent processing
         }
         
         // If not blocked, continue checking if colliding with player
         if (proj->overlaps(mGameState.xochitl))
         {
            hit = mGameState.xochitl;
            hitIsPlayer = true;
         }
      }
      
      // Enemy bullets should not collide with enemies or other enemy bullets, only with player and shield
      // Removed enemy bullet vs enemy collision detection

      // If still no one was hit, skip
      if (!hit || !hit->isActive())
         continue;

      // Re-confirm type (hit may have just been changed by fallback above)
      hitIsPlayer = (hit == mGameState.xochitl);
      hitIsEnemy = (hit->getEntityType() == NPC);

      // ---------- Player bullets (blood bullet / arrow) hit enemy ----------
      if (ownerIsPlayer && hitIsEnemy)
      {
         int damage = PLAYER_PROJECTILE_DAMAGE;
         bool isArrow = (proj->getAttackType() == ARROW);

         if (isArrow)
         {
            damage = ARROW_DAMAGE; // Arrow deals more damage
         }
         else
         {
            damage = mWeaponUpgrades.bloodBulletDamage;
         }

         hit->takeDamage(damage);
         checkEnemyDeathAndGiveExp(hit);
         
         // Tutorial: Count killed enemies
         if (hit && hit->isActive() && hit->isDead() && hit->getEntityType() == NPC)
         {
            mEnemiesKilled++;
         }

         // If arrow, check pierce
         if (isArrow && mArrowPierceCount.find(proj) != mArrowPierceCount.end())
         {
            mArrowPierceCount[proj]--;
            if (mArrowPierceCount[proj] <= 0)
            {
               // Pierce count exhausted, destroy arrow
               mArrowPierceCount.erase(proj);
               proj->deactivate();
            }
            // Otherwise continue piercing, don't destroy
         }
         else
         {
            // Blood bullet or other, destroy on hit
            proj->deactivate();
         }
         continue;
      }

      // ---------- Enemy bullet hits player ----------
      if (!ownerIsPlayer && hitIsPlayer)
      {
         mGameState.xochitl->takeDamage(FLYER_PROJECTILE_DAMAGE);
         proj->deactivate(); // Disappear immediately after hitting player
         continue;
      }
      
      // Enemy bullets should not collide with other enemies or enemy bullets, skip directly
      if (!ownerIsPlayer && hitIsEnemy)
      {
         // Enemy bullets should not collide with enemies, ignore this case
         continue;
      }
      
      // Enemy bullet colliding with other enemy bullets should also be ignored
      if (!ownerIsPlayer && hit && hit->getEntityType() == EFFECT && hit->getAIType() == BULLET)
      {
         Entity *hitOwner = hit->getOwner();
         if (!hitOwner || hitOwner != mGameState.xochitl)
         {
            // Enemy bullets should not collide with other enemy bullets, ignore
            continue;
         }
      }

      // Ignore all other cases
   }

   // Periodically clean up dead enemies and inactive entities (every 0.5 sec)
   static float cleanupTimer = 0.0f;
   cleanupTimer += deltaTime;
   if (cleanupTimer >= 0.5f)
   {
      cleanupTimer = 0.0f;
      for (auto it = mGameState.collidableEntities.begin(); it != mGameState.collidableEntities.end();)
      {
         Entity* e = *it;
         // Clean up dead enemies or inactive entities (but keep player and weapons)
         if (e && e != mGameState.xochitl)
         {
            // Only clean up dead or inactive NPC type entities
            if (e->getEntityType() == NPC && (e->isDead() || !e->isActive()))
            {
               delete e; // Free memory
               it = mGameState.collidableEntities.erase(it);
               continue;
            }
            // Clean up inactive effect entities (bullets, arrows, etc.)
            else if (e->getEntityType() == EFFECT && !e->isActive())
            {
               delete e; // Free memory
               it = mGameState.collidableEntities.erase(it);
               continue;
            }
         }
         ++it;
      }
   }

   // TUTORIAL: Check wave completion
   if (!mTutorialComplete)
   {
      int aliveEnemies = 0;
      for (Entity *e : mGameState.collidableEntities)
      {
         if (e && e->isActive() && e->getEntityType() == NPC && !e->isDead())
         {
            aliveEnemies++;
         }
      }

      if (aliveEnemies == 0 && mWaveNumber < 3)
      {
         mWaveNumber++;
         mWaveTimer = 0;
         spawnWave(mWaveNumber);
      }
      else if (aliveEnemies == 0 && mWaveNumber >= 3)
      {
         mTutorialComplete = true;
         mWaveTimer = 0;
         mGameState.nextSceneID = 5;  // Go to LevelCTitle
      }
   }
   else
   {
      // Wait a bit before transitioning
      mWaveTimer += deltaTime;
      if (mWaveTimer > 2.0f && mGameState.nextSceneID == 2)
      {
         // Transition will happen in main.cpp
      }
   }

   float halfViewW = mGameState.camera.offset.x / mGameState.camera.zoom;
   float halfViewH = mGameState.camera.offset.y / mGameState.camera.zoom;
   Vector2 cameraTarget = currentPlayerPosition;

   if (cameraTarget.x < minX + halfViewW)
      cameraTarget.x = minX + halfViewW;
   if (cameraTarget.x > maxX - halfViewW)
      cameraTarget.x = maxX - halfViewW;
   if (cameraTarget.y < minY + halfViewH)
      cameraTarget.y = minY + halfViewH;
   if (cameraTarget.y > maxY - halfViewH)
      cameraTarget.y = maxY - halfViewH;

   panCamera(&mGameState.camera, &cameraTarget);
}

void LevelB::openLevelUpMenu()
{
   mLevelUpMenuOpen = true;
   mLevelUpSelectedIndex = -1;

   // Use a pool to temporarily store all available options
   std::vector<LevelUpOption> pool;

   // 1) Unlockable weapons that haven't been unlocked yet
   if (!gHasSword)
   {
      LevelUpOption opt;
      opt.title = "Unlock Sword";
      opt.description = "Summon orbiting swords.";
      opt.upgradeId = 0;
      pool.push_back(opt);
   }

   if (!gHasShield)
   {
      LevelUpOption opt;
      opt.title = "Unlock Shield";
      opt.description = "Summon an orbiting shield.";
      opt.upgradeId = 1;
      pool.push_back(opt);
   }

   if (!gHasAura)
   {
      LevelUpOption opt;
      opt.title = "Unlock Aura";
      opt.description = "Create a damaging flame aura.";
      opt.upgradeId = 2;
      pool.push_back(opt);
   }

   if (!gHasBow)
   {
      LevelUpOption opt;
      opt.title = "Unlock Bow";
      opt.description = "Shoot arrows at distant enemies.";
      opt.upgradeId = 3;
      pool.push_back(opt);
   }

   // 2) Weapon upgrade options (if already unlocked)
   if (gHasSword)
   {
      if (mWeaponUpgrades.swordCount < 4) // Max 4 swords (3 upgrades)
      {
         LevelUpOption opt;
         opt.title = "Sword Count+";
         opt.description = "Increase number of orbiting swords.";
         opt.upgradeId = 6;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.swordSize < 48.0f) // Max 48 (3 upgrades, +8 each)
      {
         LevelUpOption opt;
         opt.title = "Sword Size+";
         opt.description = "Increase sword size and attack range.";
         opt.upgradeId = 7;
         pool.push_back(opt);
      }
   }

   if (gHasShield)
   {
      if (mWeaponUpgrades.shieldCount < 3) // Max 3 shields (2 upgrades)
      {
         LevelUpOption opt;
         opt.title = "Shield Count+";
         opt.description = "Add another orbiting shield.";
         opt.upgradeId = 8;
         pool.push_back(opt);
      }

      // Shield knockback has limit (60.0f, 4 upgrades, +10 each)
      if (mWeaponUpgrades.shieldKnockback < 60.0f)
      {
         LevelUpOption opt;
         opt.title = "Shield Knockback+";
         opt.description = "Increase shield knockback distance.";
         opt.upgradeId = 9;
         pool.push_back(opt);
      }
   }

   if (gHasBow)
   {
      if (mWeaponUpgrades.arrowPierce < 4) // Max 4 pierce (3 upgrades)
      {
         LevelUpOption opt;
         opt.title = "Arrow Pierce+";
         opt.description = "Increase arrow penetration count.";
         opt.upgradeId = 10;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.bowCooldown > 1.0f) // Min 1.0 sec (3 upgrades)
      {
         LevelUpOption opt;
         opt.title = "Bow CD-";
         opt.description = "Reduce bow cooldown time.";
         opt.upgradeId = 11;
         pool.push_back(opt);
      }
   }

   if (gHasAura)
   {
      if (mWeaponUpgrades.auraRadius < 120.0f) // Max 120 (2 upgrades, +30 each)
      {
         LevelUpOption opt;
         opt.title = "Aura Range+";
         opt.description = "Increase aura damage radius.";
         opt.upgradeId = 12;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.auraDamage < 25) // Max 25 damage (4 upgrades, +5 each)
      {
         LevelUpOption opt;
         opt.title = "Aura DMG+";
         opt.description = "Increase aura damage.";
         opt.upgradeId = 13;
         pool.push_back(opt);
      }
   }

   // 3) Blood bullet upgrades (with limits)
   {
      if (mWeaponUpgrades.bloodBulletCD > 0.25f) // Min CD 0.25 sec (3 upgrades)
   {
      LevelUpOption opt;
      opt.title = "Blood Bullet CD-";
      opt.description = "Increase blood bullet fire rate.";
      opt.upgradeId = 4;
      pool.push_back(opt);
      }
   }

   {
      if (mWeaponUpgrades.bloodBulletDamage < 20) // Max damage 20 (3 upgrades, +5 each)
   {
      LevelUpOption opt;
      opt.title = "Blood Bullet DMG+";
      opt.description = "Increase blood bullet damage.";
      opt.upgradeId = 5;
      pool.push_back(opt);
      }
   }

   // 4) Player attribute upgrade options (only appear when weapon options insufficient)
   // Calculate current weapon-related options count (excluding player attribute upgrades)
   int weaponOptionCount = (int)pool.size();
   
   // If less than 3 weapon options, add player attribute upgrade options
   if (weaponOptionCount < 3)
   {
      {
         LevelUpOption opt;
         opt.title = "Heal";
         opt.description = "Restore 25 HP.";
         opt.upgradeId = 14;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.playerSpeed < 1.5f) // Max 1.5x speed
      {
         LevelUpOption opt;
         opt.title = "Speed+";
         opt.description = "Increase player movement speed.";
         opt.upgradeId = 15;
         pool.push_back(opt);
      }
   }

   // 3) Randomly shuffle pool (simple Fisher-Yates shuffle)
   for (int i = (int)pool.size() - 1; i > 0; --i)
   {
      int j = GetRandomValue(0, i);
      LevelUpOption temp = pool[i];
      pool[i] = pool[j];
      pool[j] = temp;
   }

   // 4) Copy at most 3 from pool to actual menu array
   mLevelUpOptionCount = 0;
   for (int i = 0; i < (int)pool.size() && i < MAX_LEVELUP_OPTIONS; ++i)
   {
      mLevelUpOptions[i] = pool[i];
      ++mLevelUpOptionCount;
   }
}

void LevelB::handleLevelUpInput()
{
   // Keyboard: 1/2/3 to directly select
   if (IsKeyPressed(KEY_ONE) && mLevelUpOptionCount >= 1)
   {
      applyUpgradeChoice(0);
      return;
   }
   if (IsKeyPressed(KEY_TWO) && mLevelUpOptionCount >= 2)
   {
      applyUpgradeChoice(1);
      return;
   }
   if (IsKeyPressed(KEY_THREE) && mLevelUpOptionCount >= 3)
   {
      applyUpgradeChoice(2);
      return;
   }

   // Mouse: move to highlight, click to select
   Vector2 mouse = GetMousePosition();
   float screenW = (float)GetScreenWidth();
   float screenH = (float)GetScreenHeight();

   float boxWidth = 400.0f;
   float boxHeight = 60.0f;
   float gap = 20.0f;

   float totalHeight = mLevelUpOptionCount * boxHeight + (mLevelUpOptionCount - 1) * gap;
   float startX = screenW * 0.5f - boxWidth * 0.5f;
   float startY = screenH * 0.5f - totalHeight * 0.5f;

   mLevelUpSelectedIndex = -1;

   for (int i = 0; i < mLevelUpOptionCount; ++i)
   {
      Rectangle rect;
      rect.x = startX;
      rect.y = startY + i * (boxHeight + gap);
      rect.width = boxWidth;
      rect.height = boxHeight;

      if (CheckCollisionPointRec(mouse, rect))
      {
         mLevelUpSelectedIndex = i;
         if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
         {
            applyUpgradeChoice(i);
            return;
         }
      }
   }
}

void LevelB::applyUpgradeChoice(int index)
{
   // Play upgrade selection sound effect
   PlaySound(gChooseUpgradeSound);
   if (index < 0 || index >= mLevelUpOptionCount || index >= MAX_LEVELUP_OPTIONS)
      return;

   LevelUpOption chosen = mLevelUpOptions[index];
   std::cout << "Chosen upgrade: " << chosen.title << std::endl;

   switch (chosen.upgradeId)
   {
   // ----------------------------------------------------
   // 1: Unlock Sword -> Create orbiting swords
   // ----------------------------------------------------
   case 0:
   {
      if (!gHasSword)
      {
         gHasSword = true;

         std::map<Direction, std::vector<int>> dummySwordAtlas = {
             {RIGHT, {0}}};

         int swordCount = mWeaponUpgrades.swordCount; // Set to 2 in initialise

         for (int i = 0; i < swordCount; ++i)
         {
            Entity *sword = new Entity(
                mGameState.xochitl->getPosition(),
                {mWeaponUpgrades.swordSize, mWeaponUpgrades.swordSize},
                "assets/weapons/001.png",
                SINGLE,
                {1, 1},
                dummySwordAtlas,
                EFFECT);

            sword->setIsEffect(true);
            sword->setAttackType(MELEE);
            sword->setEntityState(WALK);
            sword->setCheckCollision(false);
            sword->setDirection(RIGHT);
            sword->setFrameSpeed(0.08f);
            sword->setAttackInterval(0.25f);

            mOrbitSwords.push_back(sword);

            float baseAngle = (2.0f * PI / swordCount) * i;
            mSwordAngles.push_back(baseAngle);

            // KEY: Create an empty overlapList for this sword
            std::vector<Entity *> emptyList;
            mSwordOverlaps.push_back(emptyList);

            mGameState.collidableEntities.push_back(sword);
            mAutoAttacks.push_back(sword);
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 2: Unlock Shield -> Create orbiting shield
   // ----------------------------------------------------
   case 1:
   {
      if (!gHasShield)
      {
         gHasShield = true;

         std::map<Direction, std::vector<int>> shieldAtlas = {
             {RIGHT, {0}},
             {LEFT, {0}},
             {UP, {0}},
             {DOWN, {0}}};

         int shieldCount = mWeaponUpgrades.shieldCount;

         for (int i = 0; i < shieldCount; ++i)
         {
         Vector2 shieldPos = mGameState.xochitl->getPosition();
            float baseAngle = (2.0f * PI / shieldCount) * i;
            shieldPos.x += cosf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;
            shieldPos.y += sinf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;

         // Shield size based on material level: Lv1=20, Lv2=24, Lv3=28
         float shieldSize = 20.0f + (mWeaponUpgrades.shieldMaterialLevel - 1) * 4.0f;
         Entity *shield = new Entity(
             shieldPos,
             {shieldSize, shieldSize},
             getShieldMaterialPath(mWeaponUpgrades.shieldMaterialLevel),
             SINGLE,
             {1, 1},
             shieldAtlas,
             EFFECT);
            shield->setIsEffect(true);
            shield->setAttackType(SHIELD);
            shield->setEntityState(WALK);
            shield->setCheckCollision(false);

            mOrbitShields.push_back(shield);
            mShieldAngles.push_back(baseAngle);

            mAutoAttacks.push_back(shield);
            mGameState.collidableEntities.push_back(shield);
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 3: Unlock Aura -> Create flame aura
   // ----------------------------------------------------
   case 2:
   {
      if (!gHasAura)
      {
         gHasAura = true;

         // Build flameAtlas (same as in initialise)
         std::vector<int> flameFrames;
         for (int i = 0; i < 49; ++i)
            flameFrames.push_back(i);

         std::map<Direction, std::vector<int>> flameAtlas = {
             {UP, flameFrames},
             {DOWN, flameFrames},
             {RIGHT, flameFrames},
             {LEFT, flameFrames}};

         Entity *flameAura = new Entity(
             {mGameState.xochitl->getPosition().x,
              mGameState.xochitl->getPosition().y},
             {mWeaponUpgrades.auraRadius * 2.0f, mWeaponUpgrades.auraRadius * 2.0f}, // Size matches attack range
             "assets/Effects/17_felspell_spritesheet.png",
             ATLAS,
             {10, 10},
             flameAtlas,
             EFFECT);

         flameAura->setIsEffect(true);
         flameAura->setDirection(DOWN);
         flameAura->setAttackType(AURA);
         flameAura->setAttackRadius(mWeaponUpgrades.auraRadius);
         flameAura->setEntityState(WALK);
         flameAura->setFrameSpeed(0.1f); // Animation speed: 0.1 seconds per frame (10fps)
         flameAura->setWalkAnimations(flameAtlas); // Set walk animations
         flameAura->setCheckCollision(false);

         mAutoAttacks.push_back(flameAura);
         mGameState.collidableEntities.push_back(flameAura);
      }
      // Aura upgrades (radius, damage) to be done later
      break;
   }

   // ----------------------------------------------------
   // 4: Unlock Bow -> Create bow emitter
   // ----------------------------------------------------
   case 3:
   {
      if (!gHasBow)
      {
         gHasBow = true;

         std::vector<int> bowIdleFrames;
         bowIdleFrames.push_back(104);

         std::vector<int> bowAttackFrames;
         bowAttackFrames.push_back(104);
         bowAttackFrames.push_back(106);
         bowAttackFrames.push_back(107);
         bowAttackFrames.push_back(108);
         bowAttackFrames.push_back(108);

         std::map<Direction, std::vector<int>> bowWalkAtlas = {
             {RIGHT, bowIdleFrames},
             {LEFT, bowIdleFrames},
             {UP, bowIdleFrames},
             {DOWN, bowIdleFrames}};

         std::map<EntityState, std::vector<int>> bowAttackAtlas = {
             {ATTACK, bowAttackFrames}};

         Vector2 bowPos = mGameState.xochitl->getPosition();
         bowPos.y += 2.0f;

         mBowEmitter = new Entity(
             bowPos,
             {10.0f, 10.0f},
             "assets/weapons/weaponSheet.png",
             ATLAS,
             {24, 5},
             bowWalkAtlas,
             EFFECT);

         // Face closest enemy (if any)
         Entity *target = findClosestEnemy(bowPos);
         float bowFacingOffset = 45.0f;

         if (target != nullptr && mGameState.xochitl != nullptr)
         {
            Vector2 to = target->getPosition();
            Vector2 from = mGameState.xochitl->getPosition();
            Vector2 dir = {to.x - from.x, to.y - from.y};
            float bowAngle = computeAngleDegFromDir(dir, bowFacingOffset);
            mBowEmitter->setAngle(bowAngle);
         }
         else
         {
            mBowEmitter->setAngle(0.0f);
         }

         mBowEmitter->setIsEffect(true);
         mBowEmitter->setAttackType(BOW);
         mBowEmitter->setAttackAnimations(bowAttackAtlas);
         mBowEmitter->setEntityState(WALK);
         mBowEmitter->setFrameSpeed(mWeaponUpgrades.bowFrameSpeed);
         mBowEmitter->setCheckCollision(false);
         mBowEmitter->setAttackInterval(mWeaponUpgrades.bowCooldown);

         mAutoAttacks.push_back(mBowEmitter);
         mGameState.collidableEntities.push_back(mBowEmitter);
      }
      break;
   }
   case 4:
   {
      mWeaponUpgrades.bloodBulletCD *= 0.75f; // Reduce by 25% each time
      if (mWeaponUpgrades.bloodBulletCD < 0.25f)
      {
         mWeaponUpgrades.bloodBulletCD = 0.25f; // Set minimum
      }

      if (gBloodEmitter)
      {
         gBloodEmitter->setAttackInterval(mWeaponUpgrades.bloodBulletCD);
      }
      break;
   }
   case 5:
   {
      mWeaponUpgrades.bloodBulletDamage += 5; // +5 per upgrade
      if (mWeaponUpgrades.bloodBulletDamage > 20)
      {
         mWeaponUpgrades.bloodBulletDamage = 20; // Set cap
      }
      break;
   }

   // ----------------------------------------------------
   // 6: Sword Count+ -> Increase sword count
   // ----------------------------------------------------
   case 6:
   {
      if (gHasSword && mWeaponUpgrades.swordCount < 4) // Max 4 swords
      {
         mWeaponUpgrades.swordCount++;
         
         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);
         
         std::map<Direction, std::vector<int>> dummySwordAtlas = {
             {RIGHT, {0}}};

         // First clean up old swords
         for (Entity *oldSword : mOrbitSwords)
         {
            // Remove from collidableEntities and mAutoAttacks
            mGameState.collidableEntities.erase(
                std::remove(mGameState.collidableEntities.begin(), 
                           mGameState.collidableEntities.end(), oldSword),
                mGameState.collidableEntities.end());
            mAutoAttacks.erase(
                std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), oldSword),
                mAutoAttacks.end());
            delete oldSword;
         }
         mOrbitSwords.clear();
         mSwordAngles.clear();
         mSwordOverlaps.clear();

         // Create new swords, evenly distributed
         for (int i = 0; i < mWeaponUpgrades.swordCount; ++i)
         {
            Entity *sw = new Entity(
                mGameState.xochitl->getPosition(),
                {mWeaponUpgrades.swordSize, mWeaponUpgrades.swordSize},
                getSwordMaterialPath(mWeaponUpgrades.swordMaterialLevel),
                SINGLE,
                {1, 1},
                dummySwordAtlas,
                EFFECT);

            sw->setIsEffect(true);
            sw->setAttackType(MELEE);
            sw->setEntityState(WALK);
            sw->setCheckCollision(false);
            sw->setDirection(RIGHT);
            sw->setFrameSpeed(0.08f);
            sw->setAttackInterval(0.25f);

            mOrbitSwords.push_back(sw);

            float baseAngle = (2.0f * PI / mWeaponUpgrades.swordCount) * i;
            mSwordAngles.push_back(baseAngle);
            std::vector<Entity *> emptyList;
            mSwordOverlaps.push_back(emptyList);

            mGameState.collidableEntities.push_back(sw);
            mAutoAttacks.push_back(sw);
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 7: Sword Size+ -> Increase sword size
   // ----------------------------------------------------
   case 7:
   {
      if (gHasSword && mWeaponUpgrades.swordSize < 48.0f)
      {
         mWeaponUpgrades.swordSize += 8.0f;
         if (mWeaponUpgrades.swordSize > 48.0f)
         {
            mWeaponUpgrades.swordSize = 48.0f;
         }

         updateWeaponMaterialLevels(mWeaponUpgrades);

         for (Entity *sword : mOrbitSwords)
         {
            sword->setScale({mWeaponUpgrades.swordSize, mWeaponUpgrades.swordSize});
         }
      }
      break;
   }

   // ----------------------------------------------------
   // ----------------------------------------------------
   case 8:
   {
      if (gHasShield && mWeaponUpgrades.shieldCount < 3)
      {
         mWeaponUpgrades.shieldCount++;

         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);

         std::map<Direction, std::vector<int>> shieldAtlas = {
             {RIGHT, {0}},
             {LEFT, {0}},
             {UP, {0}},
             {DOWN, {0}}};

         for (Entity *shield : mOrbitShields)
         {
            // Remove from collidableEntities and mAutoAttacks
            mGameState.collidableEntities.erase(
                std::remove(mGameState.collidableEntities.begin(), 
                           mGameState.collidableEntities.end(), shield),
                mGameState.collidableEntities.end());
            mAutoAttacks.erase(
                std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), shield),
                mAutoAttacks.end());
            delete shield;
         }
         mOrbitShields.clear();
         mShieldAngles.clear();

         for (int i = 0; i < mWeaponUpgrades.shieldCount; ++i)
         {
            Vector2 shieldPos = mGameState.xochitl->getPosition();
            float baseAngle = (2.0f * PI / mWeaponUpgrades.shieldCount) * i;
            shieldPos.x += cosf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;
            shieldPos.y += sinf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;

         // Shield size based on material level: Lv1=20, Lv2=24, Lv3=28
         float shieldSize = 20.0f + (mWeaponUpgrades.shieldMaterialLevel - 1) * 4.0f;
         Entity *shield = new Entity(
             shieldPos,
             {shieldSize, shieldSize},
             getShieldMaterialPath(mWeaponUpgrades.shieldMaterialLevel),
                SINGLE,
                {1, 1},
                shieldAtlas,
                EFFECT);
            shield->setIsEffect(true);
            shield->setAttackType(SHIELD);
            shield->setEntityState(WALK);
            shield->setCheckCollision(false);

            mOrbitShields.push_back(shield);
            mShieldAngles.push_back(baseAngle);

            mAutoAttacks.push_back(shield);
            mGameState.collidableEntities.push_back(shield);
         }
      }
      break;
   }

   // ----------------------------------------------------
   // ----------------------------------------------------
   case 9:
   {
      if (gHasShield)
      {
         mWeaponUpgrades.shieldKnockback += 10.0f;
         if (mWeaponUpgrades.shieldKnockback > 60.0f)
         {
            mWeaponUpgrades.shieldKnockback = 60.0f;
         }
         
         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);
      }
      break;
   }

   // ----------------------------------------------------
   // ----------------------------------------------------
   case 10:
   {
      if (gHasBow && mWeaponUpgrades.arrowPierce < 4)
      {
         mWeaponUpgrades.arrowPierce++;
         
         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);
         
         // 如果材质等级提升，需要重新创建弓实体
         if (mBowEmitter)
         {
            // 保存当前状态
            Vector2 bowPos = mBowEmitter->getPosition();
            float bowAngle = mBowEmitter->getAngle();
            
            // 从列表中移除旧实体
            mGameState.collidableEntities.erase(
                std::remove(mGameState.collidableEntities.begin(), 
                           mGameState.collidableEntities.end(), mBowEmitter),
                mGameState.collidableEntities.end());
            mAutoAttacks.erase(
                std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), mBowEmitter),
                mAutoAttacks.end());
            delete mBowEmitter;
            
            // 重新创建弓实体（使用新材质）
            std::vector<int> bowIdleFrames;
            bowIdleFrames.push_back(104);
            std::vector<int> bowAttackFrames;
            bowAttackFrames.push_back(104);
            bowAttackFrames.push_back(106);
            bowAttackFrames.push_back(107);
            bowAttackFrames.push_back(108);
            bowAttackFrames.push_back(108);
            std::map<Direction, std::vector<int>> bowWalkAtlas = {
                {RIGHT, bowIdleFrames}, {LEFT, bowIdleFrames},
                {UP, bowIdleFrames}, {DOWN, bowIdleFrames}};
            std::map<EntityState, std::vector<int>> bowAttackAtlas = {
                {ATTACK, bowAttackFrames}};
            
            mBowEmitter = new Entity(bowPos, {10.0f, 10.0f},
                getBowMaterialPath(mWeaponUpgrades.bowMaterialLevel),
                ATLAS, {24, 5}, bowWalkAtlas, EFFECT);
            mBowEmitter->setIsEffect(true);
            mBowEmitter->setAttackType(BOW);
            mBowEmitter->setAttackAnimations(bowAttackAtlas);
            mBowEmitter->setEntityState(WALK);
            mBowEmitter->setFrameSpeed(mWeaponUpgrades.bowFrameSpeed);
            mBowEmitter->setCheckCollision(false);
            mBowEmitter->setAttackInterval(mWeaponUpgrades.bowCooldown);
            mBowEmitter->setAngle(bowAngle);
            
            mAutoAttacks.push_back(mBowEmitter);
            mGameState.collidableEntities.push_back(mBowEmitter);
         }
      }
      break;
   }

   // ----------------------------------------------------
   // ----------------------------------------------------
   case 11:
   {
      if (gHasBow && mWeaponUpgrades.bowCooldown > 1.0f)
      {
         mWeaponUpgrades.bowCooldown *= 0.75f;
         if (mWeaponUpgrades.bowCooldown < 1.0f)
         {
            mWeaponUpgrades.bowCooldown = 1.0f;
         }

         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);

         if (mBowEmitter)
         {
            mBowEmitter->setAttackInterval(mWeaponUpgrades.bowCooldown);
            
            if (mWeaponUpgrades.bowMaterialLevel > 1)
            {
               Vector2 bowPos = mBowEmitter->getPosition();
               float bowAngle = mBowEmitter->getAngle();
               
               mGameState.collidableEntities.erase(
                   std::remove(mGameState.collidableEntities.begin(), 
                              mGameState.collidableEntities.end(), mBowEmitter),
                   mGameState.collidableEntities.end());
               mAutoAttacks.erase(
                   std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), mBowEmitter),
                   mAutoAttacks.end());
               delete mBowEmitter;
               
               std::vector<int> bowIdleFrames;
               bowIdleFrames.push_back(104);
               std::vector<int> bowAttackFrames;
               bowAttackFrames.push_back(104);
               bowAttackFrames.push_back(106);
               bowAttackFrames.push_back(107);
               bowAttackFrames.push_back(108);
               bowAttackFrames.push_back(108);
               std::map<Direction, std::vector<int>> bowWalkAtlas = {
                   {RIGHT, bowIdleFrames}, {LEFT, bowIdleFrames},
                   {UP, bowIdleFrames}, {DOWN, bowIdleFrames}};
               std::map<EntityState, std::vector<int>> bowAttackAtlas = {
                   {ATTACK, bowAttackFrames}};
               
               mBowEmitter = new Entity(bowPos, {10.0f, 10.0f},
                   getBowMaterialPath(mWeaponUpgrades.bowMaterialLevel),
                   ATLAS, {24, 5}, bowWalkAtlas, EFFECT);
               mBowEmitter->setIsEffect(true);
               mBowEmitter->setAttackType(BOW);
               mBowEmitter->setAttackAnimations(bowAttackAtlas);
               mBowEmitter->setEntityState(WALK);
               mBowEmitter->setFrameSpeed(mWeaponUpgrades.bowFrameSpeed);
               mBowEmitter->setCheckCollision(false);
               mBowEmitter->setAttackInterval(mWeaponUpgrades.bowCooldown);
               mBowEmitter->setAngle(bowAngle);
               
               mAutoAttacks.push_back(mBowEmitter);
               mGameState.collidableEntities.push_back(mBowEmitter);
            }
         }
      }
      break;
   }

   // ----------------------------------------------------
   // ----------------------------------------------------
   case 12:
   {
      if (gHasAura && mWeaponUpgrades.auraRadius < 120.0f)
      {
         mWeaponUpgrades.auraRadius += 30.0f;
         if (mWeaponUpgrades.auraRadius > 120.0f)
         {
            mWeaponUpgrades.auraRadius = 120.0f;
         }
         for (Entity *entity : mAutoAttacks)
         {
            if (entity && entity->getAttackType() == AURA)
            {
               entity->setAttackRadius(mWeaponUpgrades.auraRadius);
            }
         }
      }
      break;
   }

   // ----------------------------------------------------
   // ----------------------------------------------------
   case 13:
   {
      if (gHasAura && mWeaponUpgrades.auraDamage < 25)
      {
         mWeaponUpgrades.auraDamage += 5;
         if (mWeaponUpgrades.auraDamage > 25)
         {
            mWeaponUpgrades.auraDamage = 25; // 最大伤害25
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 14: Heal → 恢复生命值
   // ----------------------------------------------------
   case 14:
   {
      if (mGameState.xochitl)
      {
         int currentHP = mGameState.xochitl->getHP();
         int maxHP = mGameState.xochitl->getMaxHP();
         int healAmount = 25; // 恢复血量（提高5倍）
         int newHP = currentHP + healAmount;
         if (newHP > maxHP)
         {
            newHP = maxHP;
         }
         mGameState.xochitl->setCurrentHP(newHP);
      }
      break;
   }

   // ----------------------------------------------------
   // 15: Speed+ → 增加玩家速度
   // ----------------------------------------------------
   case 15:
   {
      if (mWeaponUpgrades.playerSpeed < 1.5f)
      {
         mWeaponUpgrades.playerSpeed += 0.1f;
         if (mWeaponUpgrades.playerSpeed > 1.5f)
         {
            mWeaponUpgrades.playerSpeed = 1.5f; // 最大1.5倍速度
         }

         if (mGameState.xochitl)
         {
            mGameState.xochitl->setSpeed((int)(Entity::DEFAULT_SPEED * mWeaponUpgrades.playerSpeed));
         }
      }
      break;
   }

   default:
      break;
   }

   mLevelUpMenuOpen = false;
}

void LevelB::render()
{
   ClearBackground(ColorFromHex(mBGColourHexCode));

   // 1. 先渲染地图（最底层）
   mGameState.map->render();

   // 2. 渲染Aura（在玩家下面），稍微调暗
   for (Entity *entity : mAutoAttacks)
   {
      if (entity && entity->getAttackType() == AURA && entity->isActive())
      {
         // 稍微调暗的颜色（不要太暗）
         Color darkTint = {200, 200, 200, 200};
         entity->renderWithTint(darkTint);
      }
   }

   // 3. 渲染玩家
   mGameState.xochitl->render();
   
   // 4. 最后渲染其他实体（敌人、武器等，在最上层）
   for (int i = 0; i < mGameState.collidableEntities.size(); ++i)
   {
      if (mGameState.collidableEntities[i])
      {
         Entity* entity = mGameState.collidableEntities[i];
         // 跳过aura效果实体，因为已经渲染过了
         // 只有EFFECT类型且attackType是AURA的才跳过
         bool isAuraEffect = (entity->getEntityType() == EFFECT && entity->getAttackType() == AURA);
         if (!isAuraEffect)
         {
            entity->render();
         }
      }
   }
}

void LevelB::renderUI()
{
   Entity *player = mGameState.xochitl;

   // Tutorial title
   const char* title = "TUTORIAL 2: COMBAT TRAINING";
   int titleWidth = MeasureText(title, 28);
   DrawText(title, GetScreenWidth()/2 - titleWidth/2, 30, 28, YELLOW);

   // Wave info
   const char* waveTexts[] = {
      "Wave 1: WANDERERS - They move randomly around the map.",
      "Wave 2: FLYERS - They shoot projectiles at you! Dodge them!",
      "Wave 3: FOLLOWERS - They chase you relentlessly!"
   };
   
   if (mWaveNumber >= 1 && mWaveNumber <= 3)
   {
      DrawText(waveTexts[mWaveNumber-1], 50, 80, 18, SKYBLUE);
   }

   // 血量条
   float hpPercent = (float)player->getHP() / player->getMaxHP();
   DrawRectangle(20, 120, 200, 20, DARKGRAY);
   DrawRectangle(20, 120, (int)(200 * hpPercent), 20, RED);
   DrawText(TextFormat("HP: %d / %d",
                       player->getHP(),
                       player->getMaxHP()),
            20, 145, 16, WHITE);

   // 经验条
   float expPercent = (float)gPlayerExp / gExpToNextLevel;
   if (expPercent > 1.0f) expPercent = 1.0f;
   DrawRectangle(20, 170, 200, 12, DARKGRAY);
   DrawRectangle(20, 170, (int)(200 * expPercent), 12, SKYBLUE);
   DrawText(TextFormat("Lv.%d  EXP: %d / %d",
                       gPlayerLevel,
                       gPlayerExp,
                       gExpToNextLevel),
            20, 187, 14, WHITE);

   // Progress
   DrawText(TextFormat("Wave: %d / 3", mWaveNumber), 50, 210, 20, GREEN);
   DrawText(TextFormat("Enemies Killed: %d", mEnemiesKilled), 50, 235, 18, WHITE);

   DrawText("Move: W A S D | Dodge enemy attacks!", 50, GetScreenHeight()-50, 16, LIGHTGRAY);

   if (mLevelUpMenuOpen)
   {
      renderLevelUpOverlay();
   }

   if (mTutorialComplete)
   {
      const char* complete = "COMBAT TRAINING COMPLETE! Entering main level...";
      int w = MeasureText(complete, 30);
      DrawText(complete, GetScreenWidth()/2 - w/2, GetScreenHeight()/2, 30, GREEN);
   }
}

void LevelB::renderLevelUpOverlay()
{
   float screenW = (float)GetScreenWidth();
   float screenH = (float)GetScreenHeight();

   // 半透明遮罩（让外围画面变暗）
   DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, 0.7f));

   float boxWidth = 420.0f;
   float boxHeight = 70.0f;
   float gap = 20.0f;

   float totalHeight = mLevelUpOptionCount * boxHeight + (mLevelUpOptionCount - 1) * gap;
   float startX = screenW * 0.5f - boxWidth * 0.5f;
   float startY = screenH * 0.5f - totalHeight * 0.5f;

   const char *titleText = "LEVEL UP!";
   int titleFontSize = 32;
   int titleWidth = MeasureText(titleText, titleFontSize);
   DrawText(
       titleText,
       (int)(screenW * 0.5f - titleWidth * 0.5f),
       (int)(startY - 70.0f),
       titleFontSize,
       YELLOW);

   const char *hintText = "Choose one upgrade (1/2/3 or click)";
   int hintFontSize = 18;
   int hintWidth = MeasureText(hintText, hintFontSize);
   DrawText(
       hintText,
       (int)(screenW * 0.5f - hintWidth * 0.5f),
       (int)(startY - 40.0f),
       hintFontSize,
       RAYWHITE);

   for (int i = 0; i < mLevelUpOptionCount; ++i)
   {
      float y = startY + i * (boxHeight + gap);

      Rectangle rect;
      rect.x = startX;
      rect.y = y;
      rect.width = boxWidth;
      rect.height = boxHeight;

      Color boxColor = DARKGRAY;
      if (i == mLevelUpSelectedIndex)
      {
         boxColor = GRAY;
      }

      DrawRectangleRec(rect, boxColor);
      DrawRectangleLinesEx(rect, 2.0f, WHITE);

      const char *name = mLevelUpOptions[i].title;
      const char *desc = mLevelUpOptions[i].description;

      DrawText(
          TextFormat("%d. %s", i + 1, name),
          (int)(rect.x + 10.0f),
          (int)(rect.y + 8.0f),
          20,
          WHITE);

      DrawText(
          desc,
          (int)(rect.x + 10.0f),
          (int)(rect.y + 36.0f),
          16,
          LIGHTGRAY);
   }
}

void LevelB::shutdown()
{
   // Unload BGM
   if (mGameState.bgm.frameCount > 0)
   {
      StopMusicStream(mGameState.bgm);
      UnloadMusicStream(mGameState.bgm);
   }
   
   // Unload sound effects
   if (gBloodBulletSound.frameCount > 0)
   {
      UnloadSound(gBloodBulletSound);
   }
   if (gUpgradeSound.frameCount > 0)
   {
      UnloadSound(gUpgradeSound);
   }
   if (gChooseUpgradeSound.frameCount > 0)
   {
      UnloadSound(gChooseUpgradeSound);
   }
   if (gPlayerDeadSound.frameCount > 0)
   {
      UnloadSound(gPlayerDeadSound);
   }
   
   // printf("LevelB::shutdown()");
   Scene::shutdown();
}