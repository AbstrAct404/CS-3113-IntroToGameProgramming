#include "Entity.h"
#include <cmath>

Entity::Entity() : mPosition {0.0f, 0.0f}, mMovement {0.0f, 0.0f}, 
                   mVelocity {0.0f, 0.0f}, mAcceleration {0.0f, 0.0f},
                   mScale {DEFAULT_SIZE, DEFAULT_SIZE},
                   mColliderDimensions {DEFAULT_SIZE, DEFAULT_SIZE}, 
                   mTexture {NULL}, mTextureType {SINGLE}, mAngle {0.0f},
                   mSpriteSheetDimensions {}, mDirection {RIGHT}, 
                   mWalkAnimations {{}}, mAnimationIndices {}, mFrameSpeed {0},
                   mEntityType {NONE},mIsCollidingBottom(false),
                   mAIType{WANDERER}, mAIState{IDLE}, mOwner(NULL){ }

Entity::Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
    EntityType entityType) : mPosition {position}, mVelocity {0.0f, 0.0f}, 
    mAcceleration {0.0f, 0.0f}, mScale {scale}, mMovement {0.0f, 0.0f}, 
    mColliderDimensions {scale}, mTexture {LoadTexture(textureFilepath)}, 
    mTextureType {SINGLE}, mDirection {RIGHT}, mWalkAnimations {{}}, 
    mAnimationIndices {}, mFrameSpeed {0}, mSpeed {DEFAULT_SPEED}, 
    mAngle {0.0f}, mEntityType {entityType}, mOriginalPos(position),
    mIsCollidingBottom(false),mAIType{WANDERER}, mAIState{IDLE}, mOwner(NULL) { }

Entity::Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
        TextureType textureType, Vector2 spriteSheetDimensions, std::map<Direction, 
        std::vector<int>> animationAtlas, EntityType entityType) : 
        mPosition {position}, mVelocity {0.0f, 0.0f}, 
        mAcceleration {0.0f, 0.0f}, mMovement { 0.0f, 0.0f }, mScale {scale},
        mColliderDimensions {scale}, mTexture {LoadTexture(textureFilepath)}, 
        mTextureType {ATLAS}, mSpriteSheetDimensions {spriteSheetDimensions},
        mWalkAnimations {animationAtlas}, mDirection {RIGHT},
        mAnimationIndices {animationAtlas.at(RIGHT)}, 
        mFrameSpeed {DEFAULT_FRAME_SPEED}, mAngle { 0.0f }, 
        mSpeed { DEFAULT_SPEED }, mEntityType {entityType}, mOriginalPos(position),
        mIsCollidingBottom(false), mAIType{WANDERER}, mAIState{IDLE}, mOwner(NULL){ }

Entity::~Entity() { UnloadTexture(mTexture); };

void Entity::checkCollisionY(std::vector<Entity*> collidableEntities)
{
    for (int i = 0; i < collidableEntities.size(); i++)
    {
        // STEP 1: For every entity that our player can collide with...
        Entity *collidableEntity = collidableEntities[i];
        
        if ((mEntityType == PLAYER || mEntityType == NPC) &&
            collidableEntity->mIsEffect &&
            collidableEntity->mAttackType == PROJECTILE)
        {
            continue;
        }

        if (isColliding(collidableEntity) && collidableEntity->mCheckCollision)
        {
            // Track collision for gameplay purposes
            mCollidedObject = collidableEntity;
            
            // Skip physical collision resolution for NPCs (pass through them)
            if (collidableEntity->mEntityType == NPC) continue;
            
            // STEP 2: Calculate the distance between its centre and our centre
            //         and use that to calculate the amount of overlap between
            //         both bodies.
            float yDistance = fabs(mPosition.y - collidableEntity->mPosition.y);
            float yOverlap  = fabs(yDistance - (mColliderDimensions.y / 2.0f) - 
                              (collidableEntity->mColliderDimensions.y / 2.0f));
            
            // STEP 3: "Unclip" ourselves from the other entity, and zero our
            //         vertical velocity.
            if (mVelocity.y > 0) 
            {
                mPosition.y -= yOverlap;
                mVelocity.y  = 0;
                mIsCollidingBottom = true;
            } else if (mVelocity.y < 0) 
            {
                mPosition.y += yOverlap;
                mVelocity.y  = 0;
                mIsCollidingTop = true;
            }
        }
    }
}

void Entity::checkCollisionX(std::vector<Entity*> collidableEntities)
{
    for (size_t i = 0; i < collidableEntities.size(); i++)
    {
        Entity *collidableEntity = collidableEntities[i];
        
        if ((mEntityType == PLAYER || mEntityType == NPC) &&
            collidableEntity->mIsEffect &&
            collidableEntity->mAttackType == PROJECTILE)
        {
            continue;
        }
        
        if (isColliding(collidableEntity) && collidableEntity->mCheckCollision)
        {
            // Track collision for gameplay purposes
            mCollidedObject = collidableEntity;
            
            // Skip physical collision resolution for NPCs (pass through them)
            if (collidableEntity->mEntityType == NPC || collidableEntity->mEntityType == NPC) continue;
            
            // When standing on a platform, we're always slightly overlapping
            // it vertically due to gravity, which causes false horizontal
            // collision detections. So the solution I found is only resolve X
            // collisions if there's significant Y overlap, preventing the 
            // platform we're standing on from acting like a wall.
            float yDistance = fabs(mPosition.y - collidableEntity->mPosition.y);
            float yOverlap  = fabs(yDistance - (mColliderDimensions.y / 2.0f) - (collidableEntity->mColliderDimensions.y / 2.0f));

            // Skip if barely touching vertically (standing on platform)
            if (yOverlap < Y_COLLISION_THRESHOLD) continue;

            float xDistance = fabs(mPosition.x - collidableEntity->mPosition.x);
            float xOverlap  = fabs(xDistance - (mColliderDimensions.x / 2.0f) - (collidableEntity->mColliderDimensions.x / 2.0f));

            if (mVelocity.x > 0) {
                mPosition.x     -= xOverlap;
                mVelocity.x      = 0;

                // Collision!
                mIsCollidingRight = true;
            } else if (mVelocity.x < 0) {
                mPosition.x    += xOverlap;
                mVelocity.x     = 0;
 
                // Collision!
                mIsCollidingLeft = true;
            }
        }
    }
}

void Entity::checkCollisionY(Map *map)
{
    if (map == nullptr) return;

    Vector2 topCentreProbe    = { mPosition.x, mPosition.y - (mColliderDimensions.y / 2.0f) };
    Vector2 topLeftProbe      = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y - (mColliderDimensions.y / 2.0f) };
    Vector2 topRightProbe     = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y - (mColliderDimensions.y / 2.0f) };

    Vector2 bottomCentreProbe = { mPosition.x, mPosition.y + (mColliderDimensions.y / 2.0f) };
    Vector2 bottomLeftProbe   = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y + (mColliderDimensions.y / 2.0f) };
    Vector2 bottomRightProbe  = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y + (mColliderDimensions.y / 2.0f) };

    float xOverlap = 0.0f;
    float yOverlap = 0.0f;

    // COLLISION ABOVE (jumping upward)
    if ((map->isSolidTileAt(topCentreProbe, &xOverlap, &yOverlap) ||
         map->isSolidTileAt(topLeftProbe, &xOverlap, &yOverlap)   ||
         map->isSolidTileAt(topRightProbe, &xOverlap, &yOverlap)) && mVelocity.y < 0.0f)
    {
        mPosition.y += yOverlap;   // push down
        mVelocity.y  = 0.0f;
        mIsCollidingTop = true;
    }

    // COLLISION BELOW (falling downward)
    if ((map->isSolidTileAt(bottomCentreProbe, &xOverlap, &yOverlap) ||
         map->isSolidTileAt(bottomLeftProbe, &xOverlap, &yOverlap)   ||
         map->isSolidTileAt(bottomRightProbe, &xOverlap, &yOverlap)) && mVelocity.y > 0.0f)
    {
        mPosition.y -= yOverlap;   // push up
        mVelocity.y  = 0.0f;
        mIsCollidingBottom = true;
    } 
}

void Entity::checkCollisionX(Map *map)
{
    if (map == nullptr) return;

    Vector2 leftCentreProbe   = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y };

    Vector2 rightCentreProbe  = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y };

    float xOverlap = 0.0f;
    float yOverlap = 0.0f;

    // COLLISION ON RIGHT (moving right)
    if (map->isSolidTileAt(rightCentreProbe, &xOverlap, &yOverlap) 
         && mVelocity.x > 0.0f && yOverlap >= 0.5f)
    {
        mPosition.x -= xOverlap * 1.01f;   // push left
        mVelocity.x  = 0.0f;
        mIsCollidingRight = true;
    }

    // COLLISION ON LEFT (moving left)
    if (map->isSolidTileAt(leftCentreProbe, &xOverlap, &yOverlap) 
         && mVelocity.x < 0.0f && yOverlap >= 0.5f)
    {
        mPosition.x += xOverlap * 1.01;   // push right
        mVelocity.x  = 0.0f;
        mIsCollidingLeft = true;
    }
}

bool Entity::isColliding(Entity *other) const 
{
    if (!other->isActive() || other == this) return false;

    float xDistance = fabs(mPosition.x - other->getPosition().x) - 
        ((mColliderDimensions.x + other->getColliderDimensions().x) / 2.0f);
    float yDistance = fabs(mPosition.y - other->getPosition().y) - 
        ((mColliderDimensions.y + other->getColliderDimensions().y) / 2.0f);

    if (xDistance < 0.0f && yDistance < 0.0f) return true;

    return false;
}

void Entity::animate(float deltaTime)
{
    // Select animation based on current state
    // ✔ Special handling for EFFECT entities to play attack animations
    if (mEntityType == EFFECT || mIsEffect)
    {
        if (!mAttackAnimations.empty() && mEntityState == ATTACK)
        {
            mAnimationIndices = mAttackAnimations.at(ATTACK);
        }
        else if (!mWalkAnimations.empty())
        {
            // If no attack animation but has walk animation, use walk animation
            mAnimationIndices = mWalkAnimations.at(mDirection);
        }
        // If neither exists, keep current animation indices
        
        // DEBUG: Output for EFFECT entities with lifetime
        static int debugCounter = 0;
        if (mLifetime > 0.0f && debugCounter++ % 60 == 0)
        {
            // printf("[DEBUG] EFFECT animate: frame=%d, indices_size=%zu, walkAnim_size=%zu\n",
            //        mCurrentFrameIndex, mAnimationIndices.size(), mWalkAnimations.size());
        }
    }

    if(this->mEntityType != EFFECT) {
        switch (mEntityState)
        {
            case WALK:
                mAnimationIndices = mWalkAnimations.at(mDirection);
                break;
            case ATTACK:
                if (!mAttackAnimations.empty())
                    mAnimationIndices = mAttackAnimations.at(mEntityState);
                break;
            default:
                mAnimationIndices = mWalkAnimations.at(mDirection);
                break;
        }
    }

    if (mAnimationIndices.empty()) return;

    mAnimationTime += deltaTime;
    // mFrameSpeed is seconds per frame (e.g. 0.1 = 10fps)
    if (mAnimationTime >= mFrameSpeed)
    {
        mAnimationTime = 0.0f;

        mCurrentFrameIndex++;
        mCurrentFrameIndex %= mAnimationIndices.size();
        
        // If in ATTACK state and animation finished, return to WALK
        if (mEntityState == ATTACK && mCurrentFrameIndex == 0)
        {
            mEntityState = WALK;
        }
    }
}

void Entity::forceAnimationStart()
{
    if (!mAttackAnimations.empty())
    {
        mAnimationIndices = mAttackAnimations.at(ATTACK);
        mCurrentFrameIndex = 0;
        mAnimationTime = 0;
    }
}


void Entity::AIWander() { 
    // 确保mOriginalPos已初始化（如果为0，使用当前位置）
    if (mOriginalPos.x == 0.0f && mOriginalPos.y == 0.0f) {
        mOriginalPos = mPosition;
    }
    
    // 固定移动范围：100像素半径
    const float WANDER_RADIUS = 100.0f;
    float distFromOrigin = Vector2Distance(mPosition, mOriginalPos);
    
    // 如果超出范围，返回原始位置方向
    if (distFromOrigin > WANDER_RADIUS) {
        Vector2 dirToOrigin = {mOriginalPos.x - mPosition.x, mOriginalPos.y - mPosition.y};
        float len = sqrtf(dirToOrigin.x * dirToOrigin.x + dirToOrigin.y * dirToOrigin.y);
        if (len > 0.001f) {
            dirToOrigin.x /= len;
            dirToOrigin.y /= len;
            // 选择主要方向移动
            if (fabsf(dirToOrigin.x) > fabsf(dirToOrigin.y)) {
                if (dirToOrigin.x > 0) moveRight();
                else moveLeft();
            } else {
                if (dirToOrigin.y > 0) moveDown();
                else moveUp();
            }
        }
        return;
    }
    
    // 在范围内时，使用基于位置和时间的伪随机移动
    // 使用原始位置作为种子，确保每个Wanderer行为不同且稳定
    int seed = (int)(mOriginalPos.x * 7.0f + mOriginalPos.y * 11.0f);
    int baseDir = (abs(seed) / 10) % 4; // 0=LEFT, 1=UP, 2=RIGHT, 3=DOWN
    
    // 基于当前位置的伪随机数，每帧稳定但不同位置会有不同结果
    // 使用位置的小数部分来生成稳定的随机数
    int posHash = (int)(mPosition.x * 0.1f) + (int)(mPosition.y * 0.1f);
    int dirChoice = (abs(posHash + seed) / 5) % 4;
    
    // 如果当前没有移动或移动很小，随机选择一个方向
    float moveLen = sqrtf(mMovement.x * mMovement.x + mMovement.y * mMovement.y);
    if (moveLen < 0.1f) {
        dirChoice = GetRandomValue(0, 3);
    }
    
    // 每帧有5%概率改变方向，让移动更自然但不会太频繁
    if (GetRandomValue(0, 100) < 5) {
        dirChoice = GetRandomValue(0, 3);
    }
    
    switch(dirChoice) {
        case 0: moveLeft(); break;
        case 1: moveUp(); break;
        case 2: moveRight(); break;
        case 3: moveDown(); break;
    }
}

 
void Entity::AIFlyer() { 
    if (mPosition.y >= mOriginalPos.y + 165.0f) mDirection = UP;
    else if (mPosition.y <= mOriginalPos.y) mDirection = DOWN;
    if(mDirection == UP) moveUp();
    else if (mDirection == DOWN) moveDown();
 }

void Entity::AIFollow(Entity* target)
{
    if (target == nullptr) return;

    // 玩家当前位置
    Vector2 playerPos = target->getPosition();

    float dx = playerPos.x - mPosition.x;
    float dy = playerPos.y - mPosition.y;

    // 距离非常近时，停止移动，避免抖动
    float dist2 = dx * dx + dy * dy;
    if (dist2 < 1.0f) {
        mMovement.x = 0.0f;
        mMovement.y = 0.0f;
        return;
    }

    float dist = sqrtf(dist2);
    float nx = dx / dist;
    float ny = dy / dist;

    // 让 follower 永远朝玩家移动，不管多远
    mMovement.x = nx;
    mMovement.y = ny;

    // 更新朝向，用于选择动画方向（谁绝对值大就按谁）
    if (std::fabs(nx) >= std::fabs(ny)) {
        if (nx < 0) mDirection = LEFT;
        else        mDirection = RIGHT;
    } else {
        if (ny < 0) mDirection = UP;
        else        mDirection = DOWN;
    }

    // 如果你还想用 AIState，可以顺便标一下
    mAIState = FOLLOWING;
}


void Entity::AIActivate(Entity *target)
{
    switch (mAIType)
    {
    case WANDERER:
        AIWander();
        break;

    case FOLLOWER:
        AIFollow(target);
        break;
    
    case FLYER:
        AIFlyer();
        break;
    
    default:
        break;
    }
}

void Entity::takeDamage(int amount)
{
    if (mEntityStatus == INACTIVE) return;

    
    if (mEntityType == PLAYER)
    {
        // player i frame
        if (mInvincible) return;

        mCurrentHP -= amount;
        if (mCurrentHP < 0) mCurrentHP = 0;

        // player i frame
        mInvincible      = true;
        mInvincibleTimer = mInvincibleDuration;

        if (mCurrentHP <= 0)
        {
            mEntityStatus = INACTIVE;
        }

        return;
    }

    // ---- enemy... ----
    mCurrentHP -= amount;
    if (mCurrentHP < 0) mCurrentHP = 0;

    if (mCurrentHP <= 0)
    {
        mEntityStatus = INACTIVE;
    }
}


void Entity::attack()
{
    mEntityState = ATTACK;

    if (mIsEffect)
    {
        if (!mAttackAnimations.empty())
        {
            mAnimationIndices = mAttackAnimations[ATTACK];
            mCurrentFrameIndex = 0;
            mAnimationTime = 0;
        }
    }
}


bool Entity::isDead() const
{
    return mCurrentHP <= 0;
}

int Entity::getHP() const { return mCurrentHP; }
int Entity::getMaxHP() const { return mMaxHP; }

void Entity::setAttackInterval(float interval)
{
    mAttackInterval = interval;
    mAttackCooldown = interval;
}

void Entity::setLifetime(float seconds)
{
    mLifetime = seconds;
}

float Entity::getLifetime() const
{
    return mLifetime;
}

bool Entity::hasLifetime() const
{
    return (mLifetime > 0.0f);
}



void Entity::update(float deltaTime, Entity *player, Map *map, 
    std::vector<Entity*> collidableEntities)
{
    // --- Lifetime handling: decrement and deactivate when expired ---
    if (mLifetime > 0.0f)
    {
        mLifetime -= deltaTime;
        if (mLifetime <= 0.0f)
        {
            this->deactivate();
            return;
        }
    }

    // EFFECT type needs animation (Aura, projectiles, etc.)
    if (mIsEffect)
    {
        // For EFFECT type, always play animation
        if (mTextureType == ATLAS)
        {
            animate(deltaTime);
        }
        else
        {
            // printf("[DEBUG] EFFECT entity but mTextureType != ATLAS! textureType=%d\n", mTextureType);
        }
        
        // 更新位置（对于有movement的effect，如飞弹）
        // 需要先计算velocity，然后更新位置
        if (mLifetime > 0.0f || GetLength(mMovement) > 0.0f)
        {
            // 计算velocity（基于movement和speed）
            mVelocity.x = mMovement.x * mSpeed;
            mVelocity.y = mMovement.y * mSpeed;
            
            // 更新位置
            mPosition.x += mVelocity.x * deltaTime;
            mPosition.y += mVelocity.y * deltaTime;
        }
        return;
    }

    // std::cout << "colliding bottom " << mIsCollidingBottom << std::endl;
    if (mEntityStatus == INACTIVE) return;
    
    // if ( mIsCollidingBottom && mVelocity.y != 0) printf("bugbug\n");
    if (mEntityType == NPC) AIActivate(player);

    resetColliderFlags();

    mVelocity.x = mMovement.x * mSpeed;
    mVelocity.y = mMovement.y * mSpeed;
    
    mVelocity.x += mAcceleration.x * deltaTime;
    if (mAIType != FLYER) mVelocity.y += mAcceleration.y * deltaTime;
    else {
        if (mDirection == UP)
            mVelocity = {0, (float)(-mSpeed)};
        if (mDirection == DOWN)
            mVelocity = {0, (float)(mSpeed)};
    }

    if (mTextureType == ATLAS && mVelocity.x != 0) {
        if (mVelocity.x < 0) {
            setDirection(LEFT);
        } else {
            setDirection(RIGHT);
        }
    }
    if (mSpawnInvincible > 0.0f) {
        mSpawnInvincible -= deltaTime;
        return;
    }


    // ––––– JUMPING ––––– //
    if (mIsJumping)
    {
        // STEP 1: Immediately return the flag to its original false state
        mIsJumping = false;
        
        // STEP 2: The player now acquires an upward velocity
        mVelocity.y -= mJumpingPower;
    }

    // let character stay in ATTACK state for 0.5s
    if (mEntityState == ATTACK && mAttackTimer > 0.0f)
    {
        mAttackTimer -= deltaTime;
        if (mAttackTimer <= 0.0f)
        {
            mEntityState = WALK;
            mAttackTimer = 0.0f;
        }
    }
    
    mPosition.y += mVelocity.y * deltaTime;
    checkCollisionY(collidableEntities);

    if (mEntityType == BLOCK || mEntityType == PLATFORM) {
        checkCollisionY(map);
    }

    mPosition.x += mVelocity.x * deltaTime;
    checkCollisionX(collidableEntities);

    if (mEntityType == BLOCK || mEntityType == PLATFORM) {
        checkCollisionX(map);
    }


    // Animate
    if (mTextureType == ATLAS) {
        // 对于NPC类型，即使没有移动也要播放动画（Idle动画）
        if (mEntityType == NPC || !(GetLength(mMovement) == 0))
            animate(deltaTime);
    }

    // ---- MOVE TOWARD TARGET IF moveTo WAS SET ---- //
    if (moveSpeed > 0.0f)
    {
        Vector2 direction = {
            moveTarget.x - mPosition.x,
            moveTarget.y - mPosition.y
        };

        float distance = sqrt(direction.x*direction.x + direction.y*direction.y);

        if (distance < 1.0f)
        {
            moveSpeed = 0.0f;
            mVelocity = {0,0};
        }
        else
        {
            direction.x /= distance;
            direction.y /= distance;

            mPosition.x += direction.x * moveSpeed * deltaTime;
            mPosition.y += direction.y * moveSpeed * deltaTime;
        }
    }
    // Handle invincibility frames
    if (mInvincible)
    {
        mInvincibleTimer -= deltaTime;
        if (mInvincibleTimer <= 0.0f)
        {
            mInvincible = false;
            mInvincibleTimer = 0.0f;
        }
    }

    
    // Wanderer AI: 只在玩家靠近时（120像素内）才朝玩家移动，否则使用AIWander的随机移动
    if (mEntityType == NPC && mAIType == WANDERER && player != nullptr)
    {
        Vector2 playerPos = player->getPosition();
        float dx = playerPos.x - mPosition.x;
        float dy = playerPos.y - mPosition.y;
        float dist2 = dx * dx + dy * dy;
        const float ACTIVATION_DISTANCE = 120.0f * 120.0f; // 120像素的平方，避免开方运算
        
        // 只在玩家靠近时才朝玩家移动
        if (dist2 < ACTIVATION_DISTANCE)
        {
            // choose primary axis by larger absolute difference
            if (fabsf(dx) > fabsf(dy))
            {
                // move horizontally toward player
                if (dx > 0.0f)
                {
                    // move right
                    setMovement({1.0f, 0.0f});
                    setDirection(RIGHT);
                }
                else
                {
                    // move left
                    setMovement({-1.0f, 0.0f});
                    setDirection(LEFT);
                }
            }
            else
            {
                // move vertically toward player
                if (dy > 0.0f)
                {
                    // move down (y increases downward in raylib)
                    setMovement({0.0f, 1.0f});
                    setDirection(DOWN);
                }
                else
                {
                    // move up
                    setMovement({0.0f, -1.0f});
                    setDirection(UP);
                }
            }

            // ensure velocity follows movement & speed (so existing physics code continues to work)
            mVelocity.x = mMovement.x * mSpeed;
            mVelocity.y = mMovement.y * mSpeed;
        }
        // 如果玩家距离远，AIWander()已经设置了移动方向，这里不需要覆盖
    }


}

void Entity::render()
{
    // DEBUG: Check if laser is being skipped due to INACTIVE
    if (mEntityType == EFFECT && mIsEffect && mScale.x > 100.0f)
    {
        static int laserCheckCount = 0;
        if (laserCheckCount++ % 10 == 0)
        {
            // printf("[DEBUG] LASER CHECK: status=%d (0=ACTIVE, 1=INACTIVE) scale=(%.1f,%.1f)\n",
            //        mEntityStatus, mScale.x, mScale.y);
        }
    }
    
    if(mEntityStatus == INACTIVE) return;
    
    // DEBUG: Output specifically for Heaven Laser (scale.x > 100 indicates laser beam)
    if (mEntityType == EFFECT && mIsEffect && mScale.x > 100.0f)
    {
        static int laserRenderCount = 0;
        // printf("[DEBUG] LASER RENDER #%d: pos(%.1f,%.1f) scale(%.1f,%.1f) frame=%d angle=%.1f\n",
        //        laserRenderCount++, mPosition.x, mPosition.y, mScale.x, mScale.y, 
        //        mCurrentFrameIndex, mAngle);
    }

    Rectangle textureArea;

    switch (mTextureType)
    {
        case SINGLE:
            // Whole texture (UV coordinates)
            textureArea = {
                // top-left corner
                0.0f, 0.0f,

                // bottom-right corner (of texture)
                static_cast<float>(mTexture.width),
                static_cast<float>(mTexture.height)
            };
            break;
        case ATLAS:
        {
            if (mAnimationIndices.size() == 0) break;

            if (mCurrentFrameIndex >= mAnimationIndices.size())
                mCurrentFrameIndex = 0;

            int frameNumber = mAnimationIndices[mCurrentFrameIndex];

            int maxFrame = mSpriteSheetDimensions.x * mSpriteSheetDimensions.y;
            if (frameNumber >= maxFrame)
                frameNumber = maxFrame - 1;

            textureArea = getUVRectangle(
                &mTexture,
                frameNumber,
                mSpriteSheetDimensions.x,
                mSpriteSheetDimensions.y
            );
            break;
        }
    }

    // Flip texture horizontally when facing left
    if (mDirection == LEFT && textureArea.width > 0) {
        textureArea.width = -textureArea.width;
    }

    // Destination rectangle – centred on gPosition
    Rectangle destinationArea = {
        mPosition.x,
        mPosition.y,
        static_cast<float>(mScale.x),
        static_cast<float>(mScale.y)
    };

    // Origin inside the source texture (centre of the texture)
    Vector2 originOffset = {
        static_cast<float>(mScale.x) / 2.0f,
        static_cast<float>(mScale.y) / 2.0f
    };

    if (mEntityType == NPC)
    {
        float hpRatio = (float)mCurrentHP / mMaxHP;

        DrawRectangle(
            mPosition.x - 15,
            mPosition.y - mScale.y/2 - 10,
            30 * hpRatio,
            4,
            RED
        );
    }


    // Render the texture on screen
    DrawTexturePro(
        mTexture, 
        textureArea, destinationArea, originOffset,
        mAngle, WHITE
    );

    // displayCollider();
}

void Entity::renderWithTint(Color tint)
{
    if(mEntityStatus == INACTIVE) return;

    Rectangle textureArea;

    switch (mTextureType)
    {
        case SINGLE:
            textureArea = {
                0.0f, 0.0f,
                static_cast<float>(mTexture.width),
                static_cast<float>(mTexture.height)
            };
            break;
        case ATLAS:
        {
            if (mAnimationIndices.size() == 0) break;

            if (mCurrentFrameIndex >= mAnimationIndices.size())
                mCurrentFrameIndex = 0;

            int frameNumber = mAnimationIndices[mCurrentFrameIndex];
            int maxFrame = mSpriteSheetDimensions.x * mSpriteSheetDimensions.y;
            if (frameNumber >= maxFrame)
                frameNumber = maxFrame - 1;

            textureArea = getUVRectangle(
                &mTexture,
                frameNumber,
                mSpriteSheetDimensions.x,
                mSpriteSheetDimensions.y
            );
            break;
        }
    }

    if (mDirection == LEFT && textureArea.width > 0) {
        textureArea.width = -textureArea.width;
    }

    Rectangle destinationArea = {
        mPosition.x,
        mPosition.y,
        static_cast<float>(mScale.x),
        static_cast<float>(mScale.y)
    };

    Vector2 originOffset = {
        static_cast<float>(mScale.x) / 2.0f,
        static_cast<float>(mScale.y) / 2.0f
    };

    // Render with custom tint
    DrawTexturePro(
        mTexture, 
        textureArea, destinationArea, originOffset,
        mAngle, tint
    );
}

void Entity::displayCollider() 
{
    // draw the collision box
    // Rectangle colliderBox = {
    //     mPosition.x - mColliderDimensions.x / 2.0f,  
    //     mPosition.y - mColliderDimensions.y / 2.0f,  
    //     mColliderDimensions.x,                        
    //     mColliderDimensions.y                        
    // };

    // DrawRectangleLines(
    //     colliderBox.x,      // Top-left X
    //     colliderBox.y,      // Top-left Y
    //     colliderBox.width,  // Width
    //     colliderBox.height, // Height
    //     GREEN               // Color
    // );
}

void Entity::moveTo(Vector2 target, float speed){
    moveTarget = target;
    moveSpeed = speed;
}