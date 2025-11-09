#include "Entity.h"

Entity::Entity() : mPosition {0.0f, 0.0f}, mMovement {0.0f, 0.0f}, 
                   mVelocity {0.0f, 0.0f}, mAcceleration {0.0f, 0.0f},
                   mScale {DEFAULT_SIZE, DEFAULT_SIZE},
                   mColliderDimensions {DEFAULT_SIZE, DEFAULT_SIZE}, 
                   mTexture {NULL}, mTextureType {SINGLE}, mAngle {0.0f},
                   mSpriteSheetDimensions {}, mDirection {RIGHT}, 
                   mWalkAnimations {{}}, mAnimationIndices {}, mFrameSpeed {0},
                   mEntityType {NONE},mIsCollidingBottom(false) { }

Entity::Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
    EntityType entityType) : mPosition {position}, mVelocity {0.0f, 0.0f}, 
    mAcceleration {0.0f, 0.0f}, mScale {scale}, mMovement {0.0f, 0.0f}, 
    mColliderDimensions {scale}, mTexture {LoadTexture(textureFilepath)}, 
    mTextureType {SINGLE}, mDirection {RIGHT}, mWalkAnimations {{}}, 
    mAnimationIndices {}, mFrameSpeed {0}, mSpeed {DEFAULT_SPEED}, 
    mAngle {0.0f}, mEntityType {entityType}, mOriginalPos(position),
    mIsCollidingBottom(false) { }

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
        mIsCollidingBottom(false) { }

Entity::~Entity() { UnloadTexture(mTexture); };

void Entity::checkCollisionY(std::vector<Entity*> collidableEntities)
{
    for (int i = 0; i < collidableEntities.size(); i++)
    {
        // STEP 1: For every entity that our player can collide with...
        Entity *collidableEntity = collidableEntities[i];
        
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

    mAnimationTime += deltaTime;
    float framesPerSecond = 1.0f / mFrameSpeed;

    if (mAnimationTime >= framesPerSecond)
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

void Entity::AIWander() { 

    if (mPosition.x >= mOriginalPos.x + 150.0f) {moveLeft();}
    else if (mPosition.x <= mOriginalPos.x) moveRight();
    else if (mDirection == LEFT) moveLeft();
    else if (mDirection == RIGHT) moveRight();
 }

 
void Entity::AIFlyer() { 
    if (mPosition.y >= mOriginalPos.y + 165.0f) mDirection = UP;
    else if (mPosition.y <= mOriginalPos.y) mDirection = DOWN;
 }

void Entity::AIFollow(Entity *target)
{
    switch (mAIState)
    {
    case IDLE:
        if (Vector2Distance(mPosition, target->getPosition()) < 250.0f) 
            mAIState = WALKING;
        break;

    case WALKING:
        // Depending on where the player is in respect to their x-position
        // Change direction of the enemy
        if (mPosition.x > target->getPosition().x) moveLeft();
        else                                       moveRight();
        if (mPosition.y > target->getPosition().y) moveDown();
        else                                       moveUp();
    
    default:
        break;
    }
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

void Entity::update(float deltaTime, Entity *player, Map *map, 
    std::vector<Entity*> collidableEntities)
{
    // std::cout << "colliding bottom " << mIsCollidingBottom << std::endl;
    if (mEntityStatus == INACTIVE) return;
    
    // if ( mIsCollidingBottom && mVelocity.y != 0) printf("bugbug\n");
    if (mEntityType == NPC) AIActivate(player);

    resetColliderFlags();

    mVelocity.x = mMovement.x * mSpeed;
    
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
    checkCollisionY(map);

    mPosition.x += mVelocity.x * deltaTime;
    checkCollisionX(collidableEntities);
    checkCollisionX(map);

    // Animate if: walking on ground OR attacking (regardless of ground/movement)
    if (mTextureType == ATLAS) {
        if ((GetLength(mMovement) != 0 && mIsCollidingBottom) || mEntityState == ATTACK || mAIType == FLYER)
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
}

void Entity::render()
{
    if(mEntityStatus == INACTIVE) return;

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
            textureArea = getUVRectangle(
                &mTexture, 
                mAnimationIndices[mCurrentFrameIndex], 
                mSpriteSheetDimensions.x, 
                mSpriteSheetDimensions.y
            );
        
        default: break;
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

    // Render the texture on screen
    DrawTexturePro(
        mTexture, 
        textureArea, destinationArea, originOffset,
        mAngle, WHITE
    );

    displayCollider();
}

void Entity::displayCollider() 
{
    // draw the collision box
    Rectangle colliderBox = {
        mPosition.x - mColliderDimensions.x / 2.0f,  
        mPosition.y - mColliderDimensions.y / 2.0f,  
        mColliderDimensions.x,                        
        mColliderDimensions.y                        
    };

    DrawRectangleLines(
        colliderBox.x,      // Top-left X
        colliderBox.y,      // Top-left Y
        colliderBox.width,  // Width
        colliderBox.height, // Height
        GREEN               // Color
    );
}

void Entity::moveTo(Vector2 target, float speed){
    moveTarget = target;
    moveSpeed = speed;
}