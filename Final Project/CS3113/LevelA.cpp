#include "LevelA.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>

LevelA::LevelA() : Scene{{0.0f}, nullptr} {}
LevelA::LevelA(Vector2 origin, const char *bgHexCode) : Scene{origin, bgHexCode} {}

LevelA::~LevelA() {}

Vector2 getRandomSpawnPosition(Vector2 playerPos, float safeRadius, Vector2 origin)
{
   float x, y;
   Vector2 pos;

   do
   {
      x = GetRandomValue(
          origin.x - (LEVEL_WIDTH * LevelA::TILE_DIMENSION) / 4,
          origin.x + (LEVEL_WIDTH * LevelA::TILE_DIMENSION) / 4);

      y = GetRandomValue(
          origin.y - (LEVEL_HEIGHT * LevelA::TILE_DIMENSION) / 4,
          origin.y + (LEVEL_HEIGHT * LevelA::TILE_DIMENSION) / 4);

      pos = {x, y};

   } while (Vector2Distance(pos, playerPos) < safeRadius);

   return pos;
}

// Get weapon material path functions
const char* getSwordMaterialPath(int level)
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

const char* getShieldMaterialPath(int level)
{
   switch(level)
   {
      case 1: return "assets/weapons/037.png"; // Level 1 material
      case 2: return "assets/weapons/039.png"; // Level 2 material
      case 3: return "assets/weapons/040.png"; // Level 3 material
      default: return "assets/weapons/037.png";
   }
}

const char* getBowMaterialPath(int level)
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
void updateWeaponMaterialLevels(LevelA::WeaponUpgradeStats& upgrades)
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
bool gHasBloodBullet = true; // Available from start
bool gHasSword = false;
bool gHasShield = false;
bool gHasBow = false;
bool gHasAura = false;

// Key weapon entity pointers (for easy parameter modification during upgrades)
Entity *gBloodEmitter = nullptr;

Entity *mBowEmitter = nullptr;
bool mBowWasAttackingLastFrame = false;

Entity *mShield = nullptr;
float mShieldAngle = 0.0f;
float mShieldOrbitSpeed = 2.0f;
float mSwordOrbitRadius = 30.0f;
float mSwordOrbitSpeed = 1.5f;
int PLAYER_PROJECTILE_DAMAGE = 1;
int FLYER_PROJECTILE_DAMAGE = 1;

// Sound effects
Sound gBloodBulletSound = {0};
Sound gUpgradeSound = {0};        // Upgrade sound effect
Sound gChooseUpgradeSound = {0};  // Choose upgrade sound effect
Sound gPlayerDeadSound = {0};     // Player death sound

// Aura damage cooldown
float gAuraDamageTimer = 0.0f;
const float AURA_DAMAGE_INTERVAL = 0.1f; // Deal damage every 0.1 sec

// Experience and level system
int gPlayerLevel = 1;
int gPlayerExp = 0;
int gExpToNextLevel = 10; // EXP needed for next level (increases with level)

// Enemy EXP drop values
const int EXP_FOLLOWER = 10;  // LevelA training robot EXP
const int EXP_WANDERER = 10;  // LevelA training robot EXP
const int EXP_FLYER = 10;     // LevelA training robot EXP

// Set of enemies that have given EXP (to avoid duplicate EXP)
std::set<Entity*> gExpGivenEnemies;

// Calculate EXP needed for level up
int getExpForLevel(int level)
{
   return 10 + (level - 1) * 5; // Lv1 needs 10, Lv2 needs 15, Lv3 needs 20...
}

// Add EXP to player
void addPlayerExp(int amount)
{
   gPlayerExp += amount;
}

bool canLevelUp()
{
   return gPlayerExp >= gExpToNextLevel;
}

// Execute level up
void doLevelUp()
{
   gPlayerExp -= gExpToNextLevel;
   gPlayerLevel++;
   gExpToNextLevel = getExpForLevel(gPlayerLevel);
}

// Get EXP value based on enemy type
int getExpFromEnemy(AIType aiType)
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
void checkEnemyDeathAndGiveExp(Entity* enemy)
{
   if (!enemy) return;
   if (!enemy->isDead()) return;
   if (enemy->getEntityType() != NPC) return;
   
   if (gExpGivenEnemies.find(enemy) != gExpGivenEnemies.end()) return;
   gExpGivenEnemies.insert(enemy);
   int exp = getExpFromEnemy(enemy->getAIType());
   addPlayerExp(exp);
}

float computeAngleDegFromDir(Vector2 dir, float offsetDeg)
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

void LevelA::initialise()
{
   Scene::initialise();
   
   // Load and play BGM (using main.cpp's bgm)
   mGameState.bgm = LoadMusicStream("assets/bgm.mp3");
   SetMusicVolume(mGameState.bgm, 0.33f);
   PlayMusicStream(mGameState.bgm);
   mGameState.nextSceneID = -1;

   mWeaponUpgrades.swordCount = 2;
   mWeaponUpgrades.swordSize = 24.0f; // Initial sword size
   mWeaponUpgrades.swordMaterialLevel = 1;

   mWeaponUpgrades.shieldOrbitRadius = 15.0f;
   mWeaponUpgrades.shieldKnockback = 20.0f;
   mWeaponUpgrades.shieldCount = 1;
   mWeaponUpgrades.shieldMaterialLevel = 1;

   mWeaponUpgrades.bowRange = 180.0f;
   mWeaponUpgrades.bowCooldown = 2.0f;
   mWeaponUpgrades.bowFrameSpeed = 0.12f;
   mWeaponUpgrades.arrowSpeed = 200.0f;
   mWeaponUpgrades.arrowPierce = 1;
   mWeaponUpgrades.bowMaterialLevel = 1;
   mWeaponUpgrades.bloodBulletCD = 0.50f;
   mWeaponUpgrades.bloodBulletDamage = 5;

   mWeaponUpgrades.auraRadius = 60.0f;
   mWeaponUpgrades.auraDamage = 5;

   mWeaponUpgrades.playerSpeed = 1.0f;
   mWeaponUpgrades.playerMaxHP = 100;

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
   mGameState.map = new Map(
      LEVEL_WIDTH, LEVEL_HEIGHT,  // map grid cols & rows
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
       {40.0f, 40.0f},                  // scale
       "assets/hero/Player_run.png",    // texture file address
       ATLAS,                           // single image or atlas?
       {1, 14},                         // atlas dimensions
       xochitlWalkAtlas,                // actual atlas
       PLAYER                           // entity type
   );

   // mGameState.xochitl->setAttackAnimations(xochitlAttackAtlas);
   mGameState.xochitl->setEntityState(WALK);

   // 应用玩家属性
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
             {mWeaponUpgrades.auraRadius * 2.0f, mWeaponUpgrades.auraRadius * 2.0f}, // 大小与攻击范围重合（radius是半径，所以size是2倍）
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

         // 盾牌大小根据材质等级：1级20.0f，2级24.0f，3级28.0f
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
      ----------- NPCs -----------
   */
   std::map<Direction, std::vector<int>> WandererAtlas = {
       {RIGHT, {1, 2, 3, 4, 5, 6, 7}},
       {LEFT, {1, 2, 3, 4, 5, 6, 7}},
       {UP, {1, 2, 3, 4, 5, 6, 7}},
       {DOWN, {1, 2, 3, 4, 5, 6, 7}},
   };
   // TUTORIAL: Create 10 training bots (no AI, static enemies)
   mBotsKilled = 0;
   mTutorialComplete = false;
   
   std::map<Direction, std::vector<int>> botAtlas = {
       {RIGHT, {0, 1, 2, 3, 4, 5, 6}},
       {LEFT, {0, 1, 2, 3, 4, 5, 6}},
       {UP, {0, 1, 2, 3, 4, 5, 6}},
       {DOWN, {0, 1, 2, 3, 4, 5, 6}},
   };

   // Spawn bots in a grid pattern around the center
   float startX = mOrigin.x - 150.0f;
   float startY = mOrigin.y - 100.0f;

   for (int i = 0; i < TRAINING_BOT_COUNT; ++i)
   {
      float x = startX + (i % 5) * 75.0f;
      float y = startY + (i / 5) * 75.0f;
      
      Entity *bot = new Entity(
          {x, y},                       // position
          {40.0f, 40.0f},              // scale
          "assets/Enemy1/Walk.png",    // texture file address
          ATLAS,
          {7, 1},
          botAtlas,
          NPC // entity type
      );
      bot->setAIType(WANDERER);  // Type but won't move
      bot->setSpeed(0);          // Static - no movement
      bot->setSpawnInvincible(0.0f);
      bot->setAcceleration({0.0f, 0.0f});
      bot->setColliderDimensions({bot->getScale().x * 0.5f, bot->getScale().y * 0.5f});
      bot->setMaxHP(30);  // Easy to kill for tutorial
      bot->setDirection(RIGHT);
      
      mGameState.collidableEntities.push_back(bot);
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

Entity *LevelA::findClosestEnemy(Vector2 pos)
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

void LevelA::update(float deltaTime)
{
   // Check if player is dead
   if (mGameState.xochitl != nullptr && mGameState.xochitl->isDead())
   {
      PlaySound(gPlayerDeadSound); // Play death sound
      mGameState.nextSceneID = 7; // Lose Scene
      return;
   }
   
   // 更新BGM（循环播放）
   UpdateMusicStream(mGameState.bgm);
   if (IsMusicStreamPlaying(mGameState.bgm) == false)
   {
      // 如果音乐播放完毕，重新播放（实现循环）
      PlayMusicStream(mGameState.bgm);
   }
   // 跳关功能：按P键快速跳到下一关
   if (IsKeyPressed(KEY_P))
   {
      mGameState.nextSceneID = 4; // Go to LevelBTitle
      return;
   }

   // upgrade menu open: game stalls
   if (mLevelUpMenuOpen)
   {
      handleLevelUpInput();
      return;
   }

   // 检查是否可以升级
   if (canLevelUp())
   {
      doLevelUp();
      PlaySound(gUpgradeSound); // 播放升级音效
      openLevelUpMenu();
   }

   // 清理已删除敌人的经验记录
   for (auto it = gExpGivenEnemies.begin(); it != gExpGivenEnemies.end();)
   {
      Entity* enemy = *it;
      bool found = false;
      for (Entity* e : mGameState.collidableEntities)
      {
         if (e == enemy)
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
          mGameState.xochitl, // 敌人 AI 用这个来追踪玩家
          mGameState.map,     // 敌人 / 子弹和地图的碰撞
          mGameState.collidableEntities);
   }

   // 3. Flyer 远程攻击（在它们移动完之后决定要不要开枪）
   for (Entity *entity : mGameState.collidableEntities)
   {
      if (!entity)
         continue;
      if (entity->getEntityType() != NPC)
         continue;
      if (entity->getAIType() != FLYER)
         continue;
      if (!entity->isActive() || entity->isDead())
         continue;

      // 更新射击冷却
      entity->updateAttackCooldown(deltaTime);
      if (!entity->canTriggerAttack())
         continue;

      // 永远朝玩家开火
      Vector2 from = entity->getPosition();
      Vector2 to = mGameState.xochitl->getPosition();

      float dx = to.x - from.x;
      float dy = to.y - from.y;
      float len2 = dx * dx + dy * dy;
      if (len2 < 0.0001f)
         continue;

      float len = sqrtf(len2);
      Vector2 dir = {dx / len, dy / len};

      // —— 生成 Flyer 的子弹（8x8 sprite sheet） ——
      Entity *proj = new Entity(
          from,
          {32.0f, 32.0f}, // 贴图大小放大一倍
          "assets/Effects/9_brightfire_spritesheet.png",
          ATLAS,
          {8, 8},
          mFlyerProjectileAtlas,
          EFFECT);

      proj->setIsEffect(true);
      proj->setAttackType(PROJECTILE);
      proj->setColliderDimensions({32.0f * 0.2f, 32.0f * 0.2f}); // 碰撞箱为大小的0.2
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
      proj->setColliderDimensions({32.0f * 0.6f, 32.0f * 0.6f}); // 增大碰撞箱从0.2到0.6，更容易命中
      proj->setCheckCollision(false);
      proj->setLifetime(3.0f);
      proj->setSpawnInvincible(0.0f);
      proj->setAIType(BULLET);
      proj->setOwner(entity);

      mGameState.collidableEntities.push_back(proj);
   }

   Vector2 currentPlayerPosition = mGameState.xochitl->getPosition();
   Vector2 pos = currentPlayerPosition;

   float minX = mOrigin.x - (LEVEL_WIDTH * TILE_DIMENSION) / 2;
   float maxX = mOrigin.x + (LEVEL_WIDTH * TILE_DIMENSION) / 2;
   float minY = mOrigin.y - (LEVEL_HEIGHT * TILE_DIMENSION) / 2;
   float maxY = mOrigin.y + (LEVEL_HEIGHT * TILE_DIMENSION) / 2;

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

   // ---------- 玩家被敌人“碰撞伤害” ----------
   const int MELEE_TOUCH_DAMAGE = 10; // 近战怪碰触伤害（提高5倍）
   const int FLYER_TOUCH_DAMAGE = 5;  // Flyer 本体碰触伤害（提高5倍）

   for (Entity *enemy : mGameState.collidableEntities)
   {
      if (!enemy)
         continue;
      if (enemy->getEntityType() != NPC)
         continue;
      if (!enemy->isActive() || enemy->isDead())
         continue;

      if (!enemy->overlaps(mGameState.xochitl))
         continue;

      if (enemy->getAIType() == FLYER)
      {
         // 远程敌人身体碰到玩家：造成一次伤害
         mGameState.xochitl->takeDamage(FLYER_TOUCH_DAMAGE);
      }
      else
      {
         // Wanderer / Follower 等近战敌人碰到玩家：造成一次伤害
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

   // Aura伤害：0.1秒造成一次伤害
   gAuraDamageTimer += deltaTime;
   if (gAuraDamageTimer >= AURA_DAMAGE_INTERVAL)
   {
      gAuraDamageTimer = 0.0f; // 重置计时器
      
      float auraRadius = mWeaponUpgrades.auraRadius;
      int auraDamage = mWeaponUpgrades.auraDamage;
   for (Entity *enemy : mGameState.collidableEntities)
   {
         if (!enemy)
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
         // 更新大小使其与攻击范围重合
         float auraSize = mWeaponUpgrades.auraRadius * 2.0f;
         entity->setScale({auraSize, auraSize});
         continue;
      }

      // bow
      if (entity->getAttackType() == BOW)
      {
         // 1. 弓跟随玩家位置
         Vector2 playerPos = mGameState.xochitl->getPosition();
         entity->setPosition(playerPos);

         // 2. 找离玩家最近的敌人，并判断是否在射程内
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

               // 让弓朝向敌人（纯视觉）
               float angleRad = atan2f(shotDir.y, shotDir.x);
               float angleDeg = angleRad * 180.0f / PI;
               entity->setAngle(angleDeg + 45.0f);
            }
         }

         // 更新冷却
         entity->updateAttackCooldown(deltaTime);

         // 没有合适目标，或者目标不在射程内 → 不射
         if (!hasTargetInRange)
         {
            continue;
         }

         // 冷却没好 → 不射
         if (!entity->canTriggerAttack())
         {
            continue;
         }

         // 3. 冷却结束 + 有目标 → 立刻发射一支 arrow
         // 播放 ATTACK 动画（只是视觉效果）
         entity->attack();
         entity->forceAnimationStart();

         // 箭的起始位置：从玩家位置沿着shotDir偏移一点，避免看起来从身体中间钻出来
         Vector2 from = playerPos;
         from.x += shotDir.x * 16.0f;
         from.y += shotDir.y * 16.0f;

         // 创建箭实体（使用 SINGLE 类型，因为 105.png 是单张图片）
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
         arrow->setAttackType(ARROW); // 使用 ARROW 类型，与血弹区分
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

         // 初始化穿透计数
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
         if (idx < 0)
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

         // 手动检测shield与敌人的碰撞（因为setCheckCollision(false)）
         for (size_t i = 0; i < mGameState.collidableEntities.size(); ++i)
         {
            Entity *enemy = mGameState.collidableEntities[i];
            if (!enemy)
               continue;
            if (enemy->getEntityType() != NPC)
               continue;
            if (!enemy->isActive())
               continue;

            // 使用overlaps手动检测碰撞
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
         if (idx < 0)
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
            if (!enemy)
               continue;
            if (enemy->getEntityType() != NPC)
               continue;
            if (!enemy->isActive())
               continue;
            if (enemy->isDead())
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

               overlapList.push_back(enemy);
            }
            else if (!inRange && wasInRange)
            {
               int lastIndex = (int)overlapList.size() - 1;
               overlapList[foundIndex] = overlapList[lastIndex];
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
      bullet->setColliderDimensions({15.0f * 0.6f, 15.0f * 0.6f}); // 设置碰撞箱，增大判定范围
      bullet->setCheckCollision(false);
      bullet->setLifetime(3.0f);
      bullet->setSpawnInvincible(0.0f);
      bullet->setAIType(BULLET);
      bullet->setOwner(mGameState.xochitl);
      PlaySound(gBloodBulletSound);
      mGameState.collidableEntities.push_back(bullet);
   }

   // ------------ 统一处理所有飞行弹幕（血弹 + 箭 + 敌人子弹）------------
   // 清理已失效的箭的穿透计数
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

   const int PLAYER_PROJECTILE_DAMAGE = 5;  // 血弹基础伤害（提高5倍）
   const int FLYER_PROJECTILE_DAMAGE = 10; // 敌人子弹伤害（提高5倍）
   const int ARROW_DAMAGE = 20;            // 弓箭伤害（提高5倍）

   for (Entity *proj : mGameState.collidableEntities)
   {
      if (!proj)
         continue;

      // 只处理效果类实体
      if (proj->getEntityType() != EFFECT)
         continue;
      if (!proj->isActive())
         continue;

      // 只处理真正“飞行弹幕”的 AI（血弹 / 箭 / Flyer 子弹）
      if (proj->getAIType() != BULLET)
         continue;

      AutoAttackType atkType = proj->getAttackType();
      if (atkType != PROJECTILE && atkType != ARROW)
         continue;

      Entity *owner = proj->getOwner();
      Entity *hit = proj->getCollidedObject();

      bool ownerIsPlayer = (owner == mGameState.xochitl);
      
      // 对于敌人子弹，忽略与敌人或其他敌人子弹的碰撞
      if (!ownerIsPlayer && hit)
      {
         // 如果hit是敌人或其他敌人子弹，忽略
         if (hit->getEntityType() == NPC || 
             (hit->getEntityType() == EFFECT && hit->getAIType() == BULLET))
         {
            Entity *hitOwner = hit->getOwner();
            if (!hitOwner || hitOwner != mGameState.xochitl)
            {
               hit = nullptr; // 清除碰撞对象
            }
         }
      }

      bool hitIsPlayer = false;
      bool hitIsEnemy = false;

      if (hit)
      {
         hitIsPlayer = (hit == mGameState.xochitl);
         hitIsEnemy = (hit->getEntityType() == NPC);
      }

      // 避免子弹打到自己（比如 Flyer 的子弹）
      if (hit == owner)
      {
         hit = nullptr;
         hitIsPlayer = false;
         hitIsEnemy = false;
      }

      // ---------- 玩家子弹兜底：如果 colliders 没有记录到，手动检查一下和敌人的重叠 ----------
      if (ownerIsPlayer && !hitIsEnemy)
      {
         // 手动检查玩家子弹与所有敌人的碰撞
         for (Entity *enemy : mGameState.collidableEntities)
         {
            if (!enemy || !enemy->isActive() || enemy->isDead())
               continue;
            if (enemy->getEntityType() != NPC)
               continue;
            
            // 检查子弹是否与敌人重叠
            if (proj->overlaps(enemy))
            {
               hit = enemy;
               hitIsEnemy = true;
               break; // 命中第一个敌人就停止
            }
         }
      }

      // ---------- 敌人子弹先检查是否与shield碰撞（格挡） ----------
      if (!ownerIsPlayer && !hitIsPlayer)
      {
         bool blockedByShield = false;
         // 检查是否与任何shield碰撞
         for (Entity *shield : mOrbitShields)
         {
            if (shield && shield->isActive() && proj->overlaps(shield))
            {
               // 被shield格挡，不造成伤害
               proj->deactivate();
               blockedByShield = true;
               break;
            }
         }
         if (blockedByShield)
         {
            continue; // 被格挡，跳过后续处理
         }
         
         // 如果没有被格挡，继续检查是否与玩家碰撞
         if (proj->overlaps(mGameState.xochitl))
         {
            hit = mGameState.xochitl;
            hitIsPlayer = true;
         }
      }
      
      // 敌人子弹不应该与敌人或其他敌人子弹碰撞，只与玩家和盾牌碰撞
      // 移除敌人子弹与敌人的碰撞检测

      // 如果最终还是没人被打到，就跳过
      if (!hit)
         continue;

      // 重新确认类型（上面可能刚刚被兜底改过 hit）
      hitIsPlayer = (hit == mGameState.xochitl);
      hitIsEnemy = (hit->getEntityType() == NPC);

      // ---------- 玩家子弹（血弹 / 箭）打敌人 ----------
      if (ownerIsPlayer && hitIsEnemy)
      {
         int damage = PLAYER_PROJECTILE_DAMAGE;
         bool isArrow = (proj->getAttackType() == ARROW);

         if (isArrow)
         {
            damage = ARROW_DAMAGE; // 箭更痛一点
         }
         else
         {
            damage = mWeaponUpgrades.bloodBulletDamage;
         }

         hit->takeDamage(damage);
         checkEnemyDeathAndGiveExp(hit);

         // 如果是箭，检查穿透
         if (isArrow && mArrowPierceCount.find(proj) != mArrowPierceCount.end())
         {
            mArrowPierceCount[proj]--;
            if (mArrowPierceCount[proj] <= 0)
            {
               // 穿透次数用尽，销毁箭
               mArrowPierceCount.erase(proj);
               proj->deactivate();
            }
            // 否则继续穿透，不销毁
         }
         else
         {
            // 血弹或其他，击中就销毁
            proj->deactivate();
         }
         continue;
      }

      // ---------- 敌人子弹打玩家 ----------
      if (!ownerIsPlayer && hitIsPlayer)
      {
         mGameState.xochitl->takeDamage(FLYER_PROJECTILE_DAMAGE);
         proj->deactivate(); // 命中玩家后立即消失
         continue;
      }
      
      // 敌人子弹不应该与其他敌人或敌人子弹碰撞，直接跳过
      if (!ownerIsPlayer && hitIsEnemy)
      {
         // 敌人子弹不应该与敌人碰撞，忽略这种情况
         continue;
      }
      
      // 敌人子弹与其他敌人子弹碰撞的情况也应该忽略
      if (!ownerIsPlayer && hit && hit->getEntityType() == EFFECT && hit->getAIType() == BULLET)
      {
         Entity *hitOwner = hit->getOwner();
         if (!hitOwner || hitOwner != mGameState.xochitl)
         {
            // 敌人子弹不应该与其他敌人子弹碰撞，忽略
            continue;
         }
      }

      // 其他情况一律忽略
   }

   // 定期清理死亡的敌人和不活跃的实体（每0.5秒清理一次）
   static float cleanupTimer = 0.0f;
   cleanupTimer += deltaTime;
   if (cleanupTimer >= 0.5f)
   {
      cleanupTimer = 0.0f;
      for (auto it = mGameState.collidableEntities.begin(); it != mGameState.collidableEntities.end();)
      {
         Entity* e = *it;
         // 清理死亡的敌人或不活跃的实体（但保留玩家和武器）
         if (e && e != mGameState.xochitl)
         {
            // 只清理NPC类型的死亡或不活跃实体
            if (e->getEntityType() == NPC && (e->isDead() || !e->isActive()))
            {
               delete e; // 释放内存
               it = mGameState.collidableEntities.erase(it);
               continue;
            }
            // 清理不活跃的效果类实体（子弹、箭等）
            else if (e->getEntityType() == EFFECT && !e->isActive())
            {
               delete e; // 释放内存
               it = mGameState.collidableEntities.erase(it);
               continue;
            }
         }
         ++it;
      }
   }

   // TUTORIAL: Check if all training bots are killed
   if (!mTutorialComplete)
   {
      int aliveBots = 0;
      for (Entity *e : mGameState.collidableEntities)
      {
         if (e && e->getEntityType() == NPC && !e->isDead())
         {
            aliveBots++;
         }
      }
      
      mBotsKilled = TRAINING_BOT_COUNT - aliveBots;
      
      if (mBotsKilled >= TRAINING_BOT_COUNT)
      {
         mTutorialComplete = true;
         mGameState.nextSceneID = 4;  // Go to LevelBTitle
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

void LevelA::openLevelUpMenu()
{
   mLevelUpMenuOpen = true;
   mLevelUpSelectedIndex = -1;

   // 用一个 pool 临时存所有可以用的选项
   std::vector<LevelUpOption> pool;

   // 1) 还没解锁的武器，先都放进来
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

   // 2) 武器升级选项（如果已解锁）
   if (gHasSword)
   {
      if (mWeaponUpgrades.swordCount < 4) // 最大4把剑（3次升级）
      {
         LevelUpOption opt;
         opt.title = "Sword Count+";
         opt.description = "Increase number of orbiting swords.";
         opt.upgradeId = 6;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.swordSize < 48.0f) // 最大48（3次升级，每次+8）
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
      if (mWeaponUpgrades.shieldCount < 3) // 最大3个盾（2次升级）
      {
         LevelUpOption opt;
         opt.title = "Shield Count+";
         opt.description = "Add another orbiting shield.";
         opt.upgradeId = 8;
         pool.push_back(opt);
      }

      // 盾击退有上限（60.0f，4次升级，每次+10）
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
      if (mWeaponUpgrades.arrowPierce < 4) // 最大4穿透（3次升级）
      {
         LevelUpOption opt;
         opt.title = "Arrow Pierce+";
         opt.description = "Increase arrow penetration count.";
         opt.upgradeId = 10;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.bowCooldown > 1.0f) // 最小1.0秒（3次升级）
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
      if (mWeaponUpgrades.auraRadius < 120.0f) // 最大120（2次升级，每次+30）
      {
         LevelUpOption opt;
         opt.title = "Aura Range+";
         opt.description = "Increase aura damage radius.";
         opt.upgradeId = 12;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.auraDamage < 25) // 最大25伤害（4次升级，每次+5）
      {
         LevelUpOption opt;
         opt.title = "Aura DMG+";
         opt.description = "Increase aura damage.";
         opt.upgradeId = 13;
         pool.push_back(opt);
      }
   }

   // 3) 血弹强化（有上限）
   {
      if (mWeaponUpgrades.bloodBulletCD > 0.25f) // 最小CD 0.25秒（3次升级）
   {
      LevelUpOption opt;
      opt.title = "Blood Bullet CD-";
      opt.description = "Increase blood bullet fire rate.";
      opt.upgradeId = 4;
      pool.push_back(opt);
      }
   }

   {
      if (mWeaponUpgrades.bloodBulletDamage < 20) // 最大伤害20（3次升级，每次+5）
   {
      LevelUpOption opt;
      opt.title = "Blood Bullet DMG+";
      opt.description = "Increase blood bullet damage.";
      opt.upgradeId = 5;
      pool.push_back(opt);
   }
   }

   // 4) 玩家属性升级选项（只在武器选项不足时出现）
   // 计算当前武器相关选项数量（不包括玩家属性升级）
   int weaponOptionCount = (int)pool.size();
   
   // 如果武器选项不足3个，添加玩家属性升级选项
   if (weaponOptionCount < 3)
   {
      {
         LevelUpOption opt;
         opt.title = "Heal";
         opt.description = "Restore 25 HP.";
         opt.upgradeId = 14;
         pool.push_back(opt);
      }

      if (mWeaponUpgrades.playerSpeed < 1.5f) // 最大速度1.5倍
      {
         LevelUpOption opt;
         opt.title = "Speed+";
         opt.description = "Increase player movement speed.";
         opt.upgradeId = 15;
         pool.push_back(opt);
      }
   }

   // 3) 随机打乱pool（简单的Fisher-Yates shuffle）
   for (int i = (int)pool.size() - 1; i > 0; --i)
   {
      int j = GetRandomValue(0, i);
      LevelUpOption temp = pool[i];
      pool[i] = pool[j];
      pool[j] = temp;
   }

   // 4) 把 pool 前最多 3 个拷贝到真正的菜单数组里
   mLevelUpOptionCount = 0;
   for (int i = 0; i < (int)pool.size() && i < MAX_LEVELUP_OPTIONS; ++i)
   {
      mLevelUpOptions[i] = pool[i];
      ++mLevelUpOptionCount;
   }
}

void LevelA::handleLevelUpInput()
{
   // 键盘：1/2/3 直接选
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

   // 鼠标：移动高亮，点击选择
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

void LevelA::applyUpgradeChoice(int index)
{
   // 播放选择升级音效
   PlaySound(gChooseUpgradeSound);
   if (index < 0 || index >= mLevelUpOptionCount)
      return;

   LevelUpOption chosen = mLevelUpOptions[index];
   std::cout << "Chosen upgrade: " << chosen.title << std::endl;

   switch (chosen.upgradeId)
   {
   // ----------------------------------------------------
   // 1: Unlock Sword  → 生成一圈环绕剑
   // ----------------------------------------------------
   case 0:
   {
      if (!gHasSword)
      {
         gHasSword = true;

         std::map<Direction, std::vector<int>> dummySwordAtlas = {
             {RIGHT, {0}}};

         int swordCount = mWeaponUpgrades.swordCount; // 你在 initialise 里设成 2

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

            // ★★★ 关键：为这把剑创建一个空的 overlapList
            std::vector<Entity *> emptyList;
            mSwordOverlaps.push_back(emptyList);

            mGameState.collidableEntities.push_back(sword);
            mAutoAttacks.push_back(sword);
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 2: Unlock Shield → 生成环绕盾
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

         // 盾牌大小根据材质等级：1级20.0f，2级24.0f，3级28.0f
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
   // 3: Unlock Aura → 生成火焰光环
   // ----------------------------------------------------
   case 2:
   {
      if (!gHasAura)
      {
         gHasAura = true;

         // 构造 flameAtlas（和你 initialise 里的一样）
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
             {mWeaponUpgrades.auraRadius * 2.0f, mWeaponUpgrades.auraRadius * 2.0f}, // 大小与攻击范围重合
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
   // 4: Unlock Bow → 生成弓的发射器
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

         // 面向最近敌人（如果有）
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
      mWeaponUpgrades.bloodBulletCD *= 0.75f; // 每次减少25%，更明显
      if (mWeaponUpgrades.bloodBulletCD < 0.25f)
      {
         mWeaponUpgrades.bloodBulletCD = 0.25f; // 给个下限
      }

      if (gBloodEmitter)
      {
         gBloodEmitter->setAttackInterval(mWeaponUpgrades.bloodBulletCD);
      }
      break;
   }
   case 5:
   {
      mWeaponUpgrades.bloodBulletDamage += 5; // 每次+5
      if (mWeaponUpgrades.bloodBulletDamage > 20)
      {
         mWeaponUpgrades.bloodBulletDamage = 20; // 给个上限
      }
      break;
   }

   // ----------------------------------------------------
   // 6: Sword Count+ → 增加剑的数量
   // ----------------------------------------------------
   case 6:
   {
      if (gHasSword && mWeaponUpgrades.swordCount < 4) // 最大4把剑
      {
         mWeaponUpgrades.swordCount++;
         
         // 检查并更新材质等级
         updateWeaponMaterialLevels(mWeaponUpgrades);
         
         std::map<Direction, std::vector<int>> dummySwordAtlas = {
             {RIGHT, {0}}};

         // 先清理旧的剑
         for (Entity *oldSword : mOrbitSwords)
         {
            // 从collidableEntities和mAutoAttacks中移除
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

         // 创建新的剑，均匀分布
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
   // 7: Sword Size+ → 增加剑的大小
   // ----------------------------------------------------
   case 7:
   {
      if (gHasSword && mWeaponUpgrades.swordSize < 48.0f)
      {
         mWeaponUpgrades.swordSize += 8.0f; // 每次+8，更明显
         if (mWeaponUpgrades.swordSize > 48.0f)
         {
            mWeaponUpgrades.swordSize = 48.0f; // 最大48
         }

         // 检查并更新材质等级
         updateWeaponMaterialLevels(mWeaponUpgrades);

         // 更新所有现有剑的大小和材质
         for (Entity *sword : mOrbitSwords)
         {
            sword->setScale({mWeaponUpgrades.swordSize, mWeaponUpgrades.swordSize});
            // 注意：Entity可能没有直接更换贴图的方法，这里可能需要重新创建实体
            // 暂时只更新大小，材质更新需要重新创建（在下次升级时）
         }
      }
      break;
   }

   // ----------------------------------------------------
   // 8: Shield Count+ → 增加盾的数量
   // ----------------------------------------------------
   case 8:
   {
      if (gHasShield && mWeaponUpgrades.shieldCount < 3) // 最大3个盾
      {
         mWeaponUpgrades.shieldCount++;

         // 检查并更新材质等级
         updateWeaponMaterialLevels(mWeaponUpgrades);

         std::map<Direction, std::vector<int>> shieldAtlas = {
             {RIGHT, {0}},
             {LEFT, {0}},
             {UP, {0}},
             {DOWN, {0}}};

         // 重新创建所有盾，使其均匀分布
         // 先清理旧的
         for (Entity *shield : mOrbitShields)
         {
            // 从collidableEntities和mAutoAttacks中移除
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

         // 创建新盾
         for (int i = 0; i < mWeaponUpgrades.shieldCount; ++i)
         {
            Vector2 shieldPos = mGameState.xochitl->getPosition();
            float baseAngle = (2.0f * PI / mWeaponUpgrades.shieldCount) * i;
            shieldPos.x += cosf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;
            shieldPos.y += sinf(baseAngle) * mWeaponUpgrades.shieldOrbitRadius;

         // 盾牌大小根据材质等级：1级20.0f，2级24.0f，3级28.0f
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
   // 9: Shield Knockback+ → 增加盾的击退距离
   // ----------------------------------------------------
   case 9:
   {
      if (gHasShield)
      {
         mWeaponUpgrades.shieldKnockback += 10.0f; // 每次+10，更明显
         if (mWeaponUpgrades.shieldKnockback > 60.0f)
         {
            mWeaponUpgrades.shieldKnockback = 60.0f; // 上限60.0f
         }
         
         // 检查并更新材质等级
         updateWeaponMaterialLevels(mWeaponUpgrades);
      }
      break;
   }

   // ----------------------------------------------------
   // 10: Arrow Pierce+ → 增加箭的穿透数量
   // ----------------------------------------------------
   case 10:
   {
      if (gHasBow && mWeaponUpgrades.arrowPierce < 4) // 最大4穿透
      {
         mWeaponUpgrades.arrowPierce++;
         
         // 检查并更新材质等级
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
   // 11: Bow CD- → 减少弓的冷却时间
   // ----------------------------------------------------
   case 11:
   {
      if (gHasBow && mWeaponUpgrades.bowCooldown > 1.0f)
      {
         mWeaponUpgrades.bowCooldown *= 0.75f; // 每次减少25%，更明显
         if (mWeaponUpgrades.bowCooldown < 1.0f)
         {
            mWeaponUpgrades.bowCooldown = 1.0f; // 最小1.0秒
         }

         // 检查并更新材质等级
         updateWeaponMaterialLevels(mWeaponUpgrades);

         if (mBowEmitter)
         {
            mBowEmitter->setAttackInterval(mWeaponUpgrades.bowCooldown);
            
            // 如果材质等级提升，重新创建弓实体（同上）
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
   // 12: Aura Range+ → 增加光环范围
   // ----------------------------------------------------
   case 12:
   {
      if (gHasAura && mWeaponUpgrades.auraRadius < 120.0f)
      {
         mWeaponUpgrades.auraRadius += 30.0f; // 每次+30，更明显
         if (mWeaponUpgrades.auraRadius > 120.0f)
         {
            mWeaponUpgrades.auraRadius = 120.0f; // 最大120
         }

         // 更新所有aura实体的半径
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
   // 13: Aura DMG+ → 增加光环伤害
   // ----------------------------------------------------
   case 13:
   {
      if (gHasAura && mWeaponUpgrades.auraDamage < 25)
      {
         mWeaponUpgrades.auraDamage += 5; // 每次+5
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

void LevelA::render()
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

void LevelA::renderUI()
{
   Entity *player = mGameState.xochitl;

   // Tutorial title
   const char* title = "TUTORIAL 1: ATTACK TRAINING";
   int titleWidth = MeasureText(title, 28);
   DrawText(title, GetScreenWidth()/2 - titleWidth/2, 30, 28, YELLOW);

   // Tutorial instructions
   DrawText("Your blood bullets auto-attack nearby enemies!", 50, 80, 18, WHITE);
   DrawText("Kill enemies to gain EXP and level up.", 50, 105, 18, WHITE);
   DrawText("When you level up, you can choose upgrades!", 50, 130, 18, SKYBLUE);
   
   // Progress
   DrawText(TextFormat("Training Bots Killed: %d / %d", mBotsKilled, TRAINING_BOT_COUNT), 50, 170, 20, GREEN);
   
   // Move instructions
   DrawText("Move: W A S D", 50, GetScreenHeight() - 50, 16, LIGHTGRAY);

   // 血量条
   float hpPercent = (float)player->getHP() / player->getMaxHP();
   DrawRectangle(20, 210, 200, 20, DARKGRAY);
   DrawRectangle(20, 210, (int)(200 * hpPercent), 20, RED);
   DrawText(TextFormat("HP: %d / %d",
                       player->getHP(),
                       player->getMaxHP()),
            20, 235, 16, WHITE);

   // 经验条
   float expPercent = (float)gPlayerExp / gExpToNextLevel;
   if (expPercent > 1.0f) expPercent = 1.0f;
   DrawRectangle(20, 260, 200, 12, DARKGRAY);
   DrawRectangle(20, 260, (int)(200 * expPercent), 12, SKYBLUE);
   DrawText(TextFormat("Lv.%d  EXP: %d / %d",
                       gPlayerLevel,
                       gPlayerExp,
                       gExpToNextLevel),
            20, 277, 14, WHITE);

   if (mLevelUpMenuOpen)
   {
      renderLevelUpOverlay();
   }

   if (mTutorialComplete)
   {
      const char* complete = "TRAINING COMPLETE! Loading next level...";
      int w = MeasureText(complete, 30);
      DrawText(complete, GetScreenWidth()/2 - w/2, GetScreenHeight()/2, 30, GREEN);
   }
}

void LevelA::renderLevelUpOverlay()
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

void LevelA::shutdown()
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
   
   // printf("LevelA::shutdown()");
   Scene::shutdown();
}