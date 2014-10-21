/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*TODO list:
 * - replace hardcoded calculations by scripts and/or read the numbers from XML defintion files
 * - the doUpkeep() functions needs script support
 */

#include "entities/Creature.h"

#include "entities/CreatureAction.h"
#include "entities/Weapon.h"
#include "entities/CreatureSound.h"
#include "entities/TreasuryObject.h"
#include "entities/ChickenEntity.h"

#include "camera/CullingQuad.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "network/ODClient.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"

#include "render/RenderRequest.h"
#include "render/RenderManager.h"

#include "rooms/RoomTreasury.h"
#include "rooms/RoomDormitory.h"

#include "sound/SoundEffectsManager.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"


#include <CEGUI/System.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/Window.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/Event.h>
#include <CEGUI/UDim.h>
#include <CEGUI/Vector.h>

#include <OgreQuaternion.h>
#include <OgreVector3.h>
#include <OgreVector2.h>

#include <cmath>
#include <algorithm>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

#define NB_COUNTER_DEATH        10

static const int MAX_LEVEL = 30;
//TODO: make this read from definition file?
static const int MaxGoldCarriedByWorkers = 1500;
static const int NB_TURN_FLEE_MAX = 5;

const std::string Creature::CREATURE_PREFIX = "Creature_";

Creature::Creature(GameMap* gameMap, CreatureDefinition* definition) :
    MovableGameEntity        (gameMap),
    mTracingCullingQuad      (NULL),
    mPhysicalAttack          (1.0),
    mMagicalAttack           (0.0),
    mPhysicalDefense         (3.0),
    mMagicalDefense          (1.5),
    mWeaponlessAtkRange      (1.0),
    mAttackWarmupTime        (1.0),
    mWeaponL                 (NULL),
    mWeaponR                 (NULL),
    mHomeTile                (NULL),
    mDefinition              (definition),
    mIsOnMap                 (false),
    mHasVisualDebuggingEntities (false),
    mAwakeness               (100.0),
    mHunger                  (0.0),
    mLevel                   (1),
    mHp                      (10.0),
    mMaxHP                   (10.0),
    mExp                     (0.0),
    mGroundSpeed             (1.0),
    mWaterSpeed              (0.0),
    mLavaSpeed               (0.0),
    mDigRate                 (0.0),
    mClaimRate               (0.0),
    mDeathCounter            (NB_COUNTER_DEATH),
    mGold                    (0),
    mJobCooldown             (0),
    mEatCooldown             (0),
    mNbTurnAction            (0),
    mPreviousPositionTile    (NULL),
    mJobRoom                 (NULL),
    mEatRoom                 (NULL),
    mStatsWindow             (NULL),
    mAttackedTile            (NULL),
    mAttackedObject          (NULL),
    mSound                   (SoundEffectsManager::getSingleton().getCreatureClassSounds(definition->getClassName())),
    mForceAction             (forcedActionNone)
{
    setName(getGameMap()->nextUniqueNameCreature(definition->getClassName()));

    setIsOnMap(false);

    setObjectType(GameEntity::creature);

    pushAction(CreatureAction::idle);

    // Note: We reset the creature to level 1 in that case.
    setLevel(1);
    mExp = 0.0;

    mMaxHP = mDefinition->getMinHp();
    setHP(mMaxHP);

    mGroundSpeed = mDefinition->getMoveSpeedGround();
    mWaterSpeed = mDefinition->getMoveSpeedWater();
    mLavaSpeed = mDefinition->getMoveSpeedLava();

    mDigRate = mDefinition->getDigRate();
    mClaimRate = mDefinition->getClaimRate();

    // Fighting stats
    mPhysicalAttack = mDefinition->getPhysicalAttack();
    mMagicalAttack = mDefinition->getMagicalAttack();
    mPhysicalDefense = mDefinition->getPhysicalDefense();
    mMagicalDefense = mDefinition->getMagicalDefense();
    mWeaponlessAtkRange = mDefinition->getAttackRange();
    mAttackWarmupTime = mDefinition->getAttackWarmupTime();
}

Creature::Creature(GameMap* gameMap) :
    MovableGameEntity        (gameMap),
    mTracingCullingQuad      (NULL),
    mPhysicalAttack          (1.0),
    mMagicalAttack           (0.0),
    mPhysicalDefense         (3.0),
    mMagicalDefense          (1.5),
    mWeaponlessAtkRange      (1.0),
    mAttackWarmupTime        (1.0),
    mWeaponL                 (NULL),
    mWeaponR                 (NULL),
    mHomeTile                (NULL),
    mDefinition              (NULL),
    mIsOnMap                 (false),
    mHasVisualDebuggingEntities (false),
    mAwakeness               (100.0),
    mHunger                  (0.0),
    mLevel                   (1),
    mHp                      (10.0),
    mMaxHP                   (10.0),
    mExp                     (0.0),
    mGroundSpeed             (1.0),
    mWaterSpeed              (0.0),
    mLavaSpeed               (0.0),
    mDigRate                 (0.0),
    mClaimRate               (0.0),
    mDeathCounter            (NB_COUNTER_DEATH),
    mGold                    (0),
    mJobCooldown             (0),
    mEatCooldown             (0),
    mNbTurnAction            (0),
    mPreviousPositionTile    (NULL),
    mJobRoom                 (NULL),
    mEatRoom                 (NULL),
    mStatsWindow             (NULL),
    mAttackedTile            (NULL),
    mAttackedObject          (NULL),
    mSound                   (NULL),
    mForceAction             (forcedActionNone)
{
    setIsOnMap(false);

    setObjectType(GameEntity::creature);

    pushAction(CreatureAction::idle);

    // Note: We reset the creature to level 1 in that case.
    setLevel(1);
    mExp = 0.0;
}

/* Destructor is needed when removing from Quadtree */
Creature::~Creature()
{
    if(mTracingCullingQuad != NULL)
    {
        mTracingCullingQuad->entry->creature_list.remove(this);
        mTracingCullingQuad->mortuaryInsert(this);
    }

    if(mWeaponL != nullptr)
        delete mWeaponL;
    if(mWeaponR != nullptr)
        delete mWeaponR;
}

void Creature::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();
    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequestCreateCreature(this);
    RenderManager::queueRenderRequest(request);

    // By default, we set the creature in idle state
    request = new RenderRequestSetObjectAnimationState(this, "Idle", true);
    RenderManager::queueRenderRequest(request);
}

void Creature::destroyMeshLocal()
{
    MovableGameEntity::destroyMeshLocal();
    getWeaponL()->destroyMesh();
    getWeaponR()->destroyMesh();
    if(getGameMap()->isServerGameMap())
        return;

    destroyStatsWindow();
    RenderRequest* request = new RenderRequestDestroyCreature(this);
    RenderManager::queueRenderRequest(request);
}

void Creature::deleteYourselfLocal()
{
    MovableGameEntity::deleteYourselfLocal();
    // If standing on a valid tile, notify that tile we are no longer there.
    if(getPositionTile() != 0)
        getPositionTile()->removeCreature(this);

    if(mWeaponL != nullptr)
        mWeaponL->deleteYourself();
    mWeaponL = nullptr;

    if(mWeaponR != nullptr)
        mWeaponR->deleteYourself();
    mWeaponR = nullptr;
}

std::string Creature::getFormat()
{
    //NOTE:  When this format changes, other changes to RoomPortal::spawnCreature() may be necessary.
    return "SeatId\tClassName\tName\tLevel\tCurrentXP\tCurrenHP\tCurrentAwakeness\t"
           "CurrentHunger\tGoldToDeposit\tPosX\tPosY\tPosZ\tLeftWeapon "
           + Weapon::getFormat() +"\tRightWeapon " + Weapon::getFormat();
}

void Creature::exportToStream(std::ostream& os)
{
    int seatId = getSeat()->getId();
    os << seatId << "\t";
    os << mDefinition->getClassName() << "\t" << getName() << "\t";
    os << getLevel() << "\t" << mExp << "\t" << getHP() << "\t";
    os << mAwakeness << "\t" << mHunger << "\t" << mGold << "\t";

    os << getPosition().x << "\t";
    os << getPosition().y << "\t";
    os << getPosition().z << "\t";

    // Check creature weapons
    Weapon* wL = mWeaponL;
    if (wL == NULL)
        wL = new Weapon(getGameMap(), "none", 0.0, 1.0, 0.0, "L", this);
    Weapon* wR = mWeaponR;
    if (wR == NULL)
        wR = new Weapon(getGameMap(), "none", 0.0, 1.0, 0.0, "R", this);

    os << wL << "\t" << wR << "\t";

    // If we had to create dummy weapons for serialization, delete them now.
    if (mWeaponL == NULL)
        delete wL;
    if (mWeaponR == NULL)
        delete wR;
}

void Creature::importFromStream(std::istream& is)
{
    double xLocation = 0.0, yLocation = 0.0, zLocation = 0.0;
    double tempDouble = 0.0;
    std::string tempString;

    int seatId = 0;
    OD_ASSERT_TRUE(is >> seatId);
    Seat* seat = getGameMap()->getSeatById(seatId);
    setSeat(seat);

    // class name
    OD_ASSERT_TRUE(is >> tempString);
    mDefinition = getGameMap()->getClassDescription(tempString);
    OD_ASSERT_TRUE_MSG(mDefinition != nullptr, "Definition=" + tempString);
    mSound = SoundEffectsManager::getSingleton().getCreatureClassSounds(tempString);

    // name
    OD_ASSERT_TRUE(is >> tempString);
    if (tempString.compare("autoname") == 0)
        tempString = getGameMap()->nextUniqueNameCreature(mDefinition->getClassName());
    setName(tempString);

    OD_ASSERT_TRUE(is >> tempDouble);
    buildStats(tempDouble);

    OD_ASSERT_TRUE(is >> tempDouble);
    mExp = tempDouble;

    OD_ASSERT_TRUE(is >> tempDouble);
    setHP(tempDouble);

    OD_ASSERT_TRUE(is >> tempDouble);
    mAwakeness = tempDouble;

    OD_ASSERT_TRUE(is >> tempDouble);
    mHunger = tempDouble;

    OD_ASSERT_TRUE(is >> tempDouble);
    mGold = static_cast<int>(tempDouble);

    OD_ASSERT_TRUE(is >> xLocation >> yLocation >> zLocation);
    setPosition(Ogre::Vector3((Ogre::Real)xLocation, (Ogre::Real)yLocation, (Ogre::Real)zLocation));

    // TODO: Load weapon from a catalog file.
    if (mWeaponL == nullptr)
        setWeaponL(new Weapon(getGameMap(), std::string(), 0.0, 0.0, 0.0, std::string()));
    OD_ASSERT_TRUE(is >> mWeaponL);

    if (mWeaponR == nullptr)
        setWeaponR(new Weapon(getGameMap(), std::string(), 0.0, 0.0, 0.0, std::string()));
    OD_ASSERT_TRUE(is >> mWeaponR);
}

void Creature::buildStats(unsigned int level)
{
    if (level > MAX_LEVEL)
        return;

    // Get the base value
    mMaxHP = mDefinition->getMinHp();
    mDigRate = mDefinition->getDigRate();
    mClaimRate = mDefinition->getClaimRate();
    mGroundSpeed = mDefinition->getMoveSpeedGround();
    mWaterSpeed = mDefinition->getMoveSpeedWater();
    mLavaSpeed  = mDefinition->getMoveSpeedLava();

    mPhysicalAttack = mDefinition->getPhysicalAttack();
    mMagicalAttack = mDefinition->getMagicalAttack();
    mPhysicalDefense = mDefinition->getPhysicalDefense();
    mMagicalDefense = mDefinition->getMagicalDefense();
    mWeaponlessAtkRange = mDefinition->getAttackRange();
    mAttackWarmupTime = mDefinition->getAttackWarmupTime();

    // Improve the stats to the current level
    double multiplier = level - 1;
    if (multiplier == 0.0)
        return;

    mMaxHP += mDefinition->getHpPerLevel() * multiplier;
    mDigRate += mDefinition->getDigRatePerLevel() * multiplier;
    mClaimRate += mDefinition->getClaimRatePerLevel() * multiplier;
    mGroundSpeed += mDefinition->getGroundSpeedPerLevel() * multiplier;
    mWaterSpeed += mDefinition->getWaterSpeedPerLevel() * multiplier;
    mLavaSpeed += mDefinition->getLavaSpeedPerLevel() * multiplier;

    mPhysicalAttack += mDefinition->getPhysicalAtkPerLevel() * multiplier;
    mMagicalAttack += mDefinition->getMagicalAtkPerLevel() * multiplier;
    mPhysicalDefense += mDefinition->getPhysicalDefPerLevel() * multiplier;
    mMagicalDefense += mDefinition->getMagicalDefPerLevel() * multiplier;
    mWeaponlessAtkRange += mDefinition->getAtkRangePerLevel() * multiplier;

    setLevel(level);
}

Creature* Creature::getCreatureFromStream(GameMap* gameMap, std::istream& is)
{
    Creature* creature = new Creature(gameMap);
    creature->importFromStream(is);
    return creature;
}

Creature* Creature::getCreatureFromPacket(GameMap* gameMap, ODPacket& is)
{
    Creature* creature = new Creature(gameMap);
    creature->importFromPacket(is);
    return creature;
}

void Creature::exportToPacket(ODPacket& os)
{
    std::string className = mDefinition->getClassName();
    os << className;

    std::string name = getName();
    os << name;

    Ogre::Vector3 position = getPosition();
    int seatId = getSeat()->getId();
    os << position;
    os << seatId;

    os << mLevel;
    os << mExp;

    os << mHp;
    os << mMaxHP;

    os << mDigRate;
    os << mClaimRate;
    os << mAwakeness;
    os << mHunger;

    os << mGroundSpeed;
    os << mWaterSpeed;
    os << mLavaSpeed;

    os << mPhysicalAttack;
    os << mMagicalAttack;
    os << mPhysicalDefense;
    os << mMagicalDefense;
    os << mWeaponlessAtkRange;

    // Check creature weapons
    Weapon* wL = mWeaponL;
    if (wL == NULL)
        wL = new Weapon(getGameMap(), "none", 0.0, 1.0, 0.0, "L", this);
    Weapon* wR = mWeaponR;
    if (wR == NULL)
        wR = new Weapon(getGameMap(), "none", 0.0, 1.0, 0.0, "R", this);

    os << wL << wR;

    // If we had to create dummy weapons for serialization, delete them now.
    if (mWeaponL == NULL)
        delete wL;
    if (mWeaponR == NULL)
        delete wR;
}

void Creature::importFromPacket(ODPacket& is)
{
    std::string tempString;

    OD_ASSERT_TRUE(is >> tempString);
    mDefinition = getGameMap()->getClassDescription(tempString);
    OD_ASSERT_TRUE_MSG(mDefinition != nullptr, "Definition=" + tempString);

    mMaxHP = mDefinition->getMinHp();
    setHP(mMaxHP);
    mSound = SoundEffectsManager::getSingleton().getCreatureClassSounds(tempString);

    OD_ASSERT_TRUE(is >> tempString);
    setName(tempString);

    Ogre::Vector3 position;
    OD_ASSERT_TRUE(is >> position);
    setPosition(position);

    int seatId;
    OD_ASSERT_TRUE(is >> seatId);
    Seat* seat = getGameMap()->getSeatById(seatId);
    setSeat(seat);

    OD_ASSERT_TRUE(is >> mLevel);
    OD_ASSERT_TRUE(is >> mExp);

    OD_ASSERT_TRUE(is >> mHp);
    OD_ASSERT_TRUE(is >> mMaxHP);

    OD_ASSERT_TRUE(is >> mDigRate);
    OD_ASSERT_TRUE(is >> mClaimRate);
    OD_ASSERT_TRUE(is >> mAwakeness);
    OD_ASSERT_TRUE(is >> mHunger);

    OD_ASSERT_TRUE(is >> mGroundSpeed);
    OD_ASSERT_TRUE(is >> mWaterSpeed);
    OD_ASSERT_TRUE(is >> mLavaSpeed);

    OD_ASSERT_TRUE(is >> mPhysicalAttack);
    OD_ASSERT_TRUE(is >> mMagicalAttack);
    OD_ASSERT_TRUE(is >> mPhysicalDefense);
    OD_ASSERT_TRUE(is >> mMagicalDefense);
    OD_ASSERT_TRUE(is >> mWeaponlessAtkRange);

    // TODO: Load weapon from a catalog file.
    setWeaponL(new Weapon(getGameMap(), std::string(), 0.0, 0.0, 0.0, std::string()));
    OD_ASSERT_TRUE(is >> mWeaponL);

    setWeaponR(new Weapon(getGameMap(), std::string(), 0.0, 0.0, 0.0, std::string()));
    OD_ASSERT_TRUE(is >> mWeaponR);
}

void Creature::setPosition(const Ogre::Vector3& v)
{
    // If we are on the gameMap we may need to update the tile we are in
    if (getIsOnMap())
    {
        // We are on the map
        // Move the creature relative to its parent scene node.  We record the
        // tile the creature is in before and after the move to properly
        // maintain the results returned by the getPositionTile() function.
        Tile *oldPositionTile = getPositionTile();

        MovableGameEntity::setPosition(v);
        Tile *newPositionTile = getPositionTile();

        if (oldPositionTile != newPositionTile)
        {
            if (oldPositionTile != 0)
                oldPositionTile->removeCreature(this);

            if (newPositionTile != 0)
                newPositionTile->addCreature(this);
        }

        if(!getGameMap()->isServerGameMap())
            mTracingCullingQuad->moveEntryDelta(this,get2dPosition());
    }
    else
    {
        // We are not on the map
        MovableGameEntity::setPosition(v);
    }
}

void Creature::drop(const Ogre::Vector3& v)
{
    setIsOnMap(true);
    setPosition(v);
    mForceAction = forcedActionSearchAction;
}

void Creature::setHP(double nHP)
{
    if (nHP > mMaxHP)
        mHp = mMaxHP;
    else
        mHp = nHP;
}

double Creature::getHP() const
{
    return mHp;
}

void Creature::setIsOnMap(bool nIsOnMap)
{
    mIsOnMap = nIsOnMap;
}

bool Creature::getIsOnMap() const
{
    return mIsOnMap;
}

void Creature::setWeaponL(Weapon* wL)
{
    if (mWeaponL)
        delete mWeaponL;
    mWeaponL = wL;
    if (!mWeaponL)
        return;

    mWeaponL->setParentCreature(this);
    mWeaponL->setHandString("L");
}

void Creature::setWeaponR(Weapon* wR)
{
    if (mWeaponR)
        delete mWeaponR;
    mWeaponR = wR;
    if (!mWeaponR)
        return;

    mWeaponR->setParentCreature(this);
    mWeaponR->setHandString("R");
}

void Creature::update(Ogre::Real timeSinceLastFrame)
{
    // Update movements, direction, ...
    MovableGameEntity::update(timeSinceLastFrame);

    if (getGameMap()->isServerGameMap())
    {
        // Reduce the attack warmup time left for creatures on the server side
        // When they are attacking
        if (mAttackWarmupTime > 0.0)
            mAttackWarmupTime -= timeSinceLastFrame;
    }
}

void Creature::doUpkeep()
{
    // if creature is not on map, we do nothing
    if(!getIsOnMap())
        return;

    // Check if the creature is alive
    if (getHP() <= 0.0)
    {
        // Let the creature lay dead on the ground for a few turns before removing it from the GameMap.
        if (mDeathCounter == NB_COUNTER_DEATH)
        {
            stopJob();
            stopEating();
            clearDestinations();
            setAnimationState("Die", false);
        }
        else if (mDeathCounter <= 0)
        {
            try
            {
                const std::string& name = getName();
                Player* player = getGameMap()->getPlayerBySeat(getSeat());
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotification::removeCreature, player);
                serverNotification->mPacket << name;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
            catch (std::bad_alloc&)
            {
                OD_ASSERT_TRUE(false);
                exit(1);
            }

            // If the creature has a homeTile where it sleeps, its bed needs to be destroyed.
            if (getHomeTile() != 0)
            {
                RoomDormitory* home = static_cast<RoomDormitory*>(getHomeTile()->getCoveringRoom());
                home->releaseTileForSleeping(getHomeTile(), this);
            }

            // Remove the creature from the game map and into the deletion queue, it will be deleted
            // when it is safe, i.e. all other pointers to it have been wiped from the program.
            LogManager::getSingleton().logMessage("Removing creature " + getName());
            getGameMap()->removeCreature(this);
            deleteYourself();
        }

        --mDeathCounter;
        return;
    }

    // If we are not standing somewhere on the map, do nothing.
    if (getPositionTile() == NULL)
        return;

    // Check to see if we have earned enough experience to level up.
    if(checkLevelUp())
    {
        setLevel(getLevel() + 1);

        // Reset XP once the level has been acquired.
        mExp = 0.0;
        //std::cout << "\n\n" << getName() << " has reached level " << getLevel() << "\n";

        if (mDefinition->isWorker())
        {
            mDigRate += mDefinition->getDigRatePerLevel();
            mClaimRate += mDefinition->getClaimRatePerLevel();
            //std::cout << "New dig rate: " << mDigRate << "\tnew dance rate: " << mDanceRate << "\n";
        }

        // Improve the base stats
        mMaxHP += mDefinition->getHpPerLevel();
        mGroundSpeed += mDefinition->getGroundSpeedPerLevel();
        mWaterSpeed += mDefinition->getWaterSpeedPerLevel();
        mLavaSpeed += mDefinition->getLavaSpeedPerLevel();

        // Improve fighting stats
        mPhysicalAttack += mDefinition->getPhysicalAtkPerLevel();
        mMagicalAttack += mDefinition->getMagicalAtkPerLevel();
        mPhysicalDefense += mDefinition->getPhysicalDefPerLevel();
        mMagicalDefense += mDefinition->getMagicalDefPerLevel();
        mWeaponlessAtkRange += mDefinition->getAtkRangePerLevel();

        if(getGameMap()->isServerGameMap())
        {
            try
            {
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotification::creatureRefresh, getGameMap()->getPlayerBySeat(getSeat()));
                exportToPacket(serverNotification->mPacket);
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
            catch (std::bad_alloc&)
            {
                OD_ASSERT_TRUE(false);
                exit(1);
            }
        }
    }

    // TODO : this is auto heal. It could be done on client side
    // Heal.
    mHp += mDefinition->getHpHealPerTurn();
    if (mHp > getMaxHp())
        mHp = getMaxHp();

    mAwakeness -= mDefinition->getAwakenessLostPerTurn();
    if (mAwakeness < 0.0)
        mAwakeness = 0.0;

    mHunger += mDefinition->getHungerGrowthPerTurn();
    if (mHunger > 100.0)
        mHunger = 100.0;

    // Look at the surrounding area
    updateVisibleTiles();
    mVisibleEnemyObjects         = getVisibleEnemyObjects();
    mReachableEnemyObjects       = getReachableAttackableObjects(mVisibleEnemyObjects);
    mReachableEnemyCreatures     = getCreaturesFromList(mReachableEnemyObjects, getDefinition()->isWorker());
    mVisibleAlliedObjects        = getVisibleAlliedObjects();
    mReachableAlliedObjects      = getReachableAttackableObjects(mVisibleAlliedObjects);

    std::vector<Tile*> markedTiles;

    if (mDigRate > 0.0)
        markedTiles = getVisibleMarkedTiles();

    decidePrioritaryAction();

    // The loopback variable allows creatures to begin processing a new
    // action immediately after some other action happens.
    bool loopBack = false;
    unsigned int loops = 0;
    ++mNbTurnAction;

    do
    {
        ++loops;
        loopBack = false;

        // Carry out the current task
        if (!mActionQueue.empty())
        {
            CreatureAction topActionItem = peekAction();
            switch (topActionItem.getType())
            {
                case CreatureAction::idle:
                    loopBack = handleIdleAction();
                    break;

                case CreatureAction::walkToTile:
                    loopBack = handleWalkToTileAction();
                    break;

                case CreatureAction::claimTile:
                    loopBack = handleClaimTileAction();
                    break;

                case CreatureAction::claimWallTile:
                    loopBack = handleClaimWallTileAction();
                    break;

                case CreatureAction::digTile:
                    loopBack = handleDigTileAction();
                    break;

                case CreatureAction::depositGold:
                    loopBack = handleDepositGoldAction();
                    break;

                case CreatureAction::findHome:
                    loopBack = handleFindHomeAction(false);
                    break;

                case CreatureAction::findHomeForced:
                    loopBack = handleFindHomeAction(true);
                    break;

                case CreatureAction::sleep:
                    loopBack = handleSleepAction();
                    break;

                case CreatureAction::jobdecided:
                    loopBack = handleJobAction(false);
                    break;

                case CreatureAction::jobforced:
                    loopBack = handleJobAction(true);
                    break;

                case CreatureAction::eatdecided:
                    loopBack = handleEatingAction(false);
                    break;

                case CreatureAction::eatforced:
                    loopBack = handleEatingAction(true);
                    break;

                case CreatureAction::attackObject:
                    loopBack = handleAttackAction();
                    break;

                case CreatureAction::fight:
                    loopBack = handleFightAction();
                    break;

                case CreatureAction::flee:
                    loopBack = handleFleeAction();
                    break;

                default:
                    LogManager::getSingleton().logMessage("ERROR:  Unhandled action type in Creature::doUpkeep():"
                        + Ogre::StringConverter::toString(topActionItem.getType()));
                    popAction();
                    loopBack = false;
                    break;
            }
        }
        else
        {
            LogManager::getSingleton().logMessage("ERROR:  Creature has empty action queue in doUpkeep(), this should not happen.");
            loopBack = false;
        }
    } while (loopBack && loops < 20);

    if(loops >= 20)
    {
        LogManager::getSingleton().logMessage("> 20 loops in Creature::doUpkeep name:" + getName() +
                " seat id: " + Ogre::StringConverter::toString(getSeat()->getId()) + ". Breaking out..");
    }

    // Update the visual debugging entities
    //if we are standing in a different tile than we were last turn
    if (mHasVisualDebuggingEntities && getPositionTile() != mPreviousPositionTile)
    {
        //TODO: This destroy and re-create is kind of a hack as its likely only a few
        //tiles will actually change.
        destroyVisualDebugEntities();
        createVisualDebugEntities();
    }
}

void Creature::decidePrioritaryAction()
{
    // Here, we should decide prioritary actions only (like fighting when an ennemy is
    // visible). And if we decide to do something, we should clear the action queue.

    // If a creature is weak and there are foes, it shall flee
    bool isWeak = (mHp < mMaxHP / 3);
    if (!mReachableEnemyObjects.empty() && isWeak)
    {
        if(isActionInList(CreatureAction::flee))
            return;

        clearDestinations();
        clearActionQueue();
        pushAction(CreatureAction::flee);
        return;
    }

    // If we are weak we do not attack
    if (isWeak)
        return;

    // If a fighter can see enemies that are reachable, he may attack
    if (!mReachableEnemyObjects.empty() && !mDefinition->isWorker())
    {
        if(isActionInList(CreatureAction::fight) || isActionInList(CreatureAction::flee))
            return;

        // If we are in bad mood, we have a probability to flee
        if(isInBadMood() && (Random::Int(0,100) > 80))
        {
            clearDestinations();
            clearActionQueue();
            pushAction(CreatureAction::flee);
            return;
        }

        // If we are not already fighting with a creature then start doing so.
        clearDestinations();
        clearActionQueue();
        pushAction(CreatureAction::fight);
        return;
    }

    // If a worker can see other workers, he should attack. mReachableEnemyCreatures is filled with workers only if we are a worker
    if (!mReachableEnemyCreatures.empty() && mDefinition->isWorker())
    {
        // If we are not already fighting with a creature then start doing so.
        if(isActionInList(CreatureAction::fight))
            return;

        clearDestinations();
        clearActionQueue();
        pushAction(CreatureAction::fight);
        return;
    }
}

bool Creature::handleIdleAction()
{
    double diceRoll = Random::Double(0.0, 1.0);
    bool loopBack = false;

    setAnimationState("Idle");

    if(mDefinition->isWorker() && mForceAction == forcedActionSearchAction)
    {
        // If a worker is dropped, he will search in the tile he is and in the 4 neighboor tiles.
        // 1 - If the tile he is in is treasury and he is carrying gold, he should deposit it
        // 2 - if one of the 4 neighboor tiles is marked, he will dig
        // 3 - if the the tile he is in is not claimed and one of the neigbboor tiles is claimed, he will claim
        // 4 - if the the tile he is in is claimed and one of the neigbboor tiles is not claimed, he will claim
        // 5 - If the tile he is in is claimed and one of the neigbboor tiles is a not claimed wall, he will claim
        Tile* position = getPositionTile();
        Seat* seat = getSeat();
        Tile* tileMarkedDig = NULL;
        Tile* tileToClaim = NULL;
        Tile* tileWallNotClaimed = NULL;
        std::vector<Tile*> creatureNeighbors = position->getAllNeighbors();
        for (std::vector<Tile*>::iterator it = creatureNeighbors.begin(); it != creatureNeighbors.end(); ++it)
        {
            Tile* tile = *it;

            if(tileMarkedDig == NULL &&
                tile->getMarkedForDigging(getGameMap()->getPlayerBySeat(seat))
                )
            {
                tileMarkedDig = tile;
            }
            else if(tileToClaim == NULL &&
                tile->getType() == Tile::claimed &&
                tile->isClaimedForSeat(seat) &&
                position->isGroundClaimable() &&
                !position->isClaimedForSeat(seat)
                )
            {
                tileToClaim = position;
            }
            else if(tileToClaim == NULL &&
                position->getType() == Tile::claimed &&
                position->isClaimedForSeat(seat) &&
                tile->isGroundClaimable() &&
                !tile->isClaimedForSeat(seat)
                )
            {
                tileToClaim = tile;
            }
            else if(tileWallNotClaimed == NULL &&
                position->getType() == Tile::claimed &&
                position->isClaimedForSeat(seat) &&
                tile->isWallClaimable(seat)
                )
            {
                tileWallNotClaimed = tile;
            }
        }
        bool forceGoldDeposit = false;
        if(mGold > 0)
        {
            Room* room = position->getCoveringRoom();
            if((room != NULL) && (room->getType() == Room::treasury))
            {
                forceGoldDeposit = true;
            }
        }

        // Now, we can decide
        if(forceGoldDeposit)
        {
            // Deposing gold is one shot, no need to remind that we
            // were dropped on treasury
            mForceAction = forcedActionNone;
            pushAction(CreatureAction::depositGold);
            return true;
        }
        else if(tileMarkedDig != NULL)
        {
            mForceAction = forcedActionDigTile;
        }
        else if(tileToClaim != NULL)
        {
            mForceAction = forcedActionClaimTile;
        }
        else if(tileWallNotClaimed != NULL)
        {
            mForceAction = forcedActionClaimWallTile;
        }
        else
        {
            // We couldn't find why we were dropped here. Let's behave as usual
            mForceAction = forcedActionNone;
        }
    }

    // Handle if worker was dropped
    if(mDefinition->isWorker() && mForceAction != forcedActionNone)
    {
        switch(mForceAction)
        {
            case forcedActionDigTile:
            {
                pushAction(CreatureAction::digTile);
                return true;
            }
            case forcedActionClaimTile:
            {
                pushAction(CreatureAction::claimTile);
                return true;
            }
            case forcedActionClaimWallTile:
            {
                pushAction(CreatureAction::claimWallTile);
                return true;
            }
            default:
                break;
        }
    }

    // Decide to check for diggable tiles
    if (mDefinition->getDigRate() > 0.0 && !getVisibleMarkedTiles().empty())
    {
        loopBack = true;
        pushAction(CreatureAction::digTile);
    }
    // Decide to check for claimable tiles
    else if (mDefinition->getClaimRate() > 0.0 && diceRoll < 0.9)
    {
        loopBack = true;
        pushAction(CreatureAction::claimTile);
    }
    // Decide to deposit the gold we are carrying into a treasury.
    else if (mDefinition->getDigRate() > 0.0 && mGold > 0)
    {
        //TODO: We need a flag to see if we have tried to do this
        // so the creature won't get confused if we are out of space.
        loopBack = true;
        pushAction(CreatureAction::depositGold);
    }

    // Fighters
    bool isWeak = (mHp < mMaxHP / 3);
    // If a fighter is weak, he should try to sleep
    if (isWeak && !mDefinition->isWorker())
    {
        if((mHomeTile != NULL) && (getGameMap()->pathExists(this, getPositionTile(), mHomeTile)))
        {
            pushAction(CreatureAction::sleep);
            return true;
        }

        // If we have no home tile, we try to find one
        if(mHomeTile == NULL)
        {
            std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::dormitory, getSeat());
            tempRooms = getGameMap()->getReachableRooms(tempRooms, getPositionTile(), this);
            if (!tempRooms.empty())
            {
                clearDestinations();
                clearActionQueue();
                pushAction(CreatureAction::sleep);
                pushAction(CreatureAction::findHome);
                return true;
            }
        }

        // If the home tile is not accessible, we wander
        wanderRandomly("Flee");
        return false;
    }

    if(!mDefinition->isWorker() && mForceAction == forcedActionSearchAction)
    {
        mForceAction = forcedActionNone;
        Tile* tile = getPositionTile();
        if(tile != NULL)
        {
            Room* room = tile->getCoveringRoom();
            if(room != NULL)
            {
                // we see if we are in an hatchery
                if((room->getType() == Room::hatchery) && room->hasOpenCreatureSpot(this))
                {
                    pushAction(CreatureAction::eatforced);
                    return true;
                }
                else if(room->getType() == Room::dormitory)
                {
                    pushAction(CreatureAction::sleep);
                    pushAction(CreatureAction::findHomeForced);
                    return true;
                }
                // If not, can we work in this room ?
                else if((room->getType() != Room::hatchery) && room->hasOpenCreatureSpot(this))
                {
                    pushAction(CreatureAction::jobforced);
                    return true;
                }
            }
        }
    }

    // Check to see if we have found a "home" tile where we can sleep. Even if we are not sleepy,
    // we want to have a bed
    if (!mDefinition->isWorker() && Random::Double(0.0, 1.0) < 0.5 && mHomeTile == NULL)
    {
        // Check to see if there are any dormitory owned by our color that we can reach.
        std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::dormitory, getSeat());
        tempRooms = getGameMap()->getReachableRooms(tempRooms, getPositionTile(), this);
        if (!tempRooms.empty())
        {
            pushAction(CreatureAction::findHome);
            return true;
        }
    }

    // If we are sleepy, we go to sleep
    if (!mDefinition->isWorker() && mHomeTile != NULL && Random::Double(0.0, 1.0) < 0.2 && Random::Double(0.0, 50.0) >= mAwakeness)
    {
        // Check to see if we can work
        pushAction(CreatureAction::sleep);
        return true;
    }

    // If we are hungry, we go to eat
    if (!mDefinition->isWorker() && Random::Double(0.0, 1.0) < 0.2 && Random::Double(50.0, 100.0) <= mHunger)
    {
        // Check to see if we can work
        pushAction(CreatureAction::eatdecided);
        return true;
    }

    // Otherwise, we try to work
    if (!mDefinition->isWorker() && Random::Double(0.0, 1.0) < 0.2
        && Random::Double(0.0, 50.0) < mAwakeness && Random::Double(50.0, 100.0) > mHunger)
    {
        // Check to see if we can work
        pushAction(CreatureAction::jobdecided);
        return true;
    }

    // Any creature.

    // Decide whether to "wander" a short distance
    if (diceRoll >= 0.6)
        return loopBack;

    // Workers should move around randomly at large jumps.  Non-workers either wander short distances or follow workers.
    Tile* tileDest = NULL;

    if (!mDefinition->isWorker())
    {
        // Non-workers only.

        // Check to see if we want to try to follow a worker around or if we want to try to explore.
        double r = Random::Double(0.0, 1.0);
        //if(creatureJob == weakFighter) r -= 0.2;
        if (r < 0.7)
        {
            bool workerFound = false;
            // Try to find a worker to follow around.
            for (unsigned int i = 0; !workerFound && i < mReachableAlliedObjects.size(); ++i)
            {
                // Check to see if we found a worker.
                if (mReachableAlliedObjects[i]->getObjectType() == GameEntity::creature
                    && static_cast<Creature*>(mReachableAlliedObjects[i])->mDefinition->isWorker())
                {
                    // We found a worker so find a tile near the worker to walk to.  See if the worker is digging.
                    Tile* tempTile = mReachableAlliedObjects[i]->getCoveredTiles()[0];
                    if (static_cast<Creature*>(mReachableAlliedObjects[i])->peekAction().getType()
                            == CreatureAction::digTile)
                    {
                        // Worker is digging, get near it since it could expose enemies.
                        int x = (int)(static_cast<double>(tempTile->x) + 3.0
                                * Random::gaussianRandomDouble());
                        int y = (int)(static_cast<double>(tempTile->y) + 3.0
                                * Random::gaussianRandomDouble());
                        tileDest = getGameMap()->getTile(x, y);
                    }
                    else
                    {
                        // Worker is not digging, wander a bit farther around the worker.
                        int x = (int)(static_cast<double>(tempTile->x) + 8.0
                                * Random::gaussianRandomDouble());
                        int y = (int)(static_cast<double>(tempTile->y) + 8.0
                                * Random::gaussianRandomDouble());
                        tileDest = getGameMap()->getTile(x, y);
                    }
                    workerFound = true;
                }

                // If there are no workers around, choose tiles far away to "roam" the dungeon.
                if (!workerFound)
                {
                    if (!mVisibleTiles.empty())
                    {
                        tileDest = mVisibleTiles[static_cast<unsigned int>(Random::Double(0.6, 0.8)
                                                                           * (mVisibleTiles.size() - 1))];
                    }
                }
            }
        }
        else
        {
            // Randomly choose a tile near where we are standing to walk to.
            if (!mVisibleTiles.empty())
            {
                unsigned int tileIndex = static_cast<unsigned int>(mVisibleTiles.size()
                                                                   * Random::Double(0.1, 0.3));
                tileDest = mVisibleTiles[tileIndex];
            }
        }
    }
    else
    {
        // Workers only.

        // Choose a tile far away from our current position to wander to.
        if (!mVisibleTiles.empty())
        {
            tileDest = mVisibleTiles[Random::Uint(mVisibleTiles.size() / 2, mVisibleTiles.size() - 1)];
        }
    }

    setDestination(tileDest);
    return true;
}

bool Creature::handleWalkToTileAction()
{
    if (mWalkQueue.empty())
    {
        popAction();
        return true;
    }

    // If we are moving during a fight, we do not wait to reach destination to force to compute again what to do
    // Because ennemies may have moved, closest creatures could be near...
    if(isActionInList(CreatureAction::fight) && mNbTurnAction > 1)
    {
        clearDestinations();
        popAction();
        return true;
    }

    return false;
}

bool Creature::handleClaimTileAction()
{
    Tile* myTile = getPositionTile();
    if (myTile == NULL)
    {
        popAction();
        return false;
    }

    if(mForceAction != forcedActionClaimTile)
    {
        // Randomly decide to stop claiming with a small probability
        std::vector<Tile*> markedTiles = getVisibleMarkedTiles();
        if (Random::Double(0.0, 1.0) < 0.1 + 0.2 * markedTiles.size())
        {
            // If there are any visible tiles marked for digging start working on that.
            if (!markedTiles.empty())
            {
                popAction();
                pushAction(CreatureAction::digTile);
                return true;
            }
        }
    }

    // See if the tile we are standing on can be claimed
    if ((!myTile->isClaimedForSeat(getSeat()) || myTile->getClaimedPercentage() < 1.0) && myTile->isGroundClaimable())
    {
        //cout << "\nTrying to claim the tile I am standing on.";
        // Check to see if one of the tile's neighbors is claimed for our color
        std::vector<Tile*> neighbors = myTile->getAllNeighbors();
        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            // Check to see if the current neighbor is a claimed ground tile
            Tile* tempTile = neighbors[j];
            if (tempTile->isClaimedForSeat(getSeat()) && (tempTile->getFullness() == 0.0) && tempTile->getClaimedPercentage() >= 1.0)
            {
                //cout << "\t\tFound a neighbor that is claimed.";
                // If we found a neighbor that is claimed for our side than we can start
                // dancing on this tile.  If there is "left over" claiming that can be done
                // it will spill over into neighboring tiles until it is gone.
                setAnimationState("Claim");
                myTile->claimForSeat(getSeat(), mDefinition->getClaimRate());
                receiveExp(1.5 * (mDefinition->getClaimRate() / (0.35 + 0.05 * getLevel())));

                // Since we danced on a tile we are done for this turn
                return false;
            }
        }
    }

    // The tile we are standing on is already claimed or is not currently
    // claimable, find candidates for claiming.
    // Start by checking the neighbor tiles of the one we are already in
    std::vector<Tile*> neighbors = myTile->getAllNeighbors();
    while (!neighbors.empty())
    {
        // If the current neighbor is claimable, walk into it and skip to the end of this turn
        int tempInt = Random::Uint(0, neighbors.size() - 1);
        Tile* tempTile = neighbors[tempInt];
        if (tempTile != NULL && tempTile->getFullness() == 0.0
            && (!tempTile->isClaimedForSeat(getSeat()) || tempTile->getClaimedPercentage() < 1.0)
            && tempTile->isGroundClaimable())
        {
            // The neighbor tile is a potential candidate for claiming, to be an actual candidate
            // though it must have a neighbor of its own that is already claimed for our side.
            Tile* tempTile2;
            std::vector<Tile*> neighbors2 = tempTile->getAllNeighbors();
            for (unsigned int i = 0; i < neighbors2.size(); ++i)
            {
                tempTile2 = neighbors2[i];
                if (tempTile2->isClaimedForSeat(getSeat())
                        && tempTile2->getClaimedPercentage() >= 1.0)
                {
                    clearDestinations();
                    addDestination((Ogre::Real)tempTile->x, (Ogre::Real)tempTile->y);
                    setAnimationState("Walk");
                    return false;
                }
            }
        }

        neighbors.erase(neighbors.begin() + tempInt);
    }

    //cout << "\nLooking at the visible tiles to see if I can claim a tile.";
    // If we still haven't found a tile to claim, check the rest of the visible tiles
    std::vector<Tile*> claimableTiles;
    for (unsigned int i = 0; i < mVisibleTiles.size(); ++i)
    {
        // if this tile is not fully claimed yet or the tile is of another player's color
        Tile* tempTile = mVisibleTiles[i];
        if (tempTile != NULL && tempTile->getFullness() == 0.0
            && (tempTile->getClaimedPercentage() < 1.0 || !tempTile->isClaimedForSeat(getSeat()))
            && tempTile->isGroundClaimable())
        {
            // Check to see if one of the tile's neighbors is claimed for our color
            neighbors = mVisibleTiles[i]->getAllNeighbors();
            for (unsigned int j = 0; j < neighbors.size(); ++j)
            {
                tempTile = neighbors[j];
                if (tempTile->isClaimedForSeat(getSeat())
                        && tempTile->getClaimedPercentage() >= 1.0)
                {
                    claimableTiles.push_back(tempTile);
                }
            }
        }
    }

    //cout << "  I see " << claimableTiles.size() << " tiles I can claim.";
    // Randomly pick a claimable tile, plot a path to it and walk to it
    unsigned int tempUnsigned = 0;
    Tile* tempTile = NULL;
    while (!claimableTiles.empty())
    {
        // Randomly find a "good" tile to claim.  A good tile is one that has many neighbors
        // already claimed, this makes the claimed are more "round" and less jagged.
        do
        {
            int numNeighborsClaimed = 0;

            // Start by randomly picking a candidate tile.
            tempTile = claimableTiles[Random::Uint(0, claimableTiles.size() - 1)];

            // Count how many of the candidate tile's neighbors are already claimed.
            neighbors = tempTile->getAllNeighbors();
            for (unsigned int i = 0; i < neighbors.size(); ++i)
            {
                if (neighbors[i]->isClaimedForSeat(getSeat()) && neighbors[i]->getClaimedPercentage() >= 1.0)
                    ++numNeighborsClaimed;
            }

            // Pick a random number in [0:1], if this number is high enough, than use this tile to claim.  The
            // bar for success approaches 0 as numTiles approaches N so this will be guaranteed to succeed at,
            // or before the time we get to the last unclaimed tile.  The bar for success is also lowered
            // according to how many neighbors are already claimed.
            //NOTE: The bar can be negative, when this happens we are guarenteed to use this candidate tile.
            double bar = 1.0 - (numNeighborsClaimed / 4.0) - (tempUnsigned / (double) (claimableTiles.size() - 1));
            if (Random::Double(0.0, 1.0) >= bar)
                break;

            // Safety catch to prevent infinite loop in case the bar for success is too high and is never met.
            if (tempUnsigned >= claimableTiles.size() - 1)
                break;

            // Increment the counter indicating how many candidate tiles we have rejected so far.
            ++tempUnsigned;
        } while (true);

        if (tempTile != NULL)
        {
            // If we find a valid path to the tile start walking to it and break
            if (setDestination(tempTile))
                return false;
        }

        // If we got to this point, the tile we randomly picked cannot be gotten to via a
        // valid path.  Delete it from the claimable tiles vector and repeat the outer
        // loop to try to find another valid tile.
        for (unsigned int i = 0; i < claimableTiles.size(); ++i)
        {
            if (claimableTiles[i] == tempTile)
            {
                claimableTiles.erase(claimableTiles.begin() + i);
                break; // Break out of this for loop.
            }
        }
    }

    // We couldn't find a tile to try to claim so we start searching for claimable walls
    mForceAction = forcedActionNone;
    popAction();
    pushAction(CreatureAction::claimWallTile);
    return true;
}

bool Creature::handleClaimWallTileAction()
{
    Tile* myTile = getPositionTile();
    if (myTile == NULL)
    {
        popAction();
        return false;
    }

    // Randomly decide to stop claiming with a small probability
    if(mForceAction != forcedActionClaimWallTile)
    {
        std::vector<Tile*> markedTiles = getVisibleMarkedTiles();
        if (Random::Double(0.0, 1.0) < 0.1 + 0.2 * markedTiles.size())
        {
            // If there are any visible tiles marked for digging start working on that.
            if (!markedTiles.empty())
            {
                popAction();
                pushAction(CreatureAction::digTile);
                return true;
            }
        }
    }

    //std::cout << "Claim wall" << std::endl;

    // See if any of the tiles is one of our neighbors
    bool wasANeighbor = false;
    std::vector<Tile*> creatureNeighbors = myTile->getAllNeighbors();
    Player* tempPlayer = getGameMap()->getPlayerBySeat(getSeat());
    for (unsigned int i = 0; i < creatureNeighbors.size() && !wasANeighbor; ++i)
    {
        if (tempPlayer == NULL)
            break;

        Tile* tempTile = creatureNeighbors[i];

        if (!tempTile->isWallClaimable(getSeat()))
            continue;

        // Dig out the tile by decreasing the tile's fullness.
        Ogre::Vector3 walkDirection(tempTile->x - getPosition().x, tempTile->y - getPosition().y, 0);
        walkDirection.normalise();
        setAnimationState("Claim", true, &walkDirection);
        tempTile->claimForSeat(getSeat(), mDefinition->getClaimRate());
        receiveExp(1.5 * mDefinition->getClaimRate() / 20.0);

        wasANeighbor = true;
        //std::cout << "Claiming wall" << std::endl;
        break;
    }

    // If we successfully found a wall tile to claim then we are done for this turn.
    if (wasANeighbor)
        return false;
    //std::cout << "Looking for a wall to claim" << std::endl;

    // Find paths to all of the neighbor tiles for all of the visible wall tiles.
    std::vector<std::list<Tile*> > possiblePaths;
    std::vector<Tile*> wallTiles = getVisibleClaimableWallTiles();
    for (unsigned int i = 0; i < wallTiles.size(); ++i)
    {
        std::vector<Tile*> neighbors = wallTiles[i]->getAllNeighbors();
        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            Tile* neighborTile = neighbors[j];
            if (neighborTile != NULL && neighborTile->getFullness() == 0.0)
                possiblePaths.push_back(getGameMap()->path(this, neighborTile));
        }
    }

    // Find the shortest path and start walking toward the tile to be dug out
    if (!possiblePaths.empty())
    {
        // Find the N shortest valid paths, see if there are any valid paths shorter than this first guess
        std::vector<std::list<Tile*> > shortPaths;
        for (unsigned int i = 0; i < possiblePaths.size(); ++i)
        {
            // If the current path is long enough to be valid
            unsigned int currentLength = possiblePaths[i].size();
            if (currentLength >= 2)
            {
                shortPaths.push_back(possiblePaths[i]);

                // If we already have enough short paths
                if (shortPaths.size() > 5)
                {
                    unsigned int longestLength, longestIndex;

                    // Kick out the longest
                    longestLength = shortPaths[0].size();
                    longestIndex = 0;
                    for (unsigned int j = 1; j < shortPaths.size(); ++j)
                    {
                        if (shortPaths[j].size() > longestLength)
                        {
                            longestLength = shortPaths.size();
                            longestIndex = j;
                        }
                    }

                    shortPaths.erase(shortPaths.begin() + longestIndex);
                }
            }
        }

        // Randomly pick a short path to take
        unsigned int numShortPaths = shortPaths.size();
        if (numShortPaths > 0)
        {
            unsigned int shortestIndex;
            shortestIndex = Random::Uint(0, numShortPaths - 1);
            std::list<Tile*> walkPath = shortPaths[shortestIndex];

            // If the path is a legitimate path, walk down it to the tile to be dug out
            if (setWalkPath(walkPath, 2, false))
            {
                setAnimationState("Walk");
                pushAction(CreatureAction::walkToTile);
                return false;
            }
        }
    }

    // If we found no path, let's stop doing this
    mForceAction = forcedActionNone;
    popAction();
    return true;
}

bool Creature::handleDigTileAction()
{
    Tile* myTile = getPositionTile();
    if (myTile == NULL)
        return false;

    // See if any of the tiles is one of our neighbors
    bool wasANeighbor = false;
    std::vector<Tile*> creatureNeighbors = myTile->getAllNeighbors();
    Player* tempPlayer = getGameMap()->getPlayerBySeat(getSeat());
    for (unsigned int i = 0; i < creatureNeighbors.size() && !wasANeighbor; ++i)
    {
        if (tempPlayer == NULL)
            break;

        Tile* tempTile = creatureNeighbors[i];

        if (!tempTile->getMarkedForDigging(tempPlayer))
            continue;

        // We found a tile marked by our controlling seat, dig out the tile.

        // If the tile is a gold tile accumulate gold for this creature.
        if (tempTile->getType() == Tile::gold)
        {
            //FIXME: Make sure we can't dig gold if the creature has max gold.
            // Or let gold on the ground, until there is space so that the player
            // isn't stuck when making a way through gold.
            double tempDouble = 5 * std::min(mDefinition->getDigRate(), tempTile->getFullness());
            mGold += (int)tempDouble;
            getSeat()->mGoldMined += static_cast<int>(tempDouble);
            receiveExp(5.0 * mDefinition->getDigRate() / 20.0);
        }

        // Dig out the tile by decreasing the tile's fullness.
        Ogre::Vector3 walkDirection(tempTile->x - getPosition().x, tempTile->y - getPosition().y, 0);
        walkDirection.normalise();
        setAnimationState("Dig", true, &walkDirection);
        double amountDug = tempTile->digOut(mDefinition->getDigRate(), true);
        if(amountDug > 0.0)
        {
            receiveExp(1.5 * mDefinition->getDigRate() / 20.0);

            // If the tile has been dug out, move into that tile and try to continue digging.
            if (tempTile->getFullness() == 0.0)
            {
                receiveExp(2.5);
                setAnimationState("Walk");

                // Remove the dig action and replace it with
                // walking to the newly dug out tile.
                //popAction();
                addDestination((Ogre::Real)tempTile->x, (Ogre::Real)tempTile->y);
                pushAction(CreatureAction::walkToTile);
            }
            //Set sound position and play dig sound.
            try
            {
                std::string name = getName();
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotification::playCreatureSound, getGameMap()->getPlayerBySeat(getSeat()));
                serverNotification->mPacket << name << CreatureSound::DIGGING;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
            catch (std::bad_alloc&)
            {
                OD_ASSERT_TRUE(false);
                exit(1);
            }
        }
        else
        {
            //We tried to dig a tile we are not able to
            //Completely bail out if this happens.
            clearActionQueue();
        }

        wasANeighbor = true;
        break;
    }

    // Check to see if we are carrying the maximum amount of gold we can carry, and if so, try to take it to a treasury.
    if (mGold >= MaxGoldCarriedByWorkers)
    {
        // Remove the dig action and replace it with a depositGold action.
        pushAction(CreatureAction::depositGold);
    }

    // If we successfully dug a tile then we are done for this turn.
    if (wasANeighbor)
        return false;

    // Find paths to all of the neighbor tiles for all of the marked visible tiles.
    std::vector<std::list<Tile*> > possiblePaths;
    std::vector<Tile*> markedTiles = getVisibleMarkedTiles();
    for (unsigned int i = 0; i < markedTiles.size(); ++i)
    {
        std::vector<Tile*> neighbors = markedTiles[i]->getAllNeighbors();
        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            Tile* neighborTile = neighbors[j];
            if (neighborTile != NULL && neighborTile->getFullness() == 0.0)
                possiblePaths.push_back(getGameMap()->path(this, neighborTile));
        }
    }

    // Find the shortest path and start walking toward the tile to be dug out
    if (!possiblePaths.empty())
    {
        // Find the N shortest valid paths, see if there are any valid paths shorter than this first guess
        std::vector<std::list<Tile*> > shortPaths;
        for (unsigned int i = 0; i < possiblePaths.size(); ++i)
        {
            // If the current path is long enough to be valid
            unsigned int currentLength = possiblePaths[i].size();
            if (currentLength >= 2)
            {
                shortPaths.push_back(possiblePaths[i]);

                // If we already have enough short paths
                if (shortPaths.size() > 5)
                {
                    unsigned int longestLength, longestIndex;

                    // Kick out the longest
                    longestLength = shortPaths[0].size();
                    longestIndex = 0;
                    for (unsigned int j = 1; j < shortPaths.size(); ++j)
                    {
                        if (shortPaths[j].size() > longestLength)
                        {
                            longestLength = shortPaths.size();
                            longestIndex = j;
                        }
                    }

                    shortPaths.erase(shortPaths.begin() + longestIndex);
                }
            }
        }

        // Randomly pick a short path to take
        unsigned int numShortPaths = shortPaths.size();
        if (numShortPaths > 0)
        {
            unsigned int shortestIndex;
            shortestIndex = Random::Uint(0, numShortPaths - 1);
            std::list<Tile*> walkPath = shortPaths[shortestIndex];

            // If the path is a legitimate path, walk down it to the tile to be dug out
            if (setWalkPath(walkPath, 2, false))
            {
                setAnimationState("Walk");
                pushAction(CreatureAction::walkToTile);
                return false;
            }
        }
    }

    // If none of our neighbors are marked for digging we got here too late.
    // Finish digging
    mForceAction = forcedActionNone;
    bool isDigging = (peekAction().getType() == CreatureAction::digTile);
    if (isDigging)
    {
        popAction();
        if(mGold > 0)
            pushAction(CreatureAction::depositGold);

        return true;
    }
    return false;
}

bool Creature::handleDepositGoldAction()
{
    // Check to see if we are standing in a treasury.
    Tile* myTile = getPositionTile();
    if (myTile == NULL)
        return false;

    Room* tempRoom = myTile->getCoveringRoom();
    if (tempRoom != NULL && tempRoom->getType() == Room::treasury)
    {
        // Deposit as much of the gold we are carrying as we can into this treasury.
        mGold -= static_cast<RoomTreasury*>(tempRoom)->depositGold(mGold, myTile);

        // Depending on how much gold we have left (what did not fit in this treasury) we may want to continue
        // looking for another treasury to put the gold into.  Roll a dice to see if we want to quit looking not.
        if (Random::Double(1.0, MaxGoldCarriedByWorkers) > mGold)
        {
            popAction();
            return false;
        }
    }

    // We were not standing in a treasury that has enough room for the gold we are carrying, so try to find one to walk to.
    // Check to see if our seat controls any treasuries.
    std::vector<Room*> treasuriesOwned = getGameMap()->getRoomsByTypeAndSeat(Room::treasury, getSeat());
    Tile* nearestTreasuryTile = NULL;
    unsigned int nearestTreasuryDistance = 0;
    bool validPathFound = false;
    std::list<Tile*> tempPath;

    // Loop over the treasuries to find the closest one.
    for (unsigned int i = 0; i < treasuriesOwned.size(); ++i)
    {
        if (!validPathFound)
        {
            // We have not yet found a valid path to a treasury, check to see if we can get to this treasury.
            unsigned int tempUnsigned = Random::Uint(0, treasuriesOwned[i]->numCoveredTiles() - 1);
            nearestTreasuryTile = treasuriesOwned[i]->getCoveredTile(tempUnsigned);
            tempPath = getGameMap()->path(this, nearestTreasuryTile);
            if (tempPath.size() >= 2 && static_cast<RoomTreasury*>(treasuriesOwned[i])->emptyStorageSpace() > 0)
            {
                validPathFound = true;
                nearestTreasuryDistance = tempPath.size();
            }
        }
        else
        {
            // We have already found at least one valid path to a treasury, see if this one is closer.
            unsigned int tempUnsigned = Random::Uint(0, treasuriesOwned[i]->numCoveredTiles() - 1);
            Tile* tempTile = treasuriesOwned[i]->getCoveredTile(tempUnsigned);
            std::list<Tile*> tempPath2 = getGameMap()->path(this, tempTile);
            if (tempPath2.size() >= 2 && tempPath2.size() < nearestTreasuryDistance
                && static_cast<RoomTreasury*>(treasuriesOwned[i])->emptyStorageSpace() > 0)
            {
                tempPath = tempPath2;
                nearestTreasuryDistance = tempPath.size();
            }
        }
    }

    if (validPathFound)
    {
        // Begin walking to this treasury.
        if (setWalkPath(tempPath, 2, false))
        {
            setAnimationState("Walk");
            pushAction(CreatureAction::walkToTile);
            return false;
        }
    }

    // If we get to here, there is either no treasuries controlled by us, or they are all
    // unreachable, or they are all full, so we let the gold on the ground
    popAction();

    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return true;
    TreasuryObject* obj = new TreasuryObject(getGameMap(), mGold);
    mGold = 0;
    Ogre::Vector3 pos(static_cast<Ogre::Real>(tile->x), static_cast<Ogre::Real>(tile->y), 0.0f);
    obj->setPosition(pos);
    getGameMap()->addRenderedMovableEntity(obj);

    return true;
}

bool Creature::handleFindHomeAction(bool isForced)
{
    // Check to see if we are standing in an open dormitory tile that we can claim as our home.
    Tile* myTile = getPositionTile();
    if (myTile == NULL)
    {
        popAction();
        return false;
    }

    if((mHomeTile != NULL) && !isForced)
    {
        popAction();
        return false;
    }

    Room* tempRoom = myTile->getCoveringRoom();
    if (tempRoom != NULL && tempRoom->getType() == Room::dormitory && getSeat()->canOwnedCreatureUseRoomFrom(tempRoom->getSeat()))
    {
        Room* roomHomeTile = NULL;
        if(mHomeTile != NULL)
        {
            roomHomeTile = mHomeTile->getCoveringRoom();
            // Same dormitory nothing to do
            if(roomHomeTile == tempRoom)
            {
                popAction();
                return true;
            }
        }

        if (static_cast<RoomDormitory*>(tempRoom)->claimTileForSleeping(myTile, this))
        {
            // We could install the bed in the dormitory. If we already had one, we remove it
            if(roomHomeTile != NULL)
                static_cast<RoomDormitory*>(roomHomeTile)->releaseTileForSleeping(mHomeTile, this);

            mHomeTile = myTile;
            popAction();
            return true;
        }

        // The tile where we are is not claimable. We search if there is another in this dormitory
        Tile* tempTile = static_cast<RoomDormitory*>(tempRoom)->getLocationForBed(
            mDefinition->getBedDim1(), mDefinition->getBedDim2());
        if(tempTile != NULL)
        {
            std::list<Tile*> tempPath = getGameMap()->path(this, tempTile);
            if (setWalkPath(tempPath, 1, false))
            {
                setAnimationState("Walk");
                pushAction(CreatureAction::walkToTile);
                return false;
            }
        }
    }

    // If we found a tile to claim as our home in the above block
    // If we have been forced, we do not search in another dormitory
    if ((mHomeTile != NULL) || isForced)
    {
        popAction();
        return true;
    }

    // Check to see if we can walk to a dormitory that does have an open tile.
    std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::dormitory, getSeat());
    std::random_shuffle(tempRooms.begin(), tempRooms.end());
    unsigned int nearestDormitoryDistance = 0;
    bool validPathFound = false;
    std::list<Tile*> tempPath;
    for (unsigned int i = 0; i < tempRooms.size(); ++i)
    {
        // Get the list of open rooms at the current dormitory and check to see if
        // there is a place where we could put a bed big enough to sleep in.
        Tile* tempTile = static_cast<RoomDormitory*>(tempRooms[i])->getLocationForBed(
                        mDefinition->getBedDim1(), mDefinition->getBedDim2());

        // If the previous attempt to place the bed in this dormitory failed, try again with the bed the other way.
        if (tempTile == NULL)
            tempTile = static_cast<RoomDormitory*>(tempRooms[i])->getLocationForBed(
                                                                     mDefinition->getBedDim2(), mDefinition->getBedDim1());

        // Check to see if either of the two possible bed orientations tried above resulted in a successful placement.
        if (tempTile != NULL)
        {
            std::list<Tile*> tempPath2 = getGameMap()->path(this, tempTile);

            // Find out the minimum valid path length of the paths determined in the above block.
            if (!validPathFound)
            {
                // If the current path is long enough to be valid then record the path and the distance.
                if (tempPath2.size() >= 2)
                {
                    tempPath = tempPath2;
                    nearestDormitoryDistance = tempPath.size();
                    validPathFound = true;
                }
            }
            else
            {
                // If the current path is long enough to be valid but shorter than the
                // shortest path seen so far, then record the path and the distance.
                if (tempPath2.size() >= 2 && tempPath2.size()
                        < nearestDormitoryDistance)
                {
                    tempPath = tempPath2;
                    nearestDormitoryDistance = tempPath.size();
                }
            }
        }
    }

    // If we found a valid path to an open room in a dormitory, then start walking along it.
    if (validPathFound)
    {
        if (setWalkPath(tempPath, 2, false))
        {
            setAnimationState("Walk");
            pushAction(CreatureAction::walkToTile);
            return false;
        }
    }

    // If we got here there are no reachable dormitory that are unclaimed so we quit trying to find one.
    popAction();
    return true;
}

bool Creature::handleJobAction(bool isForced)
{
    // Current creature tile position
    Tile* myTile = getPositionTile();

    // Randomly decide to stop working, we are more likely to stop when we are tired.
    if (Random::Double(20.0, 50.0) > mAwakeness)
    {
        popAction();

        stopJob();
        return true;
    }
    // Make sure we are on the map.
    else if (myTile != NULL)
    {
        // If we are already working, nothing to do
        if(mJobRoom != NULL)
            return false;

        // See if we are in a room where we can work. If so, we try to add the creature. If it is ok, the room
        // will handle the creature from here to make it go where it should
        Room* tempRoom = myTile->getCoveringRoom();
        if ((tempRoom != NULL) && getSeat()->canOwnedCreatureUseRoomFrom(tempRoom->getSeat()))
        {
            // It is the room responsability to test if the creature is suited for working in it
            if(tempRoom->hasOpenCreatureSpot(this) && (tempRoom->getType() != Room::hatchery) && tempRoom->addCreatureUsingRoom(this))
            {
                mJobRoom = tempRoom;
                return false;
            }
        }
    }
    else if (myTile == NULL)
    {
        // We are not on the map, don't do anything.
        popAction();

        stopJob();
        return false;
    }

    // TODO : We should decide which room to use depending on the creatures preferences (library
    // for wizards, ...).

    // Get the list of trainingHalls controlled by our seat and make sure there is at least one.
    std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::trainingHall, getSeat());

    if (tempRooms.empty())
    {
        popAction();

        stopJob();
        return true;
    }

    // Pick a room we want to work in and try to walk to it.
    double maxTrainDistance = 40.0;
    Room* tempRoom = NULL;
    int nbTry = 5;
    do
    {
        int tempInt = Random::Uint(0, tempRooms.size() - 1);
        tempRoom = tempRooms[tempInt];
        tempRooms.erase(tempRooms.begin() + tempInt);
        double tempDouble = 1.0 / (maxTrainDistance - getGameMap()->crowDistance(myTile, tempRoom->getCoveredTile(0)));
        if (Random::Double(0.0, 1.0) < tempDouble)
            break;
        --nbTry;
    } while (nbTry > 0 && !tempRoom->hasOpenCreatureSpot(this) && !tempRooms.empty());

    if (!tempRoom || !tempRoom->hasOpenCreatureSpot(this))
    {
        // The room is already being used, stop trying to work.
        popAction();
        stopJob();
        return true;
    }

    Tile* tempTile = tempRoom->getCoveredTile(Random::Uint(0, tempRoom->numCoveredTiles() - 1));
    std::list<Tile*> tempPath = getGameMap()->path(this, tempTile);
    if (tempPath.size() < maxTrainDistance && setWalkPath(tempPath, 2, false))
    {
        setAnimationState("Walk");
        pushAction(CreatureAction::walkToTile);
        return false;
    }
    else
    {
        // We could not find a room where we can work so stop trying to find one.
        popAction();
    }

    // Default action
    stopJob();
    return true;
}

bool Creature::handleEatingAction(bool isForced)
{
    // Current creature tile position
    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE(myTile != nullptr);
    if(myTile == nullptr)
    {
        popAction();

        stopEating();
        return true;
    }

    if(mEatCooldown > 0)
    {
        --mEatCooldown;
        // We do nothing
        setAnimationState("Idle");
        return false;
    }

    if ((isForced && mHunger < 5.0) ||
        (!isForced && mHunger <= Random::Double(0.0, 15.0)))
    {
        popAction();

        stopEating();
        return true;
    }

    // If we are in a hatchery, we go to the closest chicken in it. If we are not
    // in a hatchery, we check if there is a free chicken and eat it if we see it
    Tile* closestChickenTile = nullptr;
    double closestChickenDist = 0.0;
    for(std::vector<Tile*>::iterator it = mVisibleTiles.begin(); it != mVisibleTiles.end(); ++it)
    {
        Tile* tile = *it;
        const std::vector<ChickenEntity*>& chickens = tile->getChickenEntities();
        if(chickens.empty())
            continue;

        if((mEatRoom == nullptr) && (tile->getCoveringRoom() != nullptr) &&
           (tile->getCoveringRoom()->getType() == Room::RoomType::hatchery))
            continue;

        if((mEatRoom != nullptr) && (tile->getCoveringRoom() != mEatRoom))
            continue;

        double dist = std::pow(static_cast<double>(std::abs(tile->getX() - myTile->getX())), 2);
        dist += std::pow(static_cast<double>(std::abs(tile->getY() - myTile->getY())), 2);
        if((closestChickenTile == nullptr) ||
           (closestChickenDist > dist))
        {
            closestChickenTile = tile;
            closestChickenDist = dist;
        }
    }

    if(closestChickenTile != nullptr)
    {
        if(closestChickenDist <= 1.0)
        {
            // We eat the chicken
            ChickenEntity* chicken = closestChickenTile->getChickenEntities().at(0);
            chicken->eatChicken(this);
            foodEaten(20);
            mEatCooldown = Random::Int(3, 8);
            Ogre::Vector3 walkDirection = Ogre::Vector3(closestChickenTile->getX(), closestChickenTile->getY(), 0) - getPosition();
            walkDirection.normalise();
            setAnimationState("Attack1", false, &walkDirection);
            return false;
        }

        // We walk to the chicken
        std::list<Tile*> pathToChicken = getGameMap()->path(this, closestChickenTile);
        OD_ASSERT_TRUE(!pathToChicken.empty());
        if(pathToChicken.empty())
        {
            popAction();

            stopEating();
            return true;
        }

        // We make sure we don't go too far as the chicken is also moving
        if(pathToChicken.size() > 2)
        {
            // We only keep 80% of the path
            int nbTiles = 8 * pathToChicken.size() / 10;
            pathToChicken.resize(nbTiles);
        }

        if(setWalkPath(pathToChicken, 0, false))
        {
            setAnimationState("Walk");
            pushAction(CreatureAction::walkToTile);
            return false;
        }
    }

    if(mEatRoom != nullptr)
        return false;

    // See if we are in a hatchery. If so, we try to add the creature. If it is ok, the room
    // will handle the creature from here to make it go where it should
    Room* tempRoom = myTile->getCoveringRoom();
    if ((tempRoom != nullptr) && getSeat()->canOwnedCreatureUseRoomFrom(tempRoom->getSeat()) && (tempRoom->getType() == Room::hatchery) && tempRoom->hasOpenCreatureSpot(this))
    {
        if(tempRoom->addCreatureUsingRoom(this))
        {
            mEatRoom = tempRoom;
            return false;
        }
    }

    // Get the list of hatchery controlled by our seat and make sure there is at least one.
    std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::hatchery, getSeat());

    if (tempRooms.empty())
    {
        popAction();

        stopEating();
        return true;
    }

    // Pick a hatchery and try to walk to it.
    double maxDistance = 40.0;
    tempRoom = nullptr;
    int nbTry = 5;
    do
    {
        int tempInt = Random::Uint(0, tempRooms.size() - 1);
        tempRoom = tempRooms[tempInt];
        tempRooms.erase(tempRooms.begin() + tempInt);
        double tempDouble = 1.0 / (maxDistance - getGameMap()->crowDistance(myTile, tempRoom->getCoveredTile(0)));
        if (Random::Double(0.0, 1.0) < tempDouble)
            break;
        --nbTry;
    } while (nbTry > 0 && !tempRoom->hasOpenCreatureSpot(this) && !tempRooms.empty());

    if (!tempRoom || !tempRoom->hasOpenCreatureSpot(this))
    {
        // The room is already being used, stop trying to eat
        popAction();
        stopEating();
        return true;
    }

    Tile* tempTile = tempRoom->getCoveredTile(Random::Uint(0, tempRoom->numCoveredTiles() - 1));
    std::list<Tile*> tempPath = getGameMap()->path(this, tempTile);
    if (tempPath.size() < maxDistance && setWalkPath(tempPath, 2, false))
    {
        setAnimationState("Walk");
        pushAction(CreatureAction::walkToTile);
        return false;
    }
    else
    {
        // We could not find a room where we can eat so stop trying to find one.
        popAction();
    }

    // Default action
    stopEating();
    return true;
}

void Creature::stopJob()
{
    if (mJobRoom == NULL)
        return;

    mJobRoom->removeCreatureUsingRoom(this);
    mJobRoom = NULL;
}

void Creature::stopEating()
{
    if (mEatRoom == NULL)
        return;

    mEatRoom->removeCreatureUsingRoom(this);
    mEatRoom = NULL;
}

bool Creature::isJobRoom(Room* room)
{
    return mJobRoom == room;
}

bool Creature::isEatRoom(Room* room)
{
    return mEatRoom == room;
}

void Creature::changeJobRoom(Room* newRoom)
{
    if (mJobRoom != NULL)
        mJobRoom->removeCreatureUsingRoom(this);


    if(newRoom != NULL && newRoom->addCreatureUsingRoom(this))
        mJobRoom = newRoom;
    else
        mJobRoom = NULL;
}

void Creature::changeEatRoom(Room* newRoom)
{
    if (mEatRoom != NULL)
        mEatRoom->removeCreatureUsingRoom(this);


    if(newRoom != NULL && newRoom->addCreatureUsingRoom(this))
        mEatRoom = newRoom;
    else
        mEatRoom = NULL;
}

bool Creature::handleAttackAction()
{
    // We always pop action to make sure next time we will try to find if a closest foe is there
    // or if we need to hit and run
    popAction();

    if (mAttackedTile == NULL)
        return true;

    // The warmup time isn't yet finished.
    if (mAttackWarmupTime > 0.0)
        return true;

    // Reset the warmup time
    mAttackWarmupTime = mDefinition->getAttackWarmupTime();

    Tile* attackedTile = mAttackedTile;
    GameEntity* attackedObject = mAttackedObject;
    mAttackedTile = NULL;
    mAttackedObject = NULL;

    // We check what we are attacking.

    // Turn to face the creature we are attacking and set the animation state to Attack.
    Ogre::Vector3 walkDirection(attackedTile->x - getPosition().x, attackedTile->y - getPosition().y, 0);
    walkDirection.normalise();
    setAnimationState("Attack1", false, &walkDirection);

    try
    {
        std::string name = getName();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::playCreatureSound, getGameMap()->getPlayerBySeat(getSeat()));
        serverNotification->mPacket << name << CreatureSound::ATTACK;
        ODServer::getSingleton().queueServerNotification(serverNotification);

        getGameMap()->playerIsFighting(getGameMap()->getPlayerBySeat(getSeat()));

    }
    catch (std::bad_alloc&)
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }

    // Calculate how much damage we do.
    Tile* myTile = getPositionTile();
    Ogre::Real range = getGameMap()->crowDistance(myTile, attackedTile);
    // Physical
    double physDamage = getPhysicalDamage(range);
    physDamage -= attackedObject->getPhysicalDefense();
    if (physDamage < 0.0)
        physDamage = 0.0;
    // + Magical
    double magDamage = getMagicalDamage(range);
    magDamage -= attackedObject->getMagicalDefense();
    if (magDamage < 0.0)
        magDamage = 0.0;
    double damageDone = physDamage + magDamage;

    // Do the damage and award experience points to both creatures.
    attackedObject->takeDamage(this, damageDone, attackedTile);
    double expGained;
    expGained = 1.0 + 0.2 * std::pow(damageDone, 1.3);
    mAwakeness -= 0.5;

    // Give a small amount of experince to the creature we hit.
    if(attackedObject->getObjectType() == GameEntity::creature)
    {
        Creature* tempCreature = static_cast<Creature*>(attackedObject);
        tempCreature->receiveExp(0.15 * expGained);

        // Add a bonus modifier based on the level of the creature we hit
        // to expGained and give ourselves that much experience.
        if (tempCreature->getLevel() >= getLevel())
            expGained *= 1.0 + (tempCreature->getLevel() - getLevel()) / 10.0;
        else
            expGained /= 1.0 + (getLevel() - tempCreature->getLevel()) / 10.0;
    }
    receiveExp(expGained);

    return false;
}

bool Creature::handleFightAction()
{
    // If worker
    if(mDefinition->isWorker())
    {
        if (mReachableEnemyCreatures.empty())
        {
            popAction();
            return true;
        }

        // We try to attack creatures
        bool canAttackObject = false;
        if (fightClosestObjectInList(mReachableEnemyCreatures, canAttackObject))
        {
            if(canAttackObject)
            {
                pushAction(CreatureAction::attackObject);
            }
            return canAttackObject;
        }

        // We should not come here.
        OD_ASSERT_TRUE(false);
        return false;
    }
    // If there are no more enemies which are reachable, stop fighting.
    if (mReachableEnemyObjects.empty())
    {
        popAction();
        return true;
    }

    // We try to attack creatures
    bool canAttackObject = false;
    if (!mReachableEnemyCreatures.empty() && fightClosestObjectInList(mReachableEnemyCreatures, canAttackObject))
    {
        if(canAttackObject)
        {
            pushAction(CreatureAction::attackObject);
        }
        return canAttackObject;
    }

    // If no creature, we attack the rest
    if (fightInRangeObjectInList(mReachableEnemyObjects, canAttackObject))
    {
        if(canAttackObject)
        {
            pushAction(CreatureAction::attackObject);
        }
        return canAttackObject;
    }

    // We should not come here.
    OD_ASSERT_TRUE(false);
    return false;
}

bool Creature::handleSleepAction()
{
    Tile* myTile = getPositionTile();
    if (mHomeTile == NULL)
    {
        popAction();
        return false;
    }

    if (myTile != mHomeTile)
    {
        // Walk to the the home tile.
        if (setDestination(mHomeTile))
            return false;
    }
    else
    {
        // We are at the home tile so sleep.
        setAnimationState("Sleep");
        // Improve awakeness
        mAwakeness += 1.5;
        if (mAwakeness > 100.0)
            mAwakeness = 100.0;
        // Improve HP but a bit slower.
        mHp += 1.0;
        if (mHp > mMaxHP)
            mHp = mMaxHP;

        if (mAwakeness >= 100.0 && mHp >= mMaxHP)
            popAction();
    }
    return false;
}

bool Creature::handleFleeAction()
{
    // We try to go as far as possible from the ennemies within visible tiles. We will quit flee mode when there will be no more
    // ennemy objects nearby or if we have already flee for too much time
    if ((mReachableEnemyObjects.empty()) || (mNbTurnAction > NB_TURN_FLEE_MAX))
    {
        popAction();
        return true;
    }

    // We try to go closer to the dungeon temple. If we are too near or if we cannot go there, we will flee randomly
    std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::RoomType::dungeonTemple, getSeat());
    tempRooms = getGameMap()->getReachableRooms(tempRooms, getPositionTile(), this);
    if(!tempRooms.empty())
    {
        // We can go to one dungeon temple
        Room* room = tempRooms[Random::Int(0, tempRooms.size() - 1)];
        Tile* tile = room->getCoveredTiles()[0];
        std::list<Tile*> result = getGameMap()->path(this, tile);
        // If we are not too near from the dungeon temple, we go there
        if(result.size() > 5)
        {
            result.resize(5);
            if (setWalkPath(result, 2, false))
            {
                setAnimationState("Flee");
                pushAction(CreatureAction::walkToTile);
                return true;
            }
        }
    }

    // No dungeon temple is acessible or we are too near. We will wander randomly
    wanderRandomly("Flee");
    return false;
}

double Creature::getMoveSpeed() const
{
    return getMoveSpeed(getPositionTile());
}

double Creature::getMoveSpeed(Tile* tile) const
{
    OD_ASSERT_TRUE(tile != nullptr);
    if(tile == nullptr)
        return 1.0;
    OD_ASSERT_TRUE_MSG(tile->getFullness() == 0, (getGameMap()->isServerGameMap()?std::string("1"):std::string("0"))
        + " creature=" + getName() + ",tile=" + Tile::displayAsString(tile)
        + ",Fullness=" + Ogre::StringConverter::toString(tile->getFullness()));

    switch(tile->getType())
    {
        case Tile::dirt:
        case Tile::gold:
        case Tile::claimed:
            return mGroundSpeed;
        case Tile::water:
            return mWaterSpeed;
        case Tile::lava:
            return mLavaSpeed;
        default:
            break;
    }

    return mGroundSpeed;
}

double Creature::getPhysicalDamage(double range)
{
    double hitroll = 0.0;

    if (mWeaponlessAtkRange >= range)
        hitroll += Random::Uint(1.0, mPhysicalAttack);

    if (mWeaponL != NULL && mWeaponL->getRange() >= range)
        hitroll += mWeaponL->getDamage();
    if (mWeaponR != NULL && mWeaponR->getRange() >= range)
        hitroll += mWeaponR->getDamage();

    return hitroll;
}

double Creature::getMagicalDamage(double range)
{
    double hitroll = 0.0;

    if (mWeaponlessAtkRange >= range)
        hitroll += Random::Uint(0.0, mMagicalAttack);

    // TODO: Add support for magical atk to equipment
    /*
    if (mWeaponL != NULL && mWeaponL->getRange() >= range)
        hitroll += mWeaponL->getDamage();
    if (mWeaponR != NULL && mWeaponR->getRange() >= range)
        hitroll += mWeaponR->getDamage();
    */

    return hitroll;
}

double Creature::getPhysicalDefense() const
{
    double defense = mPhysicalDefense;
    if (mWeaponL != NULL)
        defense += mWeaponL->getPhysicalDefense();
    if (mWeaponR != NULL)
        defense += mWeaponR->getPhysicalDefense();

    return defense;
}

double Creature::getMagicalDefense() const
{
    double defense = mMagicalDefense;

    // TODO: Add support for magical def to equipment
    /*
    if (mWeaponL != NULL)
        defense += mWeaponL->getDefense();
    if (mWeaponR != NULL)
        defense += mWeaponR->getDefense();
    */

    return defense;
}

double Creature::getBestAttackRange() const
{
    double range = mWeaponlessAtkRange;

    // Note: The damage check is here to avoid taking defense equipment in account.
    if (mWeaponL != nullptr && mWeaponL->getRange() > range && mWeaponL->getDamage() > 0.0)
        range = mWeaponL->getRange();
    if (mWeaponR != nullptr && mWeaponR->getRange() > range && mWeaponR->getDamage() > 0.0)
        range = mWeaponR->getRange();

    return range;
}

//! \brief Increases the creature's level, adds bonuses to stat points, changes the mesh, etc.
bool Creature::checkLevelUp()
{
    if (getLevel() >= MAX_LEVEL)
        return false;

    if (mExp < 5 * (getLevel() + std::pow(getLevel() / 3.0, 2)))
        return false;

    return true;
}

void Creature::refreshFromCreature(Creature *creatureNewState)
{
    // We save the actual level to check if there is a levelup
    unsigned int oldLevel = mLevel;
    // TODO : send a messageServerNotification::creatureRefresh each time we want
    // to refresh a creature (when loss HP from combat, level up or whatever).
    // The creature update should be here and the data should be transfered
    // in the transfert functions in this file using ODPacket
    mLevel          = creatureNewState->mLevel;
    mDigRate        = creatureNewState->mDigRate;
    mClaimRate      = creatureNewState->mClaimRate;
    mMaxHP          = creatureNewState->mMaxHP;
    mHp             = creatureNewState->mHp;
    mAwakeness      = creatureNewState->mAwakeness;
    mHunger         = creatureNewState->mHunger;

    mGroundSpeed    = creatureNewState->mGroundSpeed;
    mWaterSpeed     = creatureNewState->mWaterSpeed;
    mLavaSpeed      = creatureNewState->mLavaSpeed;

    mPhysicalAttack = creatureNewState->mPhysicalAttack;
    mMagicalAttack  = creatureNewState->mMagicalAttack;
    mPhysicalDefense = creatureNewState->mPhysicalDefense;
    mMagicalDefense = creatureNewState->mMagicalDefense;

    // Scale up the mesh.
    if ((oldLevel != getLevel()) && isMeshExisting() && ((getLevel() <= 30 && getLevel() % 2 == 0) || (getLevel() > 30 && getLevel()
            % 3 == 0)))
    {
        Ogre::Real scaleFactor = (Ogre::Real)(1.0 + static_cast<double>(getLevel()) / 250.0);
        if (scaleFactor > 1.03)
            scaleFactor = 1.04;

        RenderRequest *request = new RenderRequestScaleSceneNode(mSceneNode, Ogre::Vector3(scaleFactor, scaleFactor, scaleFactor));
        RenderManager::queueRenderRequest(request);
    }
}

void Creature::updateVisibleTiles()
{
    mVisibleTiles = getGameMap()->visibleTiles(getPositionTile(), mDefinition->getSightRadius());
}

std::vector<GameEntity*> Creature::getVisibleEnemyObjects()
{
    return getVisibleForce(getSeat(), true);
}

std::vector<GameEntity*> Creature::getReachableAttackableObjects(const std::vector<GameEntity*>& objectsToCheck)
{
    std::vector<GameEntity*> tempVector;
    Tile* myTile = getPositionTile();
    std::list<Tile*> tempPath;

    // Loop over the vector of objects we are supposed to check.
    for (unsigned int i = 0; i < objectsToCheck.size(); ++i)
    {
        // Try to find a valid path from the tile this creature is in to the nearest tile where the current target object is.
        GameEntity* entity = objectsToCheck[i];
        // We only consider alive objects
        if(entity->getHP(NULL) <= 0)
            continue;

        Tile* objectTile = entity->getCoveredTiles()[0];
        if (getGameMap()->pathExists(this, myTile, objectTile))
            tempVector.push_back(objectsToCheck[i]);
    }

    return tempVector;
}

std::vector<GameEntity*> Creature::getCreaturesFromList(const std::vector<GameEntity*> &objectsToCheck, bool koboldsOnly)
{
    std::vector<GameEntity*> tempVector;

    // Loop over the vector of objects we are supposed to check.
    for (std::vector<GameEntity*>::const_iterator it = objectsToCheck.begin(); it != objectsToCheck.end(); ++it)
    {
        // Try to find a valid path from the tile this creature is in to the nearest tile where the current target object is.
        GameEntity* entity = *it;
        // We only consider alive objects
        if(entity->getObjectType() != ObjectType::creature)
            continue;

        if(koboldsOnly && !static_cast<Creature*>(entity)->getDefinition()->isWorker())
            continue;

        tempVector.push_back(entity);
    }

    return tempVector;
}

std::vector<GameEntity*> Creature::getVisibleAlliedObjects()
{
    return getVisibleForce(getSeat(), false);
}

std::vector<Tile*> Creature::getVisibleMarkedTiles()
{
    std::vector<Tile*> tempVector;
    Player *tempPlayer = getGameMap()->getPlayerBySeat(getSeat());

    // Loop over all the visible tiles.
    for (unsigned int i = 0, size = mVisibleTiles.size(); i < size; ++i)
    {
        // Check to see if the tile is marked for digging.
        if (tempPlayer != NULL && mVisibleTiles[i]->getMarkedForDigging(tempPlayer))
            tempVector.push_back(mVisibleTiles[i]);
    }

    return tempVector;
}

std::vector<Tile*> Creature::getVisibleClaimableWallTiles()
{
    std::vector<Tile*> claimableWallTiles;

    // Loop over all the visible tiles.
    for (unsigned int i = 0, size = mVisibleTiles.size(); i < size; ++i)
    {
        // Check to see if the tile is marked for digging.
        if (mVisibleTiles[i]->isWallClaimable(getSeat()))
            claimableWallTiles.push_back(mVisibleTiles[i]);
    }

    return claimableWallTiles;
}

std::vector<GameEntity*> Creature::getVisibleForce(Seat* seat, bool invert)
{
    return getGameMap()->getVisibleForce(mVisibleTiles, seat, invert);
}

void Creature::createVisualDebugEntities()
{
    mHasVisualDebuggingEntities = true;
    mVisualDebugEntityTiles.clear();

    Tile *currentTile = NULL;
    updateVisibleTiles();
    for (unsigned int i = 0; i < mVisibleTiles.size(); ++i)
    {
        currentTile = mVisibleTiles[i];

        if (currentTile == NULL)
            continue;

        RenderRequest *request = new RenderRequestCreateCreatureVisualDebug(this, currentTile);
        RenderManager::queueRenderRequest(request);

        mVisualDebugEntityTiles.push_back(currentTile);
    }
}

void Creature::destroyVisualDebugEntities()
{
    mHasVisualDebuggingEntities = false;

    Tile *currentTile = NULL;
    updateVisibleTiles();
    std::list<Tile*>::iterator itr;
    for (itr = mVisualDebugEntityTiles.begin(); itr != mVisualDebugEntityTiles.end(); ++itr)
    {
        currentTile = *itr;

        if (currentTile == NULL)
            continue;

        RenderRequest *request = new RenderRequestDestroyCreatureVisualDebug(this, currentTile);
        RenderManager::queueRenderRequest(request);
    }

}

std::vector<Tile*> Creature::getCoveredTiles()
{
    std::vector<Tile*> tempVector;
    tempVector.push_back(getPositionTile());
    return tempVector;
}

bool Creature::CloseStatsWindow(const CEGUI::EventArgs& /*e*/)
{
    destroyStatsWindow();
    return true;
}

void Creature::createStatsWindow()
{
    if (mStatsWindow != NULL)
        return;

    ClientNotification *clientNotification = new ClientNotification(
        ClientNotification::askCreatureInfos);
    std::string name = getName();
    clientNotification->mPacket << name << true;
    ODClient::getSingleton().queueClientNotification(clientNotification);

    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
    CEGUI::Window* rootWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();

    mStatsWindow = wmgr->createWindow("OD/FrameWindow", std::string("CreatureStatsWindows_") + getName());
    mStatsWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0.3, 0), CEGUI::UDim(0.3, 0)));
    mStatsWindow->setSize(CEGUI::USize(CEGUI::UDim(0, 380), CEGUI::UDim(0, 350)));

    CEGUI::Window* textWindow = wmgr->createWindow("OD/StaticText", "TextDisplay");
    textWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0.05, 0), CEGUI::UDim(0.15, 0)));
    textWindow->setSize(CEGUI::USize(CEGUI::UDim(0.9, 0), CEGUI::UDim(0.8, 0)));
    textWindow->setProperty("FrameEnabled", "False");
    textWindow->setProperty("BackgroundEnabled", "False");

    // Search for the autoclose button and make it work
    CEGUI::Window* childWindow = mStatsWindow->getChild("__auto_closebutton__");
    childWindow->subscribeEvent(CEGUI::PushButton::EventClicked,
                                        CEGUI::Event::Subscriber(&Creature::CloseStatsWindow, this));

    // Set the window title
    childWindow = mStatsWindow->getChild("__auto_titlebar__");
    childWindow->setText(getName() + " (" + getDefinition()->getClassName() + ")");

    mStatsWindow->addChild(textWindow);
    rootWindow->addChild(mStatsWindow);
    mStatsWindow->show();

    updateStatsWindow("Loading...");
}

void Creature::destroyStatsWindow()
{
    if (mStatsWindow != NULL)
    {
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotification::askCreatureInfos);
        std::string name = getName();
        clientNotification->mPacket << name << false;
        ODClient::getSingleton().queueClientNotification(clientNotification);

        mStatsWindow->destroy();
        mStatsWindow = NULL;
    }
}

void Creature::updateStatsWindow(const std::string& txt)
{
    if (mStatsWindow == NULL)
        return;

    CEGUI::Window* textWindow = mStatsWindow->getChild("TextDisplay");
    textWindow->setText(txt);
}

std::string Creature::getStatsText()
{
    // The creatures are not refreshed at each turn so this information is relevant in the server
    // GameMap only
    std::stringstream tempSS;
    tempSS << "Level: " << getLevel() << std::endl;
    tempSS << "Experience: " << mExp << std::endl;
    tempSS << "HP: " << getHP() << " / " << mMaxHP << std::endl;
    if (!getDefinition()->isWorker())
    {
        tempSS << "Awakeness: " << mAwakeness << std::endl;
        tempSS << "Hunger: " << mHunger << std::endl;
    }
    tempSS << "Move speed (Ground/Water/Lava): " << getMoveSpeedGround() << "/"
        << getMoveSpeedWater() << "/" << getMoveSpeedLava() << std::endl;
    tempSS << "Attack (Phys/Mag): " << mPhysicalAttack << "/" << mMagicalAttack << std::endl;
    tempSS << "Weaponless Attack Range: " << mWeaponlessAtkRange << std::endl;
    tempSS << "Weapon:" << std::endl;
    tempSS << "Left hand: Attack: " << mWeaponL->getDamage() << ", Range: " << mWeaponL->getRange() << std::endl;
    tempSS << "Right hand: Attack: " << mWeaponR->getDamage() << ", Range: " << mWeaponR->getRange() << std::endl;
    tempSS << "Total Defense (Phys/Mag): " << getPhysicalDefense() << "/" << getMagicalDefense() << std::endl;
    if (getDefinition()->isWorker())
    {
        tempSS << "Dig Rate: : " << getDigRate() << std::endl;
        tempSS << "Dance Rate: : " << mClaimRate << std::endl;
    }
    tempSS << "Actions:";
    for(std::deque<CreatureAction>::iterator it = mActionQueue.begin(); it != mActionQueue.end(); ++it)
    {
        CreatureAction& ca = *it;
        tempSS << " " << ca.toString();
    }
    tempSS << std::endl;
    tempSS << "Destinations:";
    for(std::deque<Ogre::Vector3>::iterator it = mWalkQueue.begin(); it != mWalkQueue.end(); ++it)
    {
        Ogre::Vector3& dest = *it;
        tempSS << Ogre::StringConverter::toString(dest) << "/";
    }
    tempSS << std::endl;
    tempSS << "Seat id: " << getSeat()->getId() << std::endl;
    tempSS << "Team id: " << getSeat()->getTeamId() << std::endl;
    tempSS << "Position: " << Ogre::StringConverter::toString(getPosition()) << std::endl;
    return tempSS.str();
}

void Creature::takeDamage(GameEntity* attacker, double damage, Tile *tileTakingDamage)
{
    mHp -= damage;
    if(!getGameMap()->isServerGameMap())
        return;

    Player* player = getGameMap()->getPlayerBySeat(getSeat());
    if (player == NULL)
        return;

    // Tells the server game map the player is under attack.
    getGameMap()->playerIsFighting(player);

    try
    {
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::creatureRefresh, player);
        exportToPacket(serverNotification->mPacket);
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
    catch (std::bad_alloc&)
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }

    // If we are a worker attacked by a worker, we fight. Otherwise, we flee (if it is a fighter, a trap,
    // or whatever)
    if(!getDefinition()->isWorker())
        return;

    bool flee = true;
    if(attacker->getObjectType() == ObjectType::creature)
    {
        Creature* creatureAttacking = static_cast<Creature*>(attacker);
        if(creatureAttacking->getDefinition()->isWorker())
        {
            // We do not flee because of this attack
            flee = false;
        }
    }

    if(flee)
    {
        clearDestinations();
        clearActionQueue();
        pushAction(CreatureAction::flee);
        return;
    }
}

void Creature::receiveExp(double experience)
{
    if (experience < 0)
        return;

    mExp += experience;
}

bool Creature::getHasVisualDebuggingEntities()
{
    return mHasVisualDebuggingEntities;
}

bool Creature::isActionInList(CreatureAction::ActionType action)
{
    for (std::deque<CreatureAction>::iterator it = mActionQueue.begin(); it != mActionQueue.end(); ++it)
    {
        CreatureAction& ca = *it;
        if (ca.getType() == action)
            return true;
    }
    return false;
}

void Creature::clearActionQueue()
{
    mNbTurnAction = 0;
    mActionQueue.clear();
    stopJob();
    stopEating();
    mActionQueue.push_front(CreatureAction::idle);
}

void Creature::pushAction(CreatureAction action)
{
    mNbTurnAction = 0;
    mActionQueue.push_front(action);
}

void Creature::popAction()
{
    mNbTurnAction = 0;
    mActionQueue.pop_front();
}

CreatureAction Creature::peekAction()
{
    return mActionQueue.front();
}

bool Creature::tryPickup(Seat* seat, bool isEditorMode)
{
    if (getHP() <= 0.0)
        return false;

    if(!getIsOnMap())
        return false;

    if(!getSeat()->canOwnedCreatureBePickedUpBy(seat) && !isEditorMode)
        return false;

    return true;
}

void Creature::pickup()
{
    // Stop the creature walking and set it off the map to prevent the AI from running on it.
    setIsOnMap(false);
    clearDestinations();
    clearActionQueue();

    Tile* tile = getPositionTile();
    if(tile != NULL)
        tile->removeCreature(this);
}

bool Creature::canGoThroughTile(const Tile* tile) const
{
    if(tile == nullptr || tile->getFullness() > 0)
        return false;

    switch(tile->getType())
    {
        case Tile::dirt:
        case Tile::gold:
        case Tile::claimed:
        {
            if(mGroundSpeed > 0.0)
                return true;

            break;
        }
        case Tile::water:
        {
            if(mWaterSpeed > 0.0)
                return true;

            break;
        }
        case Tile::lava:
        {
            if(mLavaSpeed > 0.0)
                return true;

            break;
        }
        default:
            return false;
    }
    return false;
}

bool Creature::tryDrop(Seat* seat, Tile* tile, bool isEditorMode)
{
    // check whether the tile is a ground tile ...
    if (tile->getFullness() > 0.0)
        return false;

    // In editor mode, we allow creatures to be dropped anywhere they can walk
    if(isEditorMode && canGoThroughTile(tile))
        return true;

    // If it is a worker, he can be dropped on dirt
    if (getDefinition()->isWorker() && (tile->getType() == Tile::dirt || tile->getType() == Tile::gold))
        return true;

    // Every creature can be dropped on allied claimed tiles
    if(tile->getType() == Tile::claimed && tile->getSeat() != NULL && tile->getSeat()->isAlliedSeat(getSeat()))
        return true;

    return false;
}

void Creature::playSound(CreatureSound::SoundType soundType)
{
    Ogre::Vector3 pos = getPosition();
    mSound->play(soundType, pos.x, pos.y, pos.z);
}

bool Creature::setDestination(Tile* tile)
{
    if(tile == NULL)
        return false;

    Tile *posTile = getPositionTile();
    if(posTile == NULL)
        return false;

    std::list<Tile*> result = getGameMap()->path(this, tile);

    if (setWalkPath(result, 2, false))
    {
        setAnimationState("Walk");
        pushAction(CreatureAction::walkToTile);
        return true;
    }
    return false;
}

bool Creature::fightClosestObjectInList(const std::vector<GameEntity*>& listObjects, bool& canAttackObject)
{
    canAttackObject = false;
    if(listObjects.empty())
        return false;

    // We check if we are at the best range of our foe. That will allow ranged units to hit and run
    Tile* tileCreature = getPositionTile();
    if(tileCreature == NULL)
        return false;

    // We try to find the closest enemy object
    Tile* attackedTile = NULL;
    GameEntity* attackedObject = getGameMap()->getClosestTileWhereGameEntityFromList(listObjects, tileCreature, attackedTile);
    if(attackedObject == NULL)
        return false;

    // Now that we found the closest enemy, we move to attack
    std::list<Tile*> tempPath;
    if(!getGameMap()->pathToBestFightingPosition(tempPath, this, attackedTile))
    {
        // We couldn't find a way to the foe. We wander somewhere else
        popAction();
        wanderRandomly("Walk");
        return true;
    }

    // If we are already on the good tile, we can attack
    if(tempPath.empty())
    {
        // We are in a good spot. We can attack
        canAttackObject = true;
        mAttackedTile = attackedTile;
        mAttackedObject = attackedObject;
        return true;
    }

    // We have to move to the attacked tile. If we are 1 tile from our foe (tempPath contains 2 values), before
    // moving, we check if he is moving to the same tile as we are. If yes, we don't move
    // to avoid 2 creatures going to each others tiles for ages
    if((tempPath.size() == 2) && (attackedObject->getObjectType() == ObjectType::creature))
    {
        Creature* attackedCreature = static_cast<Creature*>(attackedObject);
        if(!attackedCreature->mWalkQueue.empty())
        {
            Ogre::Vector3 attackedCreatureDest = attackedCreature->mWalkQueue.front();
            int x = static_cast<int>(std::round(attackedCreatureDest.x));
            int y = static_cast<int>(std::round(attackedCreatureDest.y));
            Tile* tileAttackedCreatureDest = getGameMap()->getTile(x, y);
            Tile* tileDest = tempPath.back();
            if(tileAttackedCreatureDest == tileDest)
            {
                // We are going to the same tile. We do nothing
                return true;
            }
        }
    }

    if (setWalkPath(tempPath, 1, false))
    {
        setAnimationState("Walk");
        pushAction(CreatureAction::walkToTile);
    }

    return true;
}

bool Creature::fightInRangeObjectInList(const std::vector<GameEntity*>& listObjects, bool& canAttackObject)
{
    canAttackObject = false;
    if(listObjects.empty())
        return false;

    // We check if we are at the best range of our foe. That will allow ranged units to hit and run
    Tile* tileCreature = getPositionTile();
    if(tileCreature == NULL)
        return false;

    // We try to find the closest enemy object within attack range
    GameEntity* closestEnnemyEntity = nullptr;
    Tile* closestEnnemyTile = nullptr;
    double closestEnnemyDist = 0.0;
    Tile* closestNotInRangeEnnemyTile = nullptr;
    double closestNotInRangeEnnemyDist = 0.0;

    // Use the weapon range when equipped, and the natural one when not.
    double weaponRangeSquared = getBestAttackRange();
    weaponRangeSquared *= weaponRangeSquared;

    // Loop over the enemyObjectsToCheck and add any within range to the tempVector.
    for (std::vector<GameEntity*>::const_iterator it = listObjects.begin(); it != listObjects.end(); ++it)
    {
        GameEntity* gameEntity = *it;
        std::vector<Tile*> tiles = gameEntity->getCoveredTiles();
        for(std::vector<Tile*>::iterator itTile = tiles.begin(); itTile != tiles.end(); ++itTile)
        {
            Tile *tempTile = *itTile;
            if (tempTile == NULL)
                continue;

            double rSquared = std::pow(tileCreature->x - tempTile->x, 2.0) + std::pow(
                    tileCreature->y - tempTile->y, 2.0);

            if (rSquared <= weaponRangeSquared)
            {
                if((closestEnnemyTile == nullptr) ||
                   (rSquared < closestEnnemyDist))
                {
                    closestEnnemyDist = rSquared;
                    closestEnnemyTile = tempTile;
                    closestEnnemyEntity = gameEntity;
                }
            }
            else
            {
                if((closestNotInRangeEnnemyTile == nullptr) ||
                   (rSquared < closestNotInRangeEnnemyDist))
                {
                    closestNotInRangeEnnemyDist = rSquared;
                    closestNotInRangeEnnemyTile = tempTile;
                }
            }
        }
    }

    if(closestEnnemyEntity != nullptr)
    {
        mAttackedObject = closestEnnemyEntity;
        mAttackedTile = closestEnnemyTile;
        canAttackObject = true;
        return true;
    }

    // There is no ennemy in range. We move to the closest non reachable
    std::list<Tile*> tempPath;
    if(!getGameMap()->pathToBestFightingPosition(tempPath, this, closestNotInRangeEnnemyTile))
    {
        // We couldn't find a way to the foe. We wander somewhere else
        popAction();
        wanderRandomly("Walk");
        return true;
    }

    if (setWalkPath(tempPath, 1, false))
    {
        setAnimationState("Walk");
        pushAction(CreatureAction::walkToTile);
    }

    return true;
}

bool Creature::wanderRandomly(const std::string& animationState)
{
    // We pick randomly a visible tile far away (at the end of visible tiles)
    if(mVisibleTiles.empty())
        return false;

    Tile* tileDestination = NULL;
    int minPick = mVisibleTiles.size() * 4 / 5;
    int indexPick = Random::Int(minPick, mVisibleTiles.size() - 1);
    while((tileDestination == NULL) && (indexPick > 0))
    {
        Tile* tile = mVisibleTiles[indexPick];
        if(getGameMap()->pathExists(this, getPositionTile(), tile))
            tileDestination = tile;

        --indexPick;
    }

    if(tileDestination == NULL)
        return false;

    std::list<Tile*> result = getGameMap()->path(this, tileDestination);
    if (setWalkPath(result, 1, false))
    {
        setAnimationState(animationState);
        pushAction(CreatureAction::walkToTile);
        return true;
    }

    return false;
}

bool Creature::isInBadMood()
{
    // TODO : set bad mood depending on the creature stats
    return (mAwakeness < 10.0 && mHunger > 90.0);
}

bool Creature::isAttackable() const
{
    if(mHp <= 0.0)
        return false;

    return true;
}
