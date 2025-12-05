#include "LevelC.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>

LevelC::LevelC() : Scene{{0.0f}, nullptr} {}
LevelC::LevelC(Vector2 origin, const char *bgHexCode) : Scene{origin, bgHexCode} {}

LevelC::~LevelC() {}

static Vector2 getRandomSpawnPosition(Vector2 playerPos, float safeRadius, Vector2 origin)
{
   float x, y;
   Vector2 pos;

   do
   {
      x = GetRandomValue(
          origin.x - (LEVELC_WIDTH * LevelC::TILE_DIMENSION) / 4,
          origin.x + (LEVELC_WIDTH * LevelC::TILE_DIMENSION) / 4);

      y = GetRandomValue(
          origin.y - (LEVELC_HEIGHT * LevelC::TILE_DIMENSION) / 4,
          origin.y + (LEVELC_HEIGHT * LevelC::TILE_DIMENSION) / 4);

      pos = {x, y};

   } while (Vector2Distance(pos, playerPos) < safeRadius);

   return pos;
}

// Function to get weapon material path
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

// Update weapon material level based on upgrade progress
// If weapon already has level system, use existing system; otherwise, use upgrade count: every 2 upgrades = +1 level
static void updateWeaponMaterialLevels(LevelC::WeaponUpgradeStats& upgrades)
{
   // Sword material upgrade: if existing level system (based on count or size), use existing system
   // Otherwise use upgrade count: every 2 upgrades = +1 level
   if (upgrades.swordMaterialLevel < 2)
   {
      // Existing level system: count >= 2 or size >= 32
      if (upgrades.swordCount >= 2 || upgrades.swordSize >= 32.0f)
      {
         upgrades.swordMaterialLevel = 2;
      }
      // New system: every 2 upgrades = +1 level
      else if (upgrades.swordUpgradeCount >= 2)
      {
         upgrades.swordMaterialLevel = 2;
      }
   }
   if (upgrades.swordMaterialLevel < 3)
   {
      if (upgrades.swordCount >= 3 || upgrades.swordSize >= 40.0f)
      {
         upgrades.swordMaterialLevel = 3;
      }
      else if (upgrades.swordUpgradeCount >= 4) // Need 4 upgrades (2 to reach level 2, then 2 more to reach level 3)
      {
         upgrades.swordMaterialLevel = 3;
      }
   }
   if (upgrades.swordMaterialLevel < 4)
   {
      if (upgrades.swordCount >= 4 || upgrades.swordSize >= 48.0f)
      {
         upgrades.swordMaterialLevel = 4;
      }
      else if (upgrades.swordUpgradeCount >= 6) // Need 6 upgrades
      {
         upgrades.swordMaterialLevel = 4;
      }
   }

   // Shield material upgrade: same logic
   int oldShieldLevel = upgrades.shieldMaterialLevel;
   if (upgrades.shieldMaterialLevel < 2)
   {
      if (upgrades.shieldCount >= 2 || upgrades.shieldKnockback >= 40.0f)
      {
         upgrades.shieldMaterialLevel = 2;
      }
      else if (upgrades.shieldUpgradeCount >= 2)
      {
         upgrades.shieldMaterialLevel = 2;
      }
   }
   if (upgrades.shieldMaterialLevel < 3)
   {
      if (upgrades.shieldCount >= 3 || upgrades.shieldKnockback >= 40.0f)
      {
         upgrades.shieldMaterialLevel = 3;
      }
      else if (upgrades.shieldUpgradeCount >= 4)
      {
         upgrades.shieldMaterialLevel = 3;
      }
   }
   
   // When material level increases, increase orbit radius (level 2: +5, level 3: +5 more)
   if (upgrades.shieldMaterialLevel > oldShieldLevel)
   {
      if (upgrades.shieldMaterialLevel == 2)
      {
         upgrades.shieldOrbitRadius += 5.0f; // Increase radius at level 2
      }
      else if (upgrades.shieldMaterialLevel == 3)
      {
         upgrades.shieldOrbitRadius += 5.0f; // Increase radius again at level 3
      }
   }

   // Bow material upgrade: same logic
   if (upgrades.bowMaterialLevel < 2)
   {
      if (upgrades.arrowPierce >= 2 || upgrades.bowCooldown <= 1.5f)
      {
         upgrades.bowMaterialLevel = 2;
      }
      else if (upgrades.bowUpgradeCount >= 2)
      {
         upgrades.bowMaterialLevel = 2;
      }
   }
   if (upgrades.bowMaterialLevel < 3)
   {
      if (upgrades.arrowPierce >= 4 || upgrades.bowCooldown <= 1.0f)
      {
         upgrades.bowMaterialLevel = 3;
      }
      else if (upgrades.bowUpgradeCount >= 4)
      {
         upgrades.bowMaterialLevel = 3;
      }
   }
}

// mWeaponUpgrades is a member variable in LevelC class

// Unlock status
static bool gHasBloodBullet = true; // Available from start
static bool gHasSword = false;
static bool gHasShield = false;
static bool gHasBow = false;
static bool gHasAura = false;
static bool gHasHeavenLaser = false; // Ultimate weapon - unlocked when blood bullet AND bow both >= level 3

// Key weapon entity pointers (for easy parameter modification during upgrades)
static Entity *gBloodEmitter = nullptr;

static Entity *mBowEmitter = nullptr;
static bool mBowWasAttackingLastFrame = false;

// Heaven Laser (Ultimate weapon) variables
static float gHeavenLaserCD = 1.0f; // Fire every 1.0 seconds
static float gHeavenLaserTimer = 0.0f;
static const int HEAVEN_LASER_DAMAGE = 1000; // One-shot kill
static const float HEAVEN_LASER_LENGTH = 400.0f; // Beam length
static const float HEAVEN_LASER_WIDTH = 32.0f; // Beam width

// Track active laser beams with their animation state
struct LaserBeam {
   Entity* entity;
   Vector2 direction;
   int lastDamageFrame; // Track which frame we last dealt damage on
   std::set<Entity*> hitEnemies; // Track enemies already hit by this beam
};
static std::vector<LaserBeam> gActiveLaserBeams;

// Cheat mode - Press L to open upgrade menu with Heaven Laser always available
static bool gCheatModeActive = false;

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
static Sound gHeavenLaserSound = {0};    // Heaven Laser firing sound
static Sound gPlayerDeadSound = {0};     // Player death sound
static Sound gPlayerHurtSound = {0};     // Player hurt sound

// Aura damage cooldown
static float gAuraDamageTimer = 0.0f;
static const float AURA_DAMAGE_INTERVAL = 0.1f; // Deal damage every 0.1 seconds

// Experience and level system
static int gPlayerLevel = 1;
static int gPlayerExp = 0;
static int gExpToNextLevel = 10; // Experience required for next level (increases with level)

// Enemy experience drop values (increased base experience)
static const int EXP_FOLLOWER = 4; // Increased from 3 to 4
static const int EXP_WANDERER = 3; // Increased from 2 to 3
static const int EXP_FLYER = 3;    // Increased from 2 to 3

// Set of enemies that have already given experience (to avoid duplicate experience)
static std::set<Entity*> gExpGivenEnemies;

// Kill counters (extern so other scenes can access)
int gTotalKills = 0;
int gFollowerKills = 0;
int gWandererKills = 0;
int gFlyerKills = 0;

// Calculate experience required for next level (exponential growth, multiply by 1.2 per level)
static int getExpForLevel(int level)
{
   // Level 1 needs 10, then multiply by 1.2 per level
   float baseExp = 10.0f;
   for (int i = 1; i < level; i++)
   {
      baseExp *= 1.2f; // Changed from 1.1x to 1.2x
   }
   return (int)baseExp;
}

// Give experience to player
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

// Get experience value based on enemy type
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

// Check if enemy just died, if so give experience (only once)
static void checkEnemyDeathAndGiveExp(Entity* enemy)
{
   if (!enemy) return;
   if (!enemy->isDead()) return;
   if (enemy->getEntityType() != NPC) return;
   
   // Check if experience has already been given
   if (gExpGivenEnemies.find(enemy) != gExpGivenEnemies.end()) return;
   
   // Mark as experience given and add experience
   gExpGivenEnemies.insert(enemy);
   int exp = getExpFromEnemy(enemy->getAIType());
   addPlayerExp(exp);
   
   // Track kill counts by enemy type
   gTotalKills++;
   switch (enemy->getAIType())
   {
      case FOLLOWER: gFollowerKills++; break;
      case WANDERER: gWandererKills++; break;
      case FLYER:    gFlyerKills++;    break;
      default: break;
   }
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

void LevelC::initialise()
{
   Scene::initialise();
   mGameState.nextSceneID = -1;
   
   // Reset kill counters
   gTotalKills = 0;
   gFollowerKills = 0;
   gWandererKills = 0;
   gFlyerKills = 0;

   // Reset weapon unlock status (static variables need to be reset on game restart)
   gHasBloodBullet = true;  // Available from start
   gHasSword = false;
   gHasShield = false;
   gHasBow = false;
   gHasAura = false;
   gHasHeavenLaser = false;
   
   // Reset weapon entity pointers
   gBloodEmitter = nullptr;
   mBowEmitter = nullptr;
   mBowWasAttackingLastFrame = false;
   gHeavenLaserTimer = 0.0f;
   
   // Reset other static variables
   gActiveLaserBeams.clear();
   gCheatModeActive = false;
   mShield = nullptr;
   mShieldAngle = 0.0f;
   gAuraDamageTimer = 0.0f;
   
   // Reset player level and exp
   gPlayerLevel = 1;
   gPlayerExp = 0;
   gExpToNextLevel = 10;
   gExpGivenEnemies.clear();

   mWeaponUpgrades.swordCount = 2;
   mWeaponUpgrades.swordSize = 24.0f; // Initial sword size
   mWeaponUpgrades.swordMaterialLevel = 1; // Initial level 1 material
   mWeaponUpgrades.swordUpgradeCount = 0; // Initial upgrade count

   mWeaponUpgrades.shieldOrbitRadius = 15.0f;
   mWeaponUpgrades.shieldKnockback = 12.0f; // Reduced knockback distance from 20 to 12
   mWeaponUpgrades.shieldCount = 1;
   mWeaponUpgrades.shieldMaterialLevel = 1; // Initial level 1 material
   mWeaponUpgrades.shieldUpgradeCount = 0; // Initial upgrade count

   mWeaponUpgrades.bowRange = 180.0f;
   mWeaponUpgrades.bowCooldown = 2.0f;
   mWeaponUpgrades.bowFrameSpeed = 0.12f;
   mWeaponUpgrades.arrowSpeed = 200.0f;
   mWeaponUpgrades.arrowPierce = 1;
   mWeaponUpgrades.bowMaterialLevel = 1; // Initial level 1 material
   mWeaponUpgrades.bowUpgradeCount = 0; // Initial upgrade count
   mWeaponUpgrades.bloodBulletCD = 0.50f;
   mWeaponUpgrades.bloodBulletDamage = 8; // Increased base damage from 5 to 8
   mWeaponUpgrades.bloodBulletUpgradeCount = 0; // Initial upgrade count

   mWeaponUpgrades.auraRadius = 60.0f;
   mWeaponUpgrades.auraDamage = 1; // Base damage reduced to 1 (actual damage will be 0.5)
   mWeaponUpgrades.auraUpgradeCount = 0; // Initial upgrade count

   // Player attribute initialization
   mWeaponUpgrades.playerSpeed = 1.0f; // Speed multiplier (1.0 = default speed)
   mWeaponUpgrades.playerMaxHP = 100; // Max HP (increased 5x)

   // Load sound effects
   gBloodBulletSound = LoadSound("assets/bloodBulletShoot.wav");
   SetSoundVolume(gBloodBulletSound, 0.2f);
   gUpgradeSound = LoadSound("assets/upgrade.wav");
   SetSoundVolume(gUpgradeSound, 0.5f);
   gChooseUpgradeSound = LoadSound("assets/chooseUpgrade.wav");
   SetSoundVolume(gChooseUpgradeSound, 0.5f);
   gHeavenLaserSound = LoadSound("assets/heavenLaser.wav");
   SetSoundVolume(gHeavenLaserSound, 0.4f);
   gPlayerDeadSound = LoadSound("assets/playerDead.wav");
   SetSoundVolume(gPlayerDeadSound, 0.6f);
   gPlayerHurtSound = LoadSound("assets/hurt 1.wav");
   SetSoundVolume(gPlayerHurtSound, 0.5f);
   
   // Load and play LevelC specific BGM (loop playback)
   mGameState.bgm = LoadMusicStream("assets/levelCbgm.mp3");
   SetMusicVolume(mGameState.bgm, 0.33f);
   PlayMusicStream(mGameState.bgm);
   

   /*
      ----------- MAP -----------
   */
   mGameState.map = new Map(
      LEVELC_WIDTH, LEVELC_HEIGHT,  // map grid cols & rows
      (unsigned int *)mLevelData, // grid data
      "assets/Tileset.png",       // texture filepath
      TILE_DIMENSION,             // tile size
      16, 4,                      // texture cols & rows (4x16)
      mOrigin                     // in-game origin
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
       {32.0f, 32.0f},                  // Reduced texture size from 40.0f to 32.0f
       "assets/hero/Player_run.png",    // texture file address
       ATLAS,                           // single image or atlas?
       {1, 14},                         // atlas dimensions
       xochitlWalkAtlas,                // actual atlas
       PLAYER                           // entity type
   );

   // mGameState.xochitl->setAttackAnimations(xochitlAttackAtlas);
   mGameState.xochitl->setEntityState(WALK);
   
   // Apply player attributes, reduce collision volume
   mGameState.xochitl->setColliderDimensions({mGameState.xochitl->getScale().x / 3.0f,
                                              mGameState.xochitl->getScale().y / 2.0f}); // Reduced from 2.5/1.5 to 3.0/2.0
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
             {mWeaponUpgrades.auraRadius * 2.0f, mWeaponUpgrades.auraRadius * 2.0f}, // Size matches attack range (radius is half, so size is 2x)
             "assets/Effects/17_felspell_spritesheet.png",
          ATLAS,
             {10, 10},
          flameAtlas,
          EFFECT);

      flameAura->setIsEffect(true);
      flameAura->setDirection(DOWN);
      flameAura->setAttackType(AURA);
      flameAura->setAttackRadius(mWeaponUpgrades.auraRadius);
      flameAura->setEntityState(WALK); // Use WALK state, use WalkAnimations
      flameAura->setFrameSpeed(0.1f); // Set animation speed
      flameAura->setWalkAnimations(flameAtlas); // Set WalkAnimations
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
         mSwordAttackTimers.push_back(0.0f); // Initialize attack timer

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

         // Shield size unified to 20.0f (all levels consistent)
         float shieldSize = 20.0f;
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
         // Reduce shield collision volume (from 100% to 60% of shieldSize)
         shield->setColliderDimensions({shieldSize * 0.4f, shieldSize * 0.4f});

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
      ----------- LEVEL C: Survival Mode - Enemies spawn via spawn system -----------
   */
   // Initialize timer and spawn system
   mGameTimer = 0.0f;
   mSpawnTimer = 0.0f;
   mBaseSpawnInterval = 2.0f;
   mWave60Spawned = false;
   mWave90Spawned = false;
   mGameWon = false;
   mGameOver = false;
   
   // Initially spawn a few enemies as opening
   for (int i = 0; i < 5; ++i)
   {
      int type = GetRandomValue(0, 2);
      spawnEnemy(type, 1.0f);
   }

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

Entity *LevelC::findClosestEnemy(Vector2 pos)
{
   Entity *closest = nullptr;
   float minDist = 600.0f;

   for (Entity *enemy : mGameState.collidableEntities)
   {
      if (enemy == nullptr)
         continue;
      if (enemy->getEntityType() != NPC)
         continue;
      if (enemy->isDead())
         continue;

      float d = Vector2Distance(pos, enemy->getPosition());
      if (d < minDist)
      {
         minDist = d;
         closest = enemy;
      }
   }
   return closest;
}

// Find N closest enemies for Heaven Laser targeting
std::vector<Entity*> LevelC::findClosestEnemies(Vector2 pos, int count)
{
   std::vector<Entity*> result;
   std::vector<std::pair<float, Entity*>> distances;

   for (Entity *enemy : mGameState.collidableEntities)
   {
      if (enemy == nullptr)
         continue;
      if (enemy->getEntityType() != NPC)
         continue;
      if (enemy->isDead())
         continue;

      float d = Vector2Distance(pos, enemy->getPosition());
      if (d < 600.0f) // Only consider enemies within range
      {
         distances.push_back({d, enemy});
      }
   }

   // Sort by distance (ascending)
   for (size_t i = 0; i < distances.size(); ++i)
   {
      for (size_t j = i + 1; j < distances.size(); ++j)
      {
         if (distances[j].first < distances[i].first)
         {
            std::swap(distances[i], distances[j]);
         }
      }
   }

   // Get the first N enemies
   for (size_t i = 0; i < distances.size() && (int)i < count; ++i)
   {
      result.push_back(distances[i].second);
   }

   return result;
}

void LevelC::update(float deltaTime)
{
   // Update BGM (loop playback)
   UpdateMusicStream(mGameState.bgm);
   if (IsMusicStreamPlaying(mGameState.bgm) == false)
   {
      // If music finished (after 1 minute), replay (implement loop)
      PlayMusicStream(mGameState.bgm);
   }
   // Check if player is dead
   if (mGameState.xochitl != nullptr && mGameState.xochitl->isDead() && !mGameOver)
   {
      mGameOver = true;
      PlaySound(gPlayerDeadSound); // Play death sound
      mGameState.nextSceneID = 7; // Lose Scene
      return;
   }
   
   // If game has ended, stop updating
   if (mGameOver)
   {
      return;
   }

   // Cheat mode: Press L to open upgrade menu with Heaven Laser always available
   if (IsKeyPressed(KEY_L))
   {
      // printf("[DEBUG] L pressed - Cheat mode activated!\n");
      gCheatModeActive = true;
      openLevelUpMenu();
      return;
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
      PlaySound(gUpgradeSound);
      
      // If Heaven Laser is unlocked, automatically add HP instead of showing menu
      if (gHasHeavenLaser)
      {
         mWeaponUpgrades.playerMaxHP += 10;
         if (mGameState.xochitl)
         {
            mGameState.xochitl->setMaxHP(mWeaponUpgrades.playerMaxHP);
            int newHP = mGameState.xochitl->getHP() + 10;
            if (newHP > mWeaponUpgrades.playerMaxHP)
               newHP = mWeaponUpgrades.playerMaxHP;
            mGameState.xochitl->setCurrentHP(newHP);
         }
      }
      else
      {
         openLevelUpMenu();
      }
   }

   // Clean up experience records of deleted enemies
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
          mGameState.map,     // Enemy / bullet collision with map
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

      // —— Spawn Flyer projectile (8x8 sprite sheet) ——
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
      proj->setColliderDimensions({32.0f * 0.4f, 32.0f * 0.4f});
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
      proj->setSpeed(80.0f); // Further reduced flight speed from 120 to 80
      proj->setWalkAnimations(mFlyerProjectileAtlas); // Set WalkAnimations to play animation
      proj->setCheckCollision(false);
      proj->setLifetime(3.0f);
      proj->setSpawnInvincible(0.0f);
      proj->setAIType(BULLET);
      proj->setOwner(entity);

      mGameState.collidableEntities.push_back(proj);
   }

   Vector2 currentPlayerPosition = mGameState.xochitl->getPosition();
   Vector2 pos = currentPlayerPosition;

   float minX = mOrigin.x - (LEVELC_WIDTH * TILE_DIMENSION) / 2;
   float maxX = mOrigin.x + (LEVELC_WIDTH * TILE_DIMENSION) / 2;
   float minY = mOrigin.y - (LEVELC_HEIGHT * TILE_DIMENSION) / 2;
   float maxY = mOrigin.y + (LEVELC_HEIGHT * TILE_DIMENSION) / 2;

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

   // ---------- Player takes collision damage from enemies ----------
   const int MELEE_TOUCH_DAMAGE = 10; // Melee enemy touch damage (increased 5x)
   const int FLYER_TOUCH_DAMAGE = 5;  // Flyer body touch damage (increased 5x)

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
         if (gPlayerHurtSound.frameCount > 0) PlaySound(gPlayerHurtSound);
      }
      else
      {
         // Wanderer / Follower melee enemies touch player: deal damage once
         mGameState.xochitl->takeDamage(MELEE_TOUCH_DAMAGE);
         if (gPlayerHurtSound.frameCount > 0) PlaySound(gPlayerHurtSound);
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

   // Aura damage: deal damage every 0.1 seconds, but damage reduced to 0.5 (deal 1 damage every 2 attacks)
   gAuraDamageTimer += deltaTime;
   static bool auraDamageToggle = false; // Global toggle, deal damage every 2 cycles
   
   if (gAuraDamageTimer >= AURA_DAMAGE_INTERVAL)
   {
      gAuraDamageTimer = 0.0f; // Reset timer
      auraDamageToggle = !auraDamageToggle; // Toggle damage state
      
      float auraRadius = mWeaponUpgrades.auraRadius;
      int auraDamage = auraDamageToggle ? 1 : 0; // Deal 1 damage every 2 attacks, average 0.5
      
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
            bool wasAlive = !enemy->isDead();
            if (auraDamage > 0) // Only attack when should deal damage
            {
               enemy->takeDamage(auraDamage);
            }
            checkEnemyDeathAndGiveExp(enemy);
            
            // Aura kill heal: if enemy is killed, restore health
            if (wasAlive && enemy->isDead())
            {
               int healAmount = 1; // Restore 1 HP per kill (reduced from 2)
               int currentHP = mGameState.xochitl->getHP();
               int maxHP = mGameState.xochitl->getMaxHP();
               int newHP = currentHP + healAmount;
               if (newHP > maxHP) newHP = maxHP;
               mGameState.xochitl->setCurrentHP(newHP);
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

         // No suitable target, or target not in range → don't shoot
         if (!hasTargetInRange)
         {
            continue;
         }

         // Cooldown not ready → don't shoot
         if (!entity->canTriggerAttack())
         {
            continue;
         }

         // 3. Cooldown finished + has target → immediately fire an arrow
         // Play ATTACK animation (visual only)
         entity->attack();
         entity->forceAnimationStart();

         // Arrow starting position: offset from player position along shotDir, avoid looking like it comes from body center
         Vector2 from = playerPos;
         from.x += shotDir.x * 16.0f;
         from.y += shotDir.y * 16.0f;

         // Create arrow entity (use SINGLE type, because 105.png is a single image)
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
         arrow->setAttackType(ARROW); // Use ARROW type, distinguish from blood bullet
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

         // Adjust rotation speed and radius based on material level
         // Level 1: speed 2.0, radius 15.0
         // Level 2: reduce speed to maintain angular velocity (radius increases, speed decreases proportionally)
         // Level 3: reduce speed to maintain angular velocity (radius increases, speed decreases proportionally)
         float currentOrbitRadius = mWeaponUpgrades.shieldOrbitRadius;
         float baseOrbitRadius = 15.0f; // Level 1 base radius
         float baseOrbitSpeed = 2.0f;  // Level 1 base speed
         
         // Calculate current level speed (maintain angular velocity: v = ω * r, so v_new = v_base * (r_new / r_base))
         float currentOrbitSpeed = baseOrbitSpeed * (baseOrbitRadius / currentOrbitRadius);
         
         mShieldAngles[idx] += currentOrbitSpeed * deltaTime;
         float ang = mShieldAngles[idx];

         Vector2 center = mGameState.xochitl->getPosition();
         Vector2 shieldPos;
         shieldPos.x = center.x + cosf(ang) * currentOrbitRadius;
         shieldPos.y = center.y + sinf(ang) * currentOrbitRadius;
         entity->setPosition(shieldPos);

         float angleRad = atan2f(shieldPos.y - center.y, shieldPos.x - center.x);
         float angleDeg = angleRad * 180.0f / PI;
         entity->setAngle(angleDeg);

         // Manually detect shield collision with enemies (because setCheckCollision(false))
         for (size_t i = 0; i < mGameState.collidableEntities.size(); ++i)
         {
            Entity *enemy = mGameState.collidableEntities[i];
            if (!enemy || !enemy->isActive() || enemy->isDead())
               continue;
            if (enemy->getEntityType() != NPC)
               continue;

            // Use overlaps to manually detect collision
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
         if (idx < 0 || idx >= (int)mSwordAngles.size() || idx >= (int)mSwordOverlaps.size() || idx >= (int)mSwordAttackTimers.size())
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
         float &attackTimer = mSwordAttackTimers[idx];
         
         // Update attack timer
         attackTimer += deltaTime;
         const float SWORD_ATTACK_INTERVAL = 0.25f; // Attack interval
         bool canAttack = (attackTimer >= SWORD_ATTACK_INTERVAL);

         // Calculate damage: based on sword size and count, max 400% of base (20)
         // Base damage 5, increase damage with size and count
         float baseDamage = 5.0f;
         float sizeBonus = (mWeaponUpgrades.swordSize - 24.0f) / 4.0f; // +1 damage per 4.0f size (doubled)
         float countBonus = (mWeaponUpgrades.swordCount - 2) * 3.0f; // +3 damage per additional sword (doubled)
         float totalDamage = baseDamage + sizeBonus + countBonus;
         if (totalDamage > 20.0f) totalDamage = 20.0f; // Max 400% of base (5 * 4 = 20)
         if (totalDamage < 1.0f) totalDamage = 1.0f;
         int swordDamage = (int)totalDamage;

         // Hit radius: centered on sword center, radius based on sword size
         // Base hit radius 14.0f corresponds to base size 24.0f
         const float baseSwordSize = 24.0f;
         const float baseHitRadius = 14.0f;
         float swordHitRadius = baseHitRadius * (mWeaponUpgrades.swordSize / baseSwordSize);

         // If attack interval reached, clear overlapList to allow all enemies in range to take damage
         if (canAttack)
         {
            attackTimer = 0.0f;
            overlapList.clear(); // Clear list, allow all enemies to take damage again
         }

         // Check all enemies if in range, centered on sword center
         // Deal damage once to all enemies in range (each enemy takes damage only once per attack cycle)
         for (size_t e = 0; e < mGameState.collidableEntities.size(); ++e)
         {
            Entity *enemy = mGameState.collidableEntities[e];
            if (!enemy || !enemy->isActive() || enemy->isDead())
               continue;
            if (enemy->getEntityType() != NPC)
               continue;

            // Calculate distance from enemy to sword center
            float dist = Vector2Distance(swordPos, enemy->getPosition());
            bool inRange = (dist <= swordHitRadius);

            // Check if enemy is already in overlapList (already took damage this attack cycle)
            bool wasInList = false;
            for (size_t j = 0; j < overlapList.size(); ++j)
            {
               if (overlapList[j] == enemy)
               {
                  wasInList = true;
                  break;
               }
            }

            // If enemy in range and not in list (hasn't taken damage this attack cycle), deal damage
            if (inRange && !wasInList && canAttack)
            {
               enemy->takeDamage(swordDamage);
               checkEnemyDeathAndGiveExp(enemy);
               overlapList.push_back(enemy); // Add to list, ensure no more damage this attack cycle
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

   // ------------ HEAVEN LASER (Ultimate Weapon) ------------
   // Unlock when: has all items (sword, shield, aura, bow) AND blood bullet maxed (3 upgrades)
   bool hasAllItems = gHasSword && gHasShield && gHasAura && gHasBow;
   bool bloodBulletMaxed = (mWeaponUpgrades.bloodBulletUpgradeCount >= 3);
   
   // Debug: Print Heaven Laser unlock status every 5 seconds
   static float heavenDebugTimer = 0.0f;
   heavenDebugTimer += deltaTime;
   if (heavenDebugTimer >= 5.0f && !gHasHeavenLaser)
   {
      heavenDebugTimer = 0.0f;
      // printf("[DEBUG] Heaven Laser check: Sword=%d Shield=%d Aura=%d Bow=%d BloodBullet=%d/3\n",
      //        gHasSword ? 1 : 0, gHasShield ? 1 : 0, gHasAura ? 1 : 0, gHasBow ? 1 : 0,
      //        mWeaponUpgrades.bloodBulletUpgradeCount);
   }
   
   if (!gHasHeavenLaser && hasAllItems && bloodBulletMaxed)
   {
      // printf("[DEBUG] Heaven Laser AUTO-UNLOCK triggered!\n");
      gHasHeavenLaser = true;
      // Disable blood bullet and bow when Heaven Laser is unlocked
      gHasBloodBullet = false;
      gHasBow = false;
      
      // Remove blood bullet emitter (safely)
      if (gBloodEmitter != nullptr)
      {
         Entity* temp = gBloodEmitter;
         gBloodEmitter = nullptr; // Clear first to prevent double delete
         mAutoAttacks.erase(std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), temp), mAutoAttacks.end());
         mGameState.collidableEntities.erase(std::remove(mGameState.collidableEntities.begin(), mGameState.collidableEntities.end(), temp), mGameState.collidableEntities.end());
         delete temp;
         // printf("[DEBUG] Blood emitter removed (auto-unlock)\n");
      }
      
      // Remove bow emitter (safely)
      if (mBowEmitter != nullptr)
      {
         Entity* temp = mBowEmitter;
         mBowEmitter = nullptr; // Clear first to prevent double delete
         mAutoAttacks.erase(std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), temp), mAutoAttacks.end());
         mGameState.collidableEntities.erase(std::remove(mGameState.collidableEntities.begin(), mGameState.collidableEntities.end(), temp), mGameState.collidableEntities.end());
         delete temp;
         // printf("[DEBUG] Bow emitter removed (auto-unlock)\n");
      }
   }

   // Process Heaven Laser attack (beam style - shoots from player towards nearest enemy)
   if (gHasHeavenLaser && mGameState.xochitl != nullptr)
   {
      gHeavenLaserTimer += deltaTime;
      // DEBUG: Confirm Heaven Laser is active
      static bool debugOnce = false;
      //if (!debugOnce) { printf("[DEBUG] Heaven Laser is ACTIVE!\n"); debugOnce = true; }
      
      // Update existing laser beams - check for damage on frame 3
      for (auto it = gActiveLaserBeams.begin(); it != gActiveLaserBeams.end();)
      {
         LaserBeam& beam = *it;
         Entity* laser = beam.entity;
         
         if (!laser || !laser->isActive() || laser->getLifetime() <= 0)
         {
            // Clean up finished laser
            if (laser)
            {
               mGameState.collidableEntities.erase(std::remove(mGameState.collidableEntities.begin(), mGameState.collidableEntities.end(), laser), mGameState.collidableEntities.end());
               delete laser;
            }
            it = gActiveLaserBeams.erase(it);
         }
         else
         {
            // Check current animation frame and deal damage on frame 3 (index 2)
            int currentFrame = laser->getCurrentFrameIndex();
            if (currentFrame >= 2 && beam.lastDamageFrame < 2)
            {
               beam.lastDamageFrame = currentFrame;
               
               // Check collision with all enemies along the beam
               Vector2 playerPos = mGameState.xochitl->getPosition();
               for (Entity* enemy : mGameState.collidableEntities)
               {
                  if (!enemy || enemy->getEntityType() != NPC || enemy->isDead())
                     continue;
                  if (beam.hitEnemies.find(enemy) != beam.hitEnemies.end())
                     continue; // Already hit this enemy
                  
                  // Check if enemy is within the beam rectangle
                  Vector2 enemyPos = enemy->getPosition();
                  Vector2 toEnemy = {enemyPos.x - playerPos.x, enemyPos.y - playerPos.y};
                  
                  // Project enemy position onto beam direction
                  float dot = toEnemy.x * beam.direction.x + toEnemy.y * beam.direction.y;
                  if (dot > 0 && dot < HEAVEN_LASER_LENGTH)
                  {
                     // Check perpendicular distance
                     float perpDist = fabsf(toEnemy.x * (-beam.direction.y) + toEnemy.y * beam.direction.x);
                     if (perpDist < HEAVEN_LASER_WIDTH + 20.0f) // Add some tolerance
                     {
                        // Hit! Deal damage
                        beam.hitEnemies.insert(enemy);
                        enemy->takeDamage(HEAVEN_LASER_DAMAGE);
                        checkEnemyDeathAndGiveExp(enemy);
                     }
                  }
               }
            }
            ++it;
         }
      }
      
      // Fire new laser beam when cooldown is ready
      if (gHeavenLaserTimer >= gHeavenLaserCD)
      {
         gHeavenLaserTimer = 0.0f;
         
         // Find nearest enemy
         Entity* target = findClosestEnemy(mGameState.xochitl->getPosition());
         
         // Calculate direction (default to right if no enemy)
         Vector2 playerPos = mGameState.xochitl->getPosition();
         Vector2 shotDir = {1.0f, 0.0f}; // Default right
         
         if (target && !target->isDead())
         {
            Vector2 toTarget = {target->getPosition().x - playerPos.x, target->getPosition().y - playerPos.y};
            float len = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
            if (len > 0.001f)
            {
               shotDir.x = toTarget.x / len;
               shotDir.y = toTarget.y / len;
            }
         }
         
         // Create laser beam animation atlas
         std::vector<int> frames;
         for (int i = 0; i < 8; ++i) frames.push_back(i);
         std::map<Direction, std::vector<int>> laserAtlas = {
            {RIGHT, frames},
            {LEFT, frames},
            {UP, frames},
            {DOWN, frames}
         };
         
         // Position beam starting from player, stretched horizontally
         Vector2 beamPos = playerPos;
         beamPos.x += shotDir.x * (HEAVEN_LASER_LENGTH / 2.0f); // Center of beam
         beamPos.y += shotDir.y * (HEAVEN_LASER_LENGTH / 2.0f);
         
         // Calculate rotation angle for the beam
         float angleRad = atan2f(shotDir.y, shotDir.x);
         float angleDeg = angleRad * 180.0f / PI;
         
         // Animation timing: 8 frames at ~16fps = 0.5 seconds total
         // mFrameSpeed is now seconds per frame (after animation fix)
         const float LASER_FRAME_SPEED = 0.06f; // ~16fps (0.06 seconds per frame)
         const float LASER_LIFETIME = 0.6f; // Slightly longer than animation duration
         
         Entity* laserBeam = new Entity(
            beamPos,
            {HEAVEN_LASER_LENGTH, HEAVEN_LASER_WIDTH}, // Stretched horizontally
            "assets/Effects/heavenLaser.png",
            ATLAS,
            {8, 1}, // 8 columns, 1 row
            laserAtlas,
            EFFECT);
         
         laserBeam->setIsEffect(true);
         laserBeam->setEntityState(WALK);
         laserBeam->setFrameSpeed(LASER_FRAME_SPEED);
         laserBeam->setCheckCollision(false);
         laserBeam->setLifetime(LASER_LIFETIME);
         laserBeam->setDirection(RIGHT);
         laserBeam->setAngle(angleDeg); // Rotate beam to face target
         laserBeam->setWalkAnimations(laserAtlas);
         
         mGameState.collidableEntities.push_back(laserBeam);
         
         // Track this beam
         LaserBeam beam;
         beam.entity = laserBeam;
         beam.direction = shotDir;
         beam.lastDamageFrame = -1;
         gActiveLaserBeams.push_back(beam);
         
         // Play Heaven Laser sound (with safety check)
         if (gHeavenLaserSound.frameCount > 0) PlaySound(gHeavenLaserSound);
         // printf("[DEBUG] Laser beam fired!\n");
         
         /*printf("[DEBUG] Laser beam created! Pos: (%.1f, %.1f), Scale: (%.1f, %.1f), Angle: %.1f\n",
                beamPos.x, beamPos.y, laserBeam->getScale().x, laserBeam->getScale().y, angleDeg);
                */
      }
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

   const int PLAYER_PROJECTILE_DAMAGE = 5;  // Blood bullet base damage (increased 5x)
   const int FLYER_PROJECTILE_DAMAGE = 5; // Enemy bullet damage reduced from 10 to 5
   const int ARROW_DAMAGE = 20;            // Arrow damage (increased 5x)

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
      
      bool ownerIsPlayer = (owner && owner == mGameState.xochitl);
      
      // For enemy bullets, ignore collision with enemies or other enemy bullets
      if (!ownerIsPlayer && hit)
      {
         // If hit is enemy or other enemy bullet, ignore
         if (hit->getEntityType() == NPC || 
             (hit->getEntityType() == EFFECT && hit->getAIType() == BULLET && hit->getOwner() != mGameState.xochitl))
         {
            hit = nullptr; // Clear collision object
         }
      }

      bool hitIsPlayer = false;
      bool hitIsEnemy = false;

      if (hit)
      {
         hitIsPlayer = (hit == mGameState.xochitl);
         hitIsEnemy = (hit->getEntityType() == NPC);
      }

      // Avoid bullet hitting self (e.g., Flyer's bullet)
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

      // ---------- Enemy bullets first check if colliding with shield (block) ----------
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
      if (!hit)
         continue;

      // Re-confirm type (hit may have just been changed by fallback above)
      if (!hit || !hit->isActive())
         continue;
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

         bool wasAlive = !hit->isDead();
         hit->takeDamage(damage);
         checkEnemyDeathAndGiveExp(hit);
         
         // Blood bullet kill heal: if enemy is killed, restore health
         if (!isArrow && wasAlive && hit->isDead())
         {
            int healAmount = 1; // Restore 1 HP per kill
            int currentHP = mGameState.xochitl->getHP();
            int maxHP = mGameState.xochitl->getMaxHP();
            int newHP = currentHP + healAmount;
            if (newHP > maxHP) newHP = maxHP;
            mGameState.xochitl->setCurrentHP(newHP);
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
         if (gPlayerHurtSound.frameCount > 0) PlaySound(gPlayerHurtSound);
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
         // Enemy bullets should not collide with other enemy bullets, ignore
         continue;
      }

      // Ignore all other cases
   }

   // ========== LEVEL C: Survival Mode - Enemy Spawn System ==========
   // Update game timer
   mGameTimer += deltaTime;
   
   // Check if completed (120 seconds)
   if (mGameTimer >= SURVIVAL_TIME && !mGameWon && !mGameOver)
   {
      mGameWon = true;
      mGameOver = true; // Game over, pause updates
      mGameState.nextSceneID = 8; // Win screen
      return; // Immediately return, stop updating game
   }
   
   // Calculate difficulty multiplier (up to 8x / 800% at 2 minutes)
   // Time factor: 1.0 -> 4.0 over 2 minutes
   float timeFactor = 1.0f + (mGameTimer / SURVIVAL_TIME) * 3.0f; // 1.0 -> 4.0
   
   // Calculate weapon level influence (takes effect after 30 seconds)
   // Additional increase based on player equipment level
   float weaponFactor = 1.0f;
   if (mGameTimer > 30.0f)
   {
      int totalWeaponLevel = getTotalWeaponLevel();
      // +10% per level, max 100% additional (so weaponFactor goes 1.0 -> 2.0)
      float weaponBonus = totalWeaponLevel * 0.1f;
      if (weaponBonus > 1.0f) weaponBonus = 1.0f; // Limit max additional 100%
      weaponFactor = 1.0f + weaponBonus; // 1.0 -> 2.0
   }
   // Total max difficulty: 4.0 * 2.0 = 8.0 (800%)
   float difficulty = timeFactor * weaponFactor;
   
   // Wave spawns (1 minute and 1.5 minutes)
   if (mGameTimer >= 60.0f && !mWave60Spawned)
   {
      spawnWave(20); // 20 Followers
      mWave60Spawned = true;
   }
   if (mGameTimer >= 90.0f && !mWave90Spawned)
   {
      spawnWave(20); // 20 Followers
      mWave90Spawned = true;
   }
   
   // Enemy speed increases over time: from 0.8 to 1.3 (or 2.5 if Heaven Laser unlocked)
   float maxSpeedMultiplier = gHasHeavenLaser ? 2.5f : 1.3f;
   float speedMultiplier = 0.8f + (mGameTimer / SURVIVAL_TIME) * (maxSpeedMultiplier - 0.8f);
   if (speedMultiplier > maxSpeedMultiplier) speedMultiplier = maxSpeedMultiplier;
   
   // Update speed of all existing enemies
   for (Entity *entity : mGameState.collidableEntities)
   {
      if (!entity || !entity->isActive() || entity->isDead())
         continue;
      if (entity->getEntityType() != NPC)
         continue;
      
      // Calculate base speed based on enemy type
      int baseSpeed = 0;
      if (entity->getAIType() == WANDERER)
      {
         baseSpeed = (int)(Entity::DEFAULT_SPEED * 0.40f);
      }
      else if (entity->getAIType() == FLYER)
      {
         baseSpeed = (int)(Entity::DEFAULT_SPEED * 0.40f);
      }
      else if (entity->getAIType() == FOLLOWER)
      {
         baseSpeed = (int)(Entity::DEFAULT_SPEED * 0.3f);
      }
      
      // Apply time-based speed multiplier
      entity->setSpeed((int)(baseSpeed * speedMultiplier));
   }
   
   // Continuously spawn enemies (spawn speed increases over time)
   mSpawnTimer += deltaTime;
   
   // Spawn interval: gradually decrease from 2 seconds to 0.5 seconds
   float spawnInterval = mBaseSpawnInterval - (mGameTimer / SURVIVAL_TIME) * 1.5f;
   if (spawnInterval < 0.5f) spawnInterval = 0.5f;
   
   // When spawning, decide spawn count based on time (spawn multiple later)
   int spawnCount = 2;  // Initially spawn 2
   if (mGameTimer > 30.0f) spawnCount = 3;  // After 30s spawn 3 each time
   if (mGameTimer > 60.0f) spawnCount = 4;  // After 1 min spawn 4 each time
   if (mGameTimer > 90.0f) spawnCount = 5;  // After 1.5 min spawn 5 each time
   
   // Periodically clean up dead enemies and inactive entities (clean every 0.5 seconds)
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
   
   // Limit max enemy count (80 enemies max, or 100 if Heaven Laser unlocked)
   const size_t MAX_ENEMIES = gHasHeavenLaser ? 100 : 80;
   int enemyCount = 0;
   for (Entity *e : mGameState.collidableEntities)
   {
      if (e && e->getEntityType() == NPC && !e->isDead())
      {
         enemyCount++;
      }
   }
   
   // If enemy count reaches limit, stop spawning new enemies
   if (enemyCount >= MAX_ENEMIES)
   {
      // Prioritize cleaning up dead enemies
      for (auto it = mGameState.collidableEntities.begin(); 
           it != mGameState.collidableEntities.end();)
      {
         Entity* e = *it;
         if (e && e != mGameState.xochitl && e->getEntityType() == NPC && e->isDead())
         {
            delete e;
            it = mGameState.collidableEntities.erase(it);
         }
         else
         {
            ++it;
         }
      }
   }

   if (mSpawnTimer >= spawnInterval)
   {
      mSpawnTimer = 0;
      
      // Check current enemy count
      int currentEnemyCount = 0;
      for (Entity *e : mGameState.collidableEntities)
      {
         if (e && e->getEntityType() == NPC && !e->isDead())
         {
            currentEnemyCount++;
         }
      }
      
      // Only spawn new enemies if count not at limit
      if (currentEnemyCount < MAX_ENEMIES)
      {
         // Dynamic Flyer limit: 5 before 1 min, 10 after 1 min
         int currentMaxFlyers = (mGameTimer >= 60.0f) ? 10 : 5;
         
         // Spawn multiple enemies (using weight system)
         for (int i = 0; i < spawnCount && currentEnemyCount < MAX_ENEMIES; i++)
         {
            int type;
            
            // If Flyer count at limit, can only spawn other types
            if (countActiveFlyers() >= currentMaxFlyers)
            {
               // Weights: 70% Follower, 30% Wanderer
               int roll = GetRandomValue(0, 99);
               type = (roll < 70) ? 2 : 0; // 2=Follower, 0=Wanderer
            }
            else if (mGameTimer >= 60.0f)
            {
               // After 1 minute: Priority spawn Flyers (40% Flyer, 40% Follower, 20% Wanderer)
               int roll = GetRandomValue(0, 99);
               if (roll < 40)
               {
                  type = 1; // Flyer (priority)
               }
               else if (roll < 80)
               {
                  type = 2; // Follower
               }
               else
               {
                  type = 0; // Wanderer
               }
            }
            else
            {
               // Before 1 minute: Normal weight system (60% Follower, 25% Wanderer, 15% Flyer)
               int roll = GetRandomValue(0, 99);
               if (roll < 60)
               {
                  type = 2; // Follower
               }
               else if (roll < 85)
               {
                  type = 0; // Wanderer
               }
               else
               {
                  type = 1; // Flyer
               }
            }
            
            spawnEnemy(type, difficulty);
            currentEnemyCount++;
         }
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

void LevelC::openLevelUpMenu()
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

   // Don't show bow unlock if Heaven Laser is already obtained
   if (!gHasBow && !gHasHeavenLaser)
   {
      LevelUpOption opt;
      opt.title = "Unlock Bow";
      opt.description = "Shoot arrows at distant enemies.";
      opt.upgradeId = 3;
      pool.push_back(opt);
   }

   // Cheat mode: ALWAYS add Heaven Laser option as first choice when L is pressed
   if (gCheatModeActive && !gHasHeavenLaser)
   {
      // printf("[DEBUG] Cheat mode active! Adding Heaven Laser option.\n");
      // Clear pool and add only Heaven Laser for guaranteed selection in cheat mode
      pool.clear();
      LevelUpOption opt;
      opt.title = "HEAVEN LASER";
      opt.description = "Ultimate weapon! Need all items + Blood Bullet maxed.";
      opt.upgradeId = 16;
      pool.push_back(opt);
      // Skip all other options when in cheat mode
   }
   // Normal mode: Add Heaven Laser when has all items AND blood bullet maxed
   else if (!gHasHeavenLaser && gHasSword && gHasShield && gHasAura && gHasBow && mWeaponUpgrades.bloodBulletUpgradeCount >= 3)
   {
      LevelUpOption opt;
      opt.title = "HEAVEN LASER";
      opt.description = "Ultimate weapon! Replaces all ranged weapons.";
      opt.upgradeId = 16;
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

      // Shield knockback has limit (40.0f, +8 each)
      if (mWeaponUpgrades.shieldKnockback < 40.0f)
      {
         LevelUpOption opt;
         opt.title = "Shield Knockback+";
         opt.description = "Increase shield knockback distance.";
         opt.upgradeId = 9;
         pool.push_back(opt);
      }
   }

   // Bow upgrades - NOT available after Heaven Laser
   if (gHasBow && !gHasHeavenLaser)
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
      if (mWeaponUpgrades.auraRadius < 80.0f) // Max 80 (not exceeding 2/3 of existing range)
      {
         LevelUpOption opt;
         opt.title = "Aura Range+";
         opt.description = "Increase aura damage radius.";
         opt.upgradeId = 12;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.auraDamage < 15) // Max 15 damage (reduced from 25)
      {
         LevelUpOption opt;
         opt.title = "Aura DMG+";
         opt.description = "Increase aura damage.";
         opt.upgradeId = 13;
         pool.push_back(opt);
      }
   }

   // 3) Blood bullet upgrades (with limits) - NOT available after Heaven Laser
   if (!gHasHeavenLaser && gHasBloodBullet)
   {
      if (mWeaponUpgrades.bloodBulletCD > 0.25f) // Min CD 0.25 sec
      {
         LevelUpOption opt;
         opt.title = "Blood Bullet CD-";
         opt.description = "Increase blood bullet fire rate.";
         opt.upgradeId = 4;
         pool.push_back(opt);
      }

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

      if (mWeaponUpgrades.playerSpeed < 1.5f) // Max speed 1.5x
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

   // 4) Copy first max 3 from pool to actual menu array
   mLevelUpOptionCount = 0;
   for (int i = 0; i < (int)pool.size() && i < MAX_LEVELUP_OPTIONS; ++i)
   {
      mLevelUpOptions[i] = pool[i];
      ++mLevelUpOptionCount;
   }
}

void LevelC::handleLevelUpInput()
{
   // Keyboard: 1/2/3 to select directly
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

void LevelC::applyUpgradeChoice(int index)
{
   // Reset cheat mode after selection
   gCheatModeActive = false;
   
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
            mSwordAttackTimers.push_back(0.0f); // Initialize attack timer

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

         // Shield size unified to 20.0f (all levels consistent)
         float shieldSize = 20.0f;
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
            // Reduce shield collision volume (from 100% to 60% of shieldSize)
            shield->setColliderDimensions({shieldSize * 0.4f, shieldSize * 0.4f});

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
         flameAura->setFrameSpeed(0.1f); // Set animation speed
         flameAura->setWalkAnimations(flameAtlas); // Set WalkAnimations
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
      mWeaponUpgrades.bloodBulletUpgradeCount++;
      mWeaponUpgrades.bloodBulletCD *= 0.70f; // Reduce by 30% each time
      // Cap minimum CD at 0.25 seconds
      if (mWeaponUpgrades.bloodBulletCD < 0.25f)
      {
         mWeaponUpgrades.bloodBulletCD = 0.25f; // Minimum CD 0.25 sec
      }

      if (gBloodEmitter)
      {
         gBloodEmitter->setAttackInterval(mWeaponUpgrades.bloodBulletCD);
      }
      break;
   }
   case 5:
   {
      mWeaponUpgrades.bloodBulletUpgradeCount++;
      mWeaponUpgrades.bloodBulletDamage += 6; // +6 per upgrade
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
         mWeaponUpgrades.swordUpgradeCount++;
         
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
         mSwordAttackTimers.clear();

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
            mSwordAttackTimers.push_back(0.0f); // Initialize attack timer

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
         mWeaponUpgrades.swordSize += 10.0f; // +10 per upgrade
         if (mWeaponUpgrades.swordSize > 48.0f)
         {
            mWeaponUpgrades.swordSize = 48.0f; // Max 48
         }
         mWeaponUpgrades.swordUpgradeCount++;

         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);

         // Update all existing swords size and material
         for (Entity *sword : mOrbitSwords)
         {
            sword->setScale({mWeaponUpgrades.swordSize, mWeaponUpgrades.swordSize});
            // Note: Entity may not have direct texture swap method, may need to recreate
            // Currently only updating size, material update needs recreation
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 8: Shield Count+ -> Increase shield count
   // ----------------------------------------------------
   case 8:
   {
      if (gHasShield && mWeaponUpgrades.shieldCount < 3) // Max 3 shields
      {
         mWeaponUpgrades.shieldCount++;
         mWeaponUpgrades.shieldUpgradeCount++;
         
         // Increase orbit radius when shield count increases (+5.0f per shield)
         mWeaponUpgrades.shieldOrbitRadius += 5.0f;
         
         // Each shield level adds 25 max HP
         mWeaponUpgrades.playerMaxHP += 25;
         if (mGameState.xochitl != nullptr)
         {
            mGameState.xochitl->setMaxHP(mWeaponUpgrades.playerMaxHP);
            // Also restore current HP (if current HP < new max HP)
            if (mGameState.xochitl->getHP() < mWeaponUpgrades.playerMaxHP)
            {
               mGameState.xochitl->setCurrentHP(mWeaponUpgrades.playerMaxHP);
            }
         }

         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);

         std::map<Direction, std::vector<int>> shieldAtlas = {
             {RIGHT, {0}},
             {LEFT, {0}},
             {UP, {0}},
             {DOWN, {0}}};

         // Recreate all shields, evenly distributed
         // First clean up old ones
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

         // Create new shields
         for (int i = 0; i < mWeaponUpgrades.shieldCount; ++i)
         {
            Vector2 shieldPos = mGameState.xochitl->getPosition();
            float baseAngle = (2.0f * PI / mWeaponUpgrades.shieldCount) * i;
            shieldPos.x += cosf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;
            shieldPos.y += sinf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;

         // Shield size unified to 20.0f (all levels consistent)
         float shieldSize = 20.0f;
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
            // Reduce shield collision volume (from 100% to 60% of shieldSize)
            shield->setColliderDimensions({shieldSize * 0.4f, shieldSize * 0.4f});

            mOrbitShields.push_back(shield);
            mShieldAngles.push_back(baseAngle);

            mAutoAttacks.push_back(shield);
            mGameState.collidableEntities.push_back(shield);
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 9: Shield Knockback+ -> Increase shield knockback distance
   // ----------------------------------------------------
   case 9:
   {
      if (gHasShield)
      {
         mWeaponUpgrades.shieldKnockback += 8.0f; // +8 per upgrade (reduced)
         if (mWeaponUpgrades.shieldKnockback > 40.0f) // Cap at 40
         {
            mWeaponUpgrades.shieldKnockback = 40.0f;
         }
         mWeaponUpgrades.shieldUpgradeCount++;
         
         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);
      }
      break;
   }

   // ----------------------------------------------------
   // 10: Arrow Pierce+ -> Increase arrow pierce count
   // ----------------------------------------------------
   case 10:
   {
      if (gHasBow && mWeaponUpgrades.arrowPierce < 4) // Max 4 pierce
      {
         mWeaponUpgrades.arrowPierce++;
         mWeaponUpgrades.bowUpgradeCount++;
         
         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);
         
         // If material level upgraded, need to recreate bow entity
         if (mBowEmitter)
         {
            // Save current state
            Vector2 bowPos = mBowEmitter->getPosition();
            float bowAngle = mBowEmitter->getAngle();
            
            // Remove old entity from lists
            mGameState.collidableEntities.erase(
                std::remove(mGameState.collidableEntities.begin(), 
                           mGameState.collidableEntities.end(), mBowEmitter),
                mGameState.collidableEntities.end());
            mAutoAttacks.erase(
                std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), mBowEmitter),
                mAutoAttacks.end());
            delete mBowEmitter;
            
            // Recreate bow entity (with new material)
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
   // 11: Bow CD- -> Reduce bow cooldown
   // ----------------------------------------------------
   case 11:
   {
      if (gHasBow && mWeaponUpgrades.bowCooldown > 1.0f)
      {
         mWeaponUpgrades.bowCooldown *= 0.70f; // Reduce by 30% each time
         if (mWeaponUpgrades.bowCooldown < 1.0f)
         {
            mWeaponUpgrades.bowCooldown = 1.0f; // Minimum 1.0 sec
         }
         mWeaponUpgrades.bowUpgradeCount++;

         // Check and update material level
         updateWeaponMaterialLevels(mWeaponUpgrades);

         if (mBowEmitter)
         {
            mBowEmitter->setAttackInterval(mWeaponUpgrades.bowCooldown);
            
            // If material level upgraded, recreate bow entity (same as above)
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
   // 12: Aura Range+ -> Increase aura range
   // ----------------------------------------------------
   case 12:
   {
      // Max range not exceeding 2/3 of initial range
      // Setting conservatively to 80 (about 1.33x of initial 60)
      if (gHasAura && mWeaponUpgrades.auraRadius < 80.0f)
      {
         mWeaponUpgrades.auraRadius += 25.0f; // Reduced from +35 to +25
         if (mWeaponUpgrades.auraRadius > 80.0f)
         {
            mWeaponUpgrades.auraRadius = 80.0f; // Max 80
         }
         mWeaponUpgrades.auraUpgradeCount++;

         // Update all aura entity radii
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
   // 13: Aura DMG+ -> Increase aura damage
   // ----------------------------------------------------
   case 13:
   {
      if (gHasAura && mWeaponUpgrades.auraDamage < 5) // Lowered cap from 25 to 15
      {
         mWeaponUpgrades.auraDamage += 1; // +1 per upgrade
         if (mWeaponUpgrades.auraDamage > 5)
         {
            mWeaponUpgrades.auraDamage = 5; // Max damage 5
         }
         mWeaponUpgrades.auraUpgradeCount++;
      }
      break;
   }

   // ----------------------------------------------------
   // 14: Heal -> Restore health
   // ----------------------------------------------------
   case 14:
   {
      if (mGameState.xochitl)
      {
         int currentHP = mGameState.xochitl->getHP();
         int maxHP = mGameState.xochitl->getMaxHP();
         int healAmount = 25; // Heal amount
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
   // 15: Speed+ -> Increase player speed
   // ----------------------------------------------------
   case 15:
   {
      if (mWeaponUpgrades.playerSpeed < 1.5f)
      {
         mWeaponUpgrades.playerSpeed += 0.1f;
         if (mWeaponUpgrades.playerSpeed > 1.5f)
         {
            mWeaponUpgrades.playerSpeed = 1.5f; // Max 1.5x speed
         }

         if (mGameState.xochitl)
         {
            mGameState.xochitl->setSpeed((int)(Entity::DEFAULT_SPEED * mWeaponUpgrades.playerSpeed));
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 16: Unlock Heaven Laser (Ultimate weapon)
   // ----------------------------------------------------
   case 16:
   {
      if (!gHasHeavenLaser)
      {
         // printf("[DEBUG] Heaven Laser UNLOCKED via upgrade menu!\n");
         gHasHeavenLaser = true;
         // Disable blood bullet and bow when Heaven Laser is unlocked
         gHasBloodBullet = false;
         gHasBow = false;
         
         // Remove blood bullet emitter (safely)
         if (gBloodEmitter != nullptr)
         {
            Entity* temp = gBloodEmitter;
            gBloodEmitter = nullptr; // Clear first to prevent double delete
            mAutoAttacks.erase(std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), temp), mAutoAttacks.end());
            mGameState.collidableEntities.erase(std::remove(mGameState.collidableEntities.begin(), mGameState.collidableEntities.end(), temp), mGameState.collidableEntities.end());
            delete temp;
            // printf("[DEBUG] Blood emitter removed (upgrade menu)\n");
         }
         
         // Remove bow emitter (safely)
         if (mBowEmitter != nullptr)
         {
            Entity* temp = mBowEmitter;
            mBowEmitter = nullptr; // Clear first to prevent double delete
            mAutoAttacks.erase(std::remove(mAutoAttacks.begin(), mAutoAttacks.end(), temp), mAutoAttacks.end());
            mGameState.collidableEntities.erase(std::remove(mGameState.collidableEntities.begin(), mGameState.collidableEntities.end(), temp), mGameState.collidableEntities.end());
            delete temp;
            // printf("[DEBUG] Bow emitter removed (upgrade menu)\n");
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 17: HP Upgrade (after Heaven Laser obtained)
   // ----------------------------------------------------
   case 17:
   {
      mWeaponUpgrades.playerMaxHP += 10;
      if (mGameState.xochitl)
      {
         mGameState.xochitl->setMaxHP(mWeaponUpgrades.playerMaxHP);
         // Also heal by 10
         int newHP = mGameState.xochitl->getHP() + 10;
         if (newHP > mWeaponUpgrades.playerMaxHP)
            newHP = mWeaponUpgrades.playerMaxHP;
         mGameState.xochitl->setCurrentHP(newHP);
      }
      break;
   }

   default:
      break;
   }

   mLevelUpMenuOpen = false;
   gCheatModeActive = false; // Reset cheat mode when menu closes
}

void LevelC::render()
{
   ClearBackground(ColorFromHex(mBGColourHexCode));

   // 1. Render map first (bottom layer)
   mGameState.map->render();

   // 2. Render Aura (below player), slightly dimmed
   for (Entity *entity : mAutoAttacks)
   {
      if (entity && entity->getAttackType() == AURA && entity->isActive())
      {
         // Debug: Check Aura animation state
         static int auraDebugCounter = 0;
         if (auraDebugCounter++ % 120 == 0) // Every 2 seconds at 60fps
         {
            // printf("[DEBUG] Aura: frame=%d, isEffect=%d, textureType=%d, animIndices=%zu\n",
            //        entity->getCurrentFrameIndex(), 
            //        entity->getEntityType() == EFFECT ? 1 : 0,
            //        entity->getTextureType(),
            //        entity->getAnimationIndicesSize());
         }
         
         // Slightly dimmed color (not too dark)
         Color darkTint = {200, 200, 200, 200};
         entity->renderWithTint(darkTint);
      }
   }

   // 3. Render player
   mGameState.xochitl->render();
   
   // 4. Finally render other entities (enemies, weapons, etc., on top layer)
   for (int i = 0; i < mGameState.collidableEntities.size(); ++i)
   {
      if (mGameState.collidableEntities[i])
      {
         Entity* entity = mGameState.collidableEntities[i];
         // Skip aura effect entity since it's already rendered
         // Only skip if EFFECT type and attackType is AURA
         bool isAuraEffect = (entity->getEntityType() == EFFECT && entity->getAttackType() == AURA);
         if (!isAuraEffect)
         {
            entity->render();
         }
      }
   }
   
   // Render active laser beams directly (they might not be in collidableEntities yet)
   for (const LaserBeam& beam : gActiveLaserBeams)
   {
      if (beam.entity && beam.entity->isActive())
      {
         beam.entity->render();
      }
   }
}

void LevelC::renderUI()
{
   Entity *player = mGameState.xochitl;

   // Survival timer (top center)
   float remainingTime = SURVIVAL_TIME - mGameTimer;
   if (remainingTime < 0) remainingTime = 0;
   int minutes = (int)(remainingTime / 60);
   int seconds = (int)(remainingTime) % 60;
   
   const char* timerText = TextFormat("SURVIVE: %d:%02d", minutes, seconds);
   int timerWidth = MeasureText(timerText, 36);
   DrawText(timerText, GetScreenWidth()/2 - timerWidth/2, 20, 36, YELLOW);
   
   // Progress bar
   float progress = mGameTimer / SURVIVAL_TIME;
   if (progress > 1.0f) progress = 1.0f;
   DrawRectangle(GetScreenWidth()/2 - 200, 70, 400, 8, DARKGRAY);
   DrawRectangle(GetScreenWidth()/2 - 200, 70, (int)(400 * progress), 8, GREEN);

   // HP bar
   float hpPercent = (float)player->getHP() / player->getMaxHP();
   DrawRectangle(20, 100, 200, 20, DARKGRAY);
   DrawRectangle(20, 100, (int)(200 * hpPercent), 20, RED);
   DrawText(TextFormat("HP: %d / %d",
                       player->getHP(),
                       player->getMaxHP()),
            20, 125, 16, WHITE);

   // EXP bar
   float expPercent = (float)gPlayerExp / gExpToNextLevel;
   if (expPercent > 1.0f) expPercent = 1.0f;
   DrawRectangle(20, 150, 200, 12, DARKGRAY);
   DrawRectangle(20, 150, (int)(200 * expPercent), 12, SKYBLUE);
   DrawText(TextFormat("Lv.%d  EXP: %d / %d",
                       gPlayerLevel,
                       gPlayerExp,
                       gExpToNextLevel),
            20, 167, 14, WHITE);

   // Weapon status display (left side)
   int weaponY = 200;
   const int weaponFontSize = 14;
   const int weaponLineHeight = 18;
   
   DrawText("-- WEAPONS --", 20, weaponY, weaponFontSize, WHITE);
   weaponY += weaponLineHeight + 5;
   
   // Sword
   if (gHasSword)
   {
      DrawText(TextFormat("Sword Lv.%d (x%d)", mWeaponUpgrades.swordMaterialLevel, mWeaponUpgrades.swordCount), 
               20, weaponY, weaponFontSize, ORANGE);
      weaponY += weaponLineHeight;
   }
   
   // Shield
   if (gHasShield)
   {
      DrawText(TextFormat("Shield Lv.%d (x%d)", mWeaponUpgrades.shieldMaterialLevel, mWeaponUpgrades.shieldCount),
               20, weaponY, weaponFontSize, SKYBLUE);
      weaponY += weaponLineHeight;
   }
   
   // Aura
   if (gHasAura)
   {
      DrawText(TextFormat("Aura Lv.%d (Range: %.0f)", mWeaponUpgrades.auraUpgradeCount + 1, mWeaponUpgrades.auraRadius),
               20, weaponY, weaponFontSize, PURPLE);
      weaponY += weaponLineHeight;
   }
   
   // Blood Bullet
   if (gHasBloodBullet)
   {
      DrawText(TextFormat("Blood Bullet Lv.%d (CD: %.1fs)", mWeaponUpgrades.bloodBulletUpgradeCount + 1, mWeaponUpgrades.bloodBulletCD),
               20, weaponY, weaponFontSize, RED);
      weaponY += weaponLineHeight;
   }
   
   // Bow
   if (gHasBow)
   {
      DrawText(TextFormat("Bow Lv.%d (CD: %.1fs)", mWeaponUpgrades.bowMaterialLevel, mWeaponUpgrades.bowCooldown),
               20, weaponY, weaponFontSize, GREEN);
      weaponY += weaponLineHeight;
   }
   
   // Heaven Laser (special ultimate weapon)
   if (gHasHeavenLaser)
   {
      DrawText("HEAVEN LASER", 20, weaponY, weaponFontSize + 2, GOLD);
      weaponY += weaponLineHeight;
      float cdPercent = gHeavenLaserTimer / gHeavenLaserCD;
      if (cdPercent > 1.0f) cdPercent = 1.0f;
      DrawRectangle(20, weaponY, 100, 8, DARKGRAY);
      DrawRectangle(20, weaponY, (int)(100 * cdPercent), 8, GOLD);
      weaponY += 15;
   }
   
   // Kill counter
   weaponY += 10;
   DrawText(TextFormat("Kills: %d", gTotalKills), 20, weaponY, weaponFontSize, LIGHTGRAY);

   if (mLevelUpMenuOpen)
   {
      renderLevelUpOverlay();
   }
   
   if (mGameWon)
   {
      const char* winText = "VICTORY! You survived 2 minutes!";
      int w = MeasureText(winText, 40);
      DrawText(winText, GetScreenWidth()/2 - w/2, GetScreenHeight()/2, 40, GREEN);
   }
}

void LevelC::renderLevelUpOverlay()
{
   float screenW = (float)GetScreenWidth();
   float screenH = (float)GetScreenHeight();

   // Shader
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

void LevelC::shutdown()
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
   if (gHeavenLaserSound.frameCount > 0)
   {
      UnloadSound(gHeavenLaserSound);
   }
   if (gPlayerDeadSound.frameCount > 0)
   {
      UnloadSound(gPlayerDeadSound);
   }
   if (gPlayerHurtSound.frameCount > 0)
   {
      UnloadSound(gPlayerHurtSound);
   }
   
   // printf("LevelC::shutdown()");
   Scene::shutdown();
}

void LevelC::spawnEnemy(int type, float difficulty)
{
   Vector2 spawnPos = getRandomSpawnPosition(mGameState.xochitl->getPosition(), 150.0f, mOrigin);
   
   std::map<Direction, std::vector<int>> WandererAtlas = {
      {RIGHT, {1, 2, 3, 4, 5, 6, 7}},
      {LEFT, {1, 2, 3, 4, 5, 6, 7}},
      {UP, {1, 2, 3, 4, 5, 6, 7}},
      {DOWN, {1, 2, 3, 4, 5, 6, 7}},
   };
   
   std::map<Direction, std::vector<int>> FlyerAtlas = {
      {RIGHT, {1, 2, 3, 4}},
      {LEFT, {1, 2, 3, 4}},
      {UP, {1, 2, 3, 4}},
      {DOWN, {1, 2, 3, 4}},
   };
   
   std::map<Direction, std::vector<int>> FollowerAtlas = {
      {LEFT, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
      {RIGHT, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
   };
   
   Entity *enemy = nullptr;
   
   if (type == 0) // Wanderer
   {
      enemy = new Entity(
         spawnPos,
         {TILE_DIMENSION, TILE_DIMENSION},
         "assets/Enemy1/Walk.png",
         ATLAS,
         {7, 1},
         WandererAtlas,
         NPC
      );
      enemy->setAIType(WANDERER);
      // Speed increases with time: 0.8 -> 1.1, then 2.0 in last 30 seconds
      float speedMultiplier = 0.8f + (mGameTimer / SURVIVAL_TIME) * 0.3f;
      if (mGameTimer >= 90.0f) speedMultiplier = 2.0f; // Last 30 seconds: max speed
      else if (speedMultiplier > 1.1f) speedMultiplier = 1.1f;
      float baseSpeed = Entity::DEFAULT_SPEED * 0.40f * speedMultiplier;
      enemy->setSpeed((int)baseSpeed);
      // HP: normal difficulty, or 1200% in last 30 seconds
      float hpMultiplier = (mGameTimer >= 90.0f) ? 12.0f : difficulty;
      enemy->setMaxHP((int)(20 * hpMultiplier));
      enemy->setColliderDimensions({enemy->getScale().x * 0.2f, enemy->getScale().y * 0.2f});
      enemy->setOriginalPos(spawnPos); // Set original position for Wanderer AI
      enemy->setFrameSpeed(0.1f); // Set animation speed
      enemy->setEntityState(WALK); // Ensure state is WALK
      enemy->moveRight();
   }
   else if (type == 1) // Flyer
   {
      enemy = new Entity(
         spawnPos,
         {30.0f, 30.0f},
         "assets/Enemy 2/Idle Enemy2.png",
         ATLAS,
         {4, 1},
         FlyerAtlas,
         NPC
      );
      enemy->setAIType(FLYER);
      // Speed increases with time: 0.8 -> 1.1, then 2.0 in last 30 seconds
      float speedMultiplier = 0.8f + (mGameTimer / SURVIVAL_TIME) * 0.3f;
      if (mGameTimer >= 90.0f) speedMultiplier = 2.0f; // Last 30 seconds: max speed
      else if (speedMultiplier > 1.1f) speedMultiplier = 1.1f;
      float baseSpeed = Entity::DEFAULT_SPEED * 0.40f * speedMultiplier;
      enemy->setSpeed((int)baseSpeed);
      // HP: normal difficulty, or 1200% in last 30 seconds
      float hpMultiplier = (mGameTimer >= 90.0f) ? 12.0f : difficulty;
      enemy->setMaxHP((int)(18 * hpMultiplier));
      enemy->setAttackInterval(3.5f); // Fixed fire rate, doesn't scale with difficulty
      enemy->setColliderDimensions({enemy->getScale().x * 0.5f, enemy->getScale().y * 0.5f});
      enemy->setFrameSpeed(0.1f); // Set animation speed
      enemy->setEntityState(WALK); // Ensure state is WALK
      enemy->setDirection(DOWN);
   }
   else if (type == 2) // Follower
   {
      enemy = new Entity(
         spawnPos,
         {TILE_DIMENSION, TILE_DIMENSION},
         "assets/Enemy 3/Walk.png",
         ATLAS,
         {2, 1},
         FollowerAtlas,
         NPC
      );
      enemy->setAIType(FOLLOWER);
      enemy->setAIState(FOLLOWING);
      // Speed increases with time: 0.8 -> 1.1, then 2.0 in last 30 seconds
      float speedMultiplier = 0.8f + (mGameTimer / SURVIVAL_TIME) * 0.3f;
      if (mGameTimer >= 90.0f) speedMultiplier = 2.0f; // Last 30 seconds: max speed
      else if (speedMultiplier > 1.1f) speedMultiplier = 1.1f;
      float baseSpeed = Entity::DEFAULT_SPEED * 0.3f * speedMultiplier;
      enemy->setSpeed((int)baseSpeed);
      // HP: normal difficulty, or 1200% in last 30 seconds
      float hpMultiplier = (mGameTimer >= 90.0f) ? 12.0f : difficulty;
      enemy->setMaxHP((int)(25 * hpMultiplier));
      enemy->setColliderDimensions({enemy->getScale().x / 1.0f, enemy->getScale().y / 1.0f});
      enemy->setFrameSpeed(0.1f); // Set animation speed
      enemy->setEntityState(WALK); // Ensure state is WALK
      enemy->moveRight();
   }
   
   if (enemy)
   {
      enemy->setSpawnInvincible(0.0f);
      enemy->setAcceleration({0.0f, 0.0f});
      enemy->render();
      mGameState.collidableEntities.push_back(enemy);
   }
}

void LevelC::spawnWave(int followerCount)
{
   for (int i = 0; i < followerCount; i++)
   {
      float difficulty = 1.0f + (mGameTimer / SURVIVAL_TIME) * 2.0f;
      if (mGameTimer > 30.0f)
      {
         int totalWeaponLevel = getTotalWeaponLevel();
         difficulty *= (1.0f + totalWeaponLevel * 0.1f);
      }
      // Reduce wave enemy speed (multiply by 0.7)
      spawnEnemy(2, difficulty * 0.7f); // 2 = Follower, 30% speed reduction
   }
}

int LevelC::countActiveFlyers()
{
   int count = 0;
   for (Entity *e : mGameState.collidableEntities)
   {
      if (e && e->getEntityType() == NPC && e->getAIType() == FLYER && !e->isDead())
      {
         count++;
      }
   }
   return count;
}

int LevelC::getTotalWeaponLevel()
{
   int total = 0;
   total += mWeaponUpgrades.swordCount;
   total += (int)(mWeaponUpgrades.swordSize / 8.0f); // Every 8.0f counts as one level
   total += mWeaponUpgrades.shieldCount;
   total += (int)(mWeaponUpgrades.shieldKnockback / 10.0f); // Every 10.0f counts as one level
   total += mWeaponUpgrades.arrowPierce;
   total += (int)((2.0f - mWeaponUpgrades.bowCooldown) / 0.25f); // CD reduction counts as levels
   total += (int)(mWeaponUpgrades.auraRadius / 30.0f); // Every 30.0f counts as one level
   total += mWeaponUpgrades.auraDamage / 5; // Every 5 damage counts as one level
   return total;
}