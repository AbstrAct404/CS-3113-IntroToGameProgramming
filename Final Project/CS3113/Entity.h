#ifndef ENTITY_H
#define ENTITY_H

#include "Map.h"

enum Direction    { LEFT, UP, RIGHT, DOWN      }; // For walking
enum EntityStatus { ACTIVE, INACTIVE                   };
enum EntityType   { PLAYER, BLOCK, PLATFORM, NPC, EFFECT, TRIGGER, NONE };
enum EntityState  { WALK, WAIT, ATTACK              };
enum AIType       { WANDERER, FOLLOWER, FLYER, BULLET        };
enum AIState      { WALKING, IDLE, FOLLOWING           };
enum AutoAttackType { AURA, PROJECTILE, MELEE, SHIELD, BOW, ARROW, MAGIC      };
enum EffectType    { NONE_EFFECT, CENTER, RANGE, TRACE          };

class Entity
{
private:
    Vector2 mPosition;
    Vector2 mOriginalPos;
    Vector2 mMovement;
    Vector2 mVelocity;
    Vector2 mAcceleration;
    std::string mEntityID = "";
    bool mCheckCollision = true;
    EntityState mEntityState = WALK;
    AutoAttackType mAttackType;
    float mAttackTimer = 0.0f;
    float mAttackCooldown = 2.5f;
    float mAttackInterval = 2.0f;
    float mAttackRadius;
    Entity* mTarget = nullptr;
    EffectType mEffectType = NONE_EFFECT;
    float mLifetime = -1.0f; // in seconds, -1 means infinite
    Entity* mOwner; // who spawned this entity (for projectiles/effects)

    Vector2 mScale;
    Vector2 mColliderDimensions;
    
    Texture2D mTexture;
    TextureType mTextureType;
    Vector2 mSpriteSheetDimensions;
    
    std::map<Direction, std::vector<int>> mWalkAnimations;
    std::map<EntityState, std::vector<int>> mAttackAnimations;
    std::vector<int> mAnimationIndices;
    Direction mDirection;
    float mFrameSpeed;
    int movePhase = 0;

    int mCurrentFrameIndex = 0;
    float mAnimationTime = 0.0f;

    bool mIsJumping = false;
    float mJumpingPower = 0.0f;
    
    static constexpr float ATTACK_DURATION = 0.5f;

    int mSpeed;
    float mAngle;

    bool mIsCollidingTop    = false;
    bool mIsCollidingBottom = false;
    bool mIsCollidingRight  = false;
    bool mIsCollidingLeft   = false;
    bool ignoreMapCollision = false;
    bool mIsEffect = false;


    Entity* mCollidedObject = nullptr;

    EntityStatus mEntityStatus = ACTIVE;
    EntityType   mEntityType;

    AIType  mAIType;
    AIState mAIState;

    bool isColliding(Entity *other) const;

    void checkCollisionY(std::vector<Entity*> collidableEntities);
    void checkCollisionY(Map *map);

    void checkCollisionX(std::vector<Entity*> collidableEntities);
    void checkCollisionX(Map *map);
    
    void resetColliderFlags() 
    {
        mIsCollidingTop    = false;
        mIsCollidingBottom = false;
        mIsCollidingRight  = false;
        mIsCollidingLeft   = false;
        mCollidedObject = nullptr;
    }

    void animate(float deltaTime);
    void AIActivate(Entity *target);
    void AIWander();
    void AIFlyer();
    void AIFollow(Entity *target);

    // ====== HP SYSTEM ======
    int mMaxHP = 100;      // 默认血量提高5倍
    int mCurrentHP = 100;
    bool mInvincible = false;
    float mInvincibleTimer = 0.0f;
    float mInvincibleDuration = 0.3f;
    float mSpawnInvincible = 1.0f;
    bool mAttackActive = false;

    


public:
    static constexpr int   DEFAULT_SIZE          = 250;
    static constexpr int   DEFAULT_SPEED         = 50;
    static constexpr int   DEFAULT_FRAME_SPEED   = 14;
    static constexpr float Y_COLLISION_THRESHOLD = 0.5f;

    Entity();
    Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
        EntityType entityType);
    Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
        TextureType textureType, Vector2 spriteSheetDimensions, 
        std::map<Direction, std::vector<int>> animationAtlas, 
        EntityType entityType);
    ~Entity();

    void update(float deltaTime, Entity *player, Map *map, 
        std::vector<Entity*> collidableEntities);
    void render();
    void renderWithTint(Color tint);
    void normaliseMovement() { Normalise(&mMovement); }

    void jump()       { mIsJumping = true;  }
    void attack();
    void activate()   { mEntityStatus  = ACTIVE;   }
    void deactivate() { mEntityStatus  = INACTIVE; }
    void displayCollider();
    void forceAnimationStart();


    bool isActive() { return mEntityStatus == ACTIVE ? true : false; }

    void moveUp()    { mMovement.y = -1; mDirection = UP;    }
    void moveDown()  { mMovement.y =  1; mDirection = DOWN;  }
    void moveLeft()  { mMovement.x = -1; mDirection = LEFT;  }
    void moveRight() { mMovement.x =  1; mDirection = RIGHT; }
    void moveTo(Vector2 target, float speed);

    void resetMovement() { mMovement = { 0.0f, 0.0f }; }
    Vector2 moveTarget = {0,0};
    float moveSpeed = 0.0f;
    bool isMoving = false;
    float mHomingSpeed = 200.0f;


    Vector2     getPosition()              const { return mPosition;              }
    Vector2     getMovement()              const { return mMovement;              }
    Vector2     getVelocity()              const { return mVelocity;              }
    Vector2     getAcceleration()          const { return mAcceleration;          }
    Vector2     getScale()                 const { return mScale;                 }
    Vector2     getColliderDimensions()    const { return mColliderDimensions;    }
    Vector2     getSpriteSheetDimensions() const { return mSpriteSheetDimensions; }
    Texture2D   getTexture()               const { return mTexture;               }
    TextureType getTextureType()           const { return mTextureType;           }
    size_t      getAnimationIndicesSize()  const { return mAnimationIndices.size(); }
    Direction   getDirection()             const { return mDirection;             }
    int         getFrameSpeed()            const { return mFrameSpeed;            }
    float       getJumpingPower()          const { return mJumpingPower;          }
    bool        isJumping()                const { return mIsJumping;             }
    int         getSpeed()                 const { return mSpeed;                 }
    float       getAngle()                 const { return mAngle;                 }
    EntityType  getEntityType()            const { return mEntityType;            }
    AIType      getAIType()                const { return mAIType;                }
    AIState     getAIState()               const { return mAIState;               }
    std::string getEntityID()              const { return mEntityID;              }
    Entity*     getCollidedObject()        const { return mCollidedObject;    }
    bool        getCheckCollision()        const { return mCheckCollision;        }
    EntityState getEntityState()           const { return mEntityState;           }
    bool        getIsEffect()              const { return mIsEffect;              }

    void setTarget(Entity* target) { mTarget = target; }
    Entity* getTarget() const { return mTarget; }
    float getAttackInterval() const { return mAttackInterval; }
    AutoAttackType getAttackType() const { return mAttackType; }
    
    bool isCollidingTop()    const { return mIsCollidingTop;    }
    bool isCollidingBottom() const { return mIsCollidingBottom; }
    bool isCollidingLeft()    const { return mIsCollidingLeft;    }
    bool isCollidingRight() const { return mIsCollidingRight; }

    std::map<Direction, std::vector<int>> getWalkAnimations() const { return mWalkAnimations; }
    std::map<EntityState, std::vector<int>> getAttackAnimations() const { return mAttackAnimations; }

    void setPosition(Vector2 newPosition)
        { mPosition = newPosition; 
            mOriginalPos = newPosition;            }
    void setOriginalPos(Vector2 newOriginalPos)
        { mOriginalPos = newOriginalPos;          }
    void setMovement(Vector2 newMovement)
        { mMovement = newMovement;                 }
    void setAcceleration(Vector2 newAcceleration)
        { mAcceleration = newAcceleration;         }
    void setScale(Vector2 newScale)
        { mScale = newScale;                       }
    void setTexture(const char *textureFilepath)
        { mTexture = LoadTexture(textureFilepath); }
    void setColliderDimensions(Vector2 newDimensions) 
        { mColliderDimensions = newDimensions;     }
    void setSpriteSheetDimensions(Vector2 newDimensions) 
        { mSpriteSheetDimensions = newDimensions;  }
    void setSpeed(int newSpeed)
        { mSpeed  = newSpeed;                      }
    void setFrameSpeed(float newSpeed)
        { mFrameSpeed = newSpeed;                  }
    void setJumpingPower(float newJumpingPower)
        { mJumpingPower = newJumpingPower;         }
    void setAngle(float newAngle) 
        { mAngle = newAngle;                       }
    void setEntityType(EntityType entityType)
        { mEntityType = entityType;                }
    void setDirection(Direction newDirection)
    { 
        mDirection = newDirection;

        if (mTextureType == ATLAS) {
            // Update animation indices based on current state
            if (mEntityState == WALK)
                mAnimationIndices = mWalkAnimations.at(mDirection);
            else if (mEntityState == ATTACK && !mAttackAnimations.empty())
                mAnimationIndices = mAttackAnimations.at(mEntityState);
        }
    }
    void setAIState(AIState newState)
        { mAIState = newState;                     }
    void setAIType(AIType newType)
        { mAIType = newType;                       }
    void setEntityID(std::string entityID)
        { mEntityID = entityID;                    }
    void setCheckCollision(bool state) { mCheckCollision = state; }
    void setEntityState(EntityState state)  
        { 
            mEntityState = state;
            mCurrentFrameIndex = 0;  // Reset animation when state changes
        }
    void setWalkAnimations(std::map<Direction, std::vector<int>> animations)
        { mWalkAnimations = animations;            }
    void setAttackAnimations(std::map<EntityState, std::vector<int>> animations)
        { mAttackAnimations = animations;          } 
    
    void resize(Vector2 newSize)
    {
        mScale = newSize;

        mColliderDimensions = {
            newSize.x * 0.4f,
            newSize.y * 1.0f
        };
    }
    void setIsEffect(bool effect) { mIsEffect = effect; }
    int getCurrentFrameIndex() const { return mCurrentFrameIndex; }

    void setAttackType(AutoAttackType type) { mAttackType = type; }
    void setAttackRadius(float r) { mAttackRadius = r; }
    float getAttackRadius() const { return mAttackRadius; }

    void takeDamage(int amount);
    bool isDead() const;
    int getHP() const;
    int getMaxHP() const;
    void setAttackInterval(float interval);

    void updateAttackCooldown(float dt) {
        mAttackCooldown -= dt;
    }

    bool canTriggerAttack() {
        if (mAttackCooldown <= 0.0f) {
            mAttackCooldown = mAttackInterval;
            return true;
        }
        return false;
    }

    // Lifetime (seconds). if <= 0 => no lifetime (infinite)
    void setLifetime(float seconds);
    float getLifetime() const;
    bool hasLifetime() const;
    void setSpawnInvincible(float t) { mSpawnInvincible = t; }
    void setMaxHP(int maxHP) { 
        mMaxHP = maxHP; 
        mCurrentHP = maxHP; 
    }
    void setCurrentHP(int currentHP) { 
        mCurrentHP = currentHP; 
        if (mCurrentHP > mMaxHP) mCurrentHP = mMaxHP;
    }
    bool overlaps(Entity* other) const { return isColliding(other); }

        // -------- find owner --------
    void  setOwner(Entity* owner) { mOwner = owner; }
    Entity* getOwner() const { return mOwner; }


};

#endif // ENTITY_CPP