#include "Scene.h"
#include "LevelC.h"
#include <iostream>

LevelC::LevelC()                                      : Scene { {0.0f}, nullptr   } {}
LevelC::LevelC(Vector2 origin, const char *bgHexCode) : Scene { origin, bgHexCode } {}

LevelC::~LevelC() {}

void LevelC::initialise()
{
   Scene::initialise();
   mGameState.nextSceneID = -1;

   /*
      ----------- MAP -----------
   */
   mGameState.map = new Map(
      LEVEL_WIDTH, LEVEL_HEIGHT,    // map grid cols & rows
      (unsigned int *) mLevelData,  // grid data
      "assets/tileset.png",         // texture filepath
      TILE_DIMENSION,               // tile size
      2, 1,                         // texture cols & rows
      mOrigin                       // in-game origin
   );

   /*
      ----------- PROTAGONIST -----------
   */
   std::map<Direction, std::vector<int>> xochitlWalkAtlas = {
      {RIGHT,  {  10, 11, 12, 13, 14, 15, 16, 17}},
      {LEFT,  {  10, 11, 12, 13, 14, 15, 16, 17}},
      {UP,    {  0, 1, 2, 3, 4, 5}},
      {DOWN, {  0, 1, 2, 3, 4, 5}},
   };

   std::map<EntityState, std::vector<int>> xochitlAttackAtlas = {
      {ATTACK,  {  30, 31, 32, 33, 34, 35, 36}},
   };

   float sizeRatio  = 48.0f / 64.0f;

   mGameState.xochitl = new Entity(
      {mOrigin.x - 650.0f, mOrigin.y}, // position
      {100.0f, 100.0f},             // scale
      "assets/Player.png",                   // texture file address
      ATLAS,                                    // single image or atlas?
      { 11, 10 },                                 // atlas dimensions
      xochitlWalkAtlas,                         // actual atlas
      PLAYER                                    // entity type
   );
   
   mGameState.xochitl->setAttackAnimations(xochitlAttackAtlas);
   mGameState.xochitl->setEntityState(WALK);
   
   mGameState.xochitl->setJumpingPower(550.0f);
   mGameState.xochitl->setColliderDimensions({
      mGameState.xochitl->getScale().x / 2.5f,
      mGameState.xochitl->getScale().y / 1.5f
   });
   mGameState.xochitl->setAcceleration({0.0f, ACCELERATION_OF_GRAVITY});


   Entity* save0 = new Entity {
      {mOrigin.x + 7*TILE_DIMENSION+20, 6*TILE_DIMENSION},
      {TILE_DIMENSION, TILE_DIMENSION},
      "assets/save0.png",
      BLOCK
   };

   Entity* save1 = new Entity {
      {mOrigin.x + 10*TILE_DIMENSION+20, 6*TILE_DIMENSION},
      {TILE_DIMENSION, TILE_DIMENSION},
      "assets/save1.png",
      BLOCK
   };

   Entity* trap1 = new Entity {
      {mOrigin.x - 12.5f*TILE_DIMENSION, 7*TILE_DIMENSION+20},
      {TILE_DIMENSION, TILE_DIMENSION},
      "assets/trap.png",
      BLOCK
   };

   Entity* trap2 = new Entity {
      {mOrigin.x - 1.0f*TILE_DIMENSION, 5*TILE_DIMENSION},
      {TILE_DIMENSION, TILE_DIMENSION},
      "assets/trap.png",
      BLOCK
   }; 
   Entity* trap3 = new Entity {
      {mOrigin.x + 15*TILE_DIMENSION, 11*TILE_DIMENSION},
      {TILE_DIMENSION, TILE_DIMENSION},
      "assets/trap.png",
      BLOCK
   };

   Entity* block = new Entity {
      {mOrigin.x + 7*TILE_DIMENSION+20, 7*TILE_DIMENSION},
      {TILE_DIMENSION, TILE_DIMENSION},
      "assets/mud.png",
      BLOCK
   };

   save1->deactivate();

   mGameState.collidableEntities.push_back(save0);
   mGameState.collidableEntities.push_back(save1);
   mGameState.collidableEntities.push_back(trap1);
   mGameState.collidableEntities.push_back(trap2);
   mGameState.collidableEntities.push_back(trap3);
   
   // create wanderer NPCs
  for (int i = 0; i < 2; ++i) {
      Entity* walker = new Entity(
         {0, 0}, // position
         {TILE_DIMENSION, TILE_DIMENSION},                       // scale
         "assets/trap.png",                  // texture file address
         NPC                                     // entity type
      );
      walker->setAIType(WANDERER);
      walker->setSpeed(Entity::DEFAULT_SPEED * 0.10f);
      walker->setAcceleration({0.0f, ACCELERATION_OF_GRAVITY}); // Add gravity!
      walker->setColliderDimensions({
         walker->getScale().x / 1.5f,
         walker->getScale().y / 1.0f
      });
      walker->moveRight();
      walker->render();
   
      mGameState.collidableEntities.push_back(walker);
  }
  mGameState.collidableEntities[5]->setPosition({mOrigin.x - 5*TILE_DIMENSION , 4*TILE_DIMENSION});
  mGameState.collidableEntities[6]->setPosition({mOrigin.x - 2*TILE_DIMENSION , 4*TILE_DIMENSION});


   // create FLYER NPCs      
  for (int i = 0; i < 3; ++i) {
      Entity* apple = new Entity(
         {0, 0}, // position
         {30.0f, 30.0f},                       // scale
         "assets/apple.png",                // texture file address
         NPC                                     // entity type
      );
      apple->setAIType(FLYER);
      apple->setSpeed(Entity::DEFAULT_SPEED * 0.10f);
      apple->setColliderDimensions({
         apple->getScale().x,
         apple->getScale().y
      });
      apple->setDirection(DOWN);
      apple->render();
      mGameState.collidableEntities.push_back(apple);
  }
  mGameState.collidableEntities[7]->setPosition({mOrigin.x - 10*TILE_DIMENSION , 0});
  mGameState.collidableEntities[8]->setPosition({mOrigin.x + 9*TILE_DIMENSION , 3*TILE_DIMENSION});
  mGameState.collidableEntities[9]->setPosition({mOrigin.x + 5*TILE_DIMENSION , 1*TILE_DIMENSION});
   
  //create follower NPCs
  std::map<Direction, std::vector<int>> followerAnimationAtlas = {
      {LEFT,  { 0,1,2,3,4,5,6,7,8,9 }},
      {RIGHT, { 0,1,2,3,4,5,6,7,8,9 }},
   };
   
   Entity* follower = new Entity(
      {0, 0}, // position
      {TILE_DIMENSION, TILE_DIMENSION},                       // scale
      "assets/Enemy.png",                  // texture file address
      ATLAS,
      {1,10},
      followerAnimationAtlas,
      NPC                                     // entity type
   );
   follower->setAIType(FOLLOWER);
   follower->setSpeed(Entity::DEFAULT_SPEED * 0.20f);
   follower->setAcceleration({0.0f, ACCELERATION_OF_GRAVITY}); // Add gravity!
   follower->setColliderDimensions({
      follower->getScale().x / 2.0f,
      follower->getScale().y / 1.0f
   });
   follower->moveRight();
   follower->render();
   mGameState.collidableEntities.push_back(follower);
   mGameState.collidableEntities[10]->setPosition({mOrigin.x , 7*TILE_DIMENSION});

   // Create Triggers
   Entity* trigger1 = new Entity(
      {mOrigin.x + 5*TILE_DIMENSION, mOrigin.y}, 
      {4*TILE_DIMENSION, TILE_DIMENSION},"assets/mud.png",
      TRIGGER
   );
   trigger1->setColliderDimensions({4*TILE_DIMENSION, TILE_DIMENSION});
   mGameState.collidableEntities.push_back(trigger1);

   Entity* trigger2 = new Entity(
      {mOrigin.x + 7*TILE_DIMENSION, mOrigin.y+2*TILE_DIMENSION-1}, 
      {4*TILE_DIMENSION, TILE_DIMENSION},"assets/mud.png",
      TRIGGER
   );
   trigger2->setColliderDimensions({4*TILE_DIMENSION, TILE_DIMENSION});
   mGameState.collidableEntities.push_back(trigger2);

   Entity* trigger3 = new Entity(
      {mOrigin.x + 7*TILE_DIMENSION, mOrigin.y+2*TILE_DIMENSION-1}, 
      {TILE_DIMENSION, TILE_DIMENSION},"assets/mud.png",
      TRIGGER
   );
   trigger3->setColliderDimensions({TILE_DIMENSION, TILE_DIMENSION});
   mGameState.collidableEntities.push_back(trigger3);


   mGameState.collidableEntities[11]->setPosition({mOrigin.x - 9 * TILE_DIMENSION, 8*TILE_DIMENSION-1});
   mGameState.collidableEntities[12]->setPosition({mOrigin.x - 2 * TILE_DIMENSION, 8*TILE_DIMENSION-1});
   mGameState.collidableEntities[13]->setPosition({mOrigin.x - 2 * TILE_DIMENSION, 6*TILE_DIMENSION});

   mGameState.collidableEntities.push_back(block);

   Entity* trap4 = new Entity {
      {mOrigin.x + 6.0f*TILE_DIMENSION, 8*TILE_DIMENSION},
      {TILE_DIMENSION, TILE_DIMENSION},
      "assets/trap.png",
      BLOCK
   }; 
   Entity* trap5 = new Entity {
      {mOrigin.x + 8.5f*TILE_DIMENSION, 8*TILE_DIMENSION},
      {TILE_DIMENSION, TILE_DIMENSION},
      "assets/trap.png",
      BLOCK
   };

   mGameState.collidableEntities.push_back(trap4);
   mGameState.collidableEntities.push_back(trap5);

   int currLives = lives;
   for (int i = 0; i < 3; ++i) {
      if (currLives > 0) {
         mGameState.hearts.push_back (new Entity (
            {50 + TILE_DIMENSION*i, 50},
            {TILE_DIMENSION, TILE_DIMENSION},
            "assets/heartFull.png",
            BLOCK
         ));
      }
      else {
         mGameState.hearts.push_back (new Entity (
            {50 + TILE_DIMENSION*i, 50},
            {TILE_DIMENSION, TILE_DIMENSION},
            "assets/heartEmpty.png",
            BLOCK
         ));
      }
      --currLives;
   }

   /*
      ----------- CAMERA -----------
   */
   mGameState.camera = { 0 };                                    // zero initialize
   mGameState.camera.target = mGameState.xochitl->getPosition(); // camera follows player
   mGameState.camera.offset = mOrigin;                           // camera offset to center of screen
   mGameState.camera.rotation = 0.0f;                            // no rotation
   mGameState.camera.zoom = 1.0f;                                // default zoom
}

void LevelC::update(float deltaTime)
{
   mGameState.xochitl->update(
      deltaTime,      // delta time / fixed timestep
      nullptr,        // player
      mGameState.map, // map
      mGameState.collidableEntities              // col. entity count
   );

   if (mGameState.xochitl->getCollidedObject() == mGameState.collidableEntities[0]) {
      
            mGameState.collidableEntities[0]->deactivate();
            mGameState.collidableEntities[1]->activate();
            mGameState.collidableEntities[13]->deactivate();
            Vector2 target = {
               mGameState.collidableEntities[1]->getPosition().x-20,
               mGameState.collidableEntities[1]->getPosition().y+20
            };
            mGameState.collidableEntities[4]->moveTo(target, 360.0f);
            Vector2 target1 = {
               mGameState.collidableEntities[15]->getPosition().x+TILE_DIMENSION,
               mGameState.collidableEntities[15]->getPosition().y
            };
            mGameState.collidableEntities[15]->moveTo(target, 360.0f);
   }

   for (size_t i = 2; i < mGameState.collidableEntities.size(); ++i) {
      if (mGameState.xochitl->getEntityState() == ATTACK) {
         if(i==3 || i==4 || i==11 || i==12 || i==13 || i ==14) continue;
         if(mGameState.collidableEntities[11]->isActive()){
            mGameState.collidableEntities[11]->deactivate();
         }
         Vector2 playerPos = mGameState.xochitl->getPosition();
         Vector2 npc = mGameState.collidableEntities[i]->getPosition();
         if (((playerPos.x - npc.x) < 50 && (playerPos.x - npc.x) > 0 && mGameState.xochitl->getDirection() == LEFT)
               || ((npc.x - playerPos.x) < 60 && (npc.x - playerPos.x) > 0 && mGameState.xochitl->getDirection() == RIGHT)
               && abs(npc.y - playerPos.y) < 20 )
             mGameState.collidableEntities[i]->deactivate();
      }
      else if (mGameState.xochitl->getCollidedObject() == mGameState.collidableEntities[i]) {
         if(i == 11 || i == 12 || i==13 || i ==14) continue;
         --lives;
         if (lives > 0) mGameState.nextSceneID = 2;
         else mGameState.nextSceneID = 4;
      }
   }

   Entity* collided = mGameState.xochitl->getCollidedObject();
   if (collided != nullptr) { 
      for (Entity* p : mGameState.projectiles) {
         if (collided == p) {
            --lives;
            if (lives > 0) mGameState.nextSceneID = 2;
            else mGameState.nextSceneID = 4;
         }
      }
   }


   for (Entity* entity : mGameState.collidableEntities) {
      entity->update(
         deltaTime,
         mGameState.xochitl,
         mGameState.map,
         {}
      );
   }

   Vector2 currentPlayerPosition = { mGameState.xochitl->getPosition().x, mOrigin.y-90 };

   if (mGameState.xochitl->getPosition().y > 800.0f) {
      --lives;
      if (lives > 0) mGameState.nextSceneID = 2;
      else mGameState.nextSceneID = 4;
   }
   if (mGameState.xochitl->getCollidedObject() == mGameState.collidableEntities[1]) {
            mGameState.nextSceneID = 5;
      }

   if (mGameState.xochitl->getCollidedObject() == mGameState.collidableEntities[11]) 
   {
      Vector2 target = {
         mGameState.collidableEntities[2]->getPosition().x + 6*TILE_DIMENSION,
         mGameState.collidableEntities[2]->getPosition().y
      };

      mGameState.collidableEntities[2]->moveTo(target, 480.0f);

      Vector2 target2 = {
         mGameState.collidableEntities[3]->getPosition().x - 12*TILE_DIMENSION,
         mGameState.collidableEntities[3]->getPosition().y
      };
      mGameState.collidableEntities[3]->moveTo(target2, 480.0f);
   }

   if (mGameState.xochitl->getCollidedObject() == mGameState.collidableEntities[12])
   {
      float bulletAngle = 0.0f;
      float angleStep = 12.0f;

      for (int i = 0; i < 10; ++i) {
         Entity* apple = new Entity(
            {mGameState.xochitl->getPosition().x, 0}, // position
            {30.0f, 30.0f},                       // scale
            "assets/apple.png",                // texture file address
            NPC                                     // entity type
         );
         apple->setAIType(FLYER);
         apple->setSpeed(Entity::DEFAULT_SPEED * 0.10f);
         apple->setColliderDimensions({
            apple->getScale().x,
            apple->getScale().y
         });
         apple->setDirection(UP);
         apple->render();
         mGameState.collidableEntities.push_back(apple);
      }
   

   }




   panCamera(&mGameState.camera, &currentPlayerPosition);
}

void LevelC::render()
{
   ClearBackground(ColorFromHex(mBGColourHexCode));

   mGameState.xochitl->render();
   for (int i = 0; i < mGameState.collidableEntities.size(); ++i) {
      mGameState.collidableEntities[i]->render();
   }
   for (Entity* bullet : mGameState.projectiles) {
      bullet->render();
   }    
   
   mGameState.map->render();
}

void LevelC::renderUI()
{
   for (int i = 0; i < 3; ++i) {
      mGameState.hearts[i]->render();
   }
}

void LevelC::shutdown()
{
   printf("LevelC::shutdown()");
   Scene::shutdown();
}