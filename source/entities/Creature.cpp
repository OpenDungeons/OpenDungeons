/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "entities/Creature.h"

#include "creaturemood/CreatureMood.h"

#include "entities/CreatureAction.h"
#include "entities/Weapon.h"
#include "entities/CreatureSound.h"
#include "entities/TreasuryObject.h"
#include "entities/ChickenEntity.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "network/ODClient.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"

#include "render/RenderManager.h"

#include "rooms/RoomCrypt.h"
#include "rooms/RoomTreasury.h"
#include "rooms/RoomDormitory.h"

#include "sound/SoundEffectsManager.h"

#include "spell/Spell.h"

#include "utils/ConfigManager.h"
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

//TODO: make this read from definition file?
static const int MaxGoldCarriedByWorkers = 1500;
static const int NB_TURN_FLEE_MAX = 5;

const std::string Creature::CREATURE_PREFIX = "Creature_";

Creature::Creature(GameMap* gameMap, const CreatureDefinition* definition) :
    MovableGameEntity        (gameMap),
    mPhysicalAttack          (1.0),
    mMagicalAttack           (0.0),
    mPhysicalDefense         (3.0),
    mMagicalDefense          (1.5),
    mWeaponlessAtkRange      (1.0),
    mAttackWarmupTime        (1.0),
    mWeaponL                 (nullptr),
    mWeaponR                 (nullptr),
    mHomeTile                (nullptr),
    mDefinition              (definition),
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
    mDeathCounter            (0),
    mJobCooldown             (0),
    mEatCooldown             (0),
    mGoldFee                 (0),
    mGoldCarried             (0),
    mJobRoom                 (nullptr),
    mEatRoom                 (nullptr),
    mStatsWindow             (nullptr),
    mForceAction             (forcedActionNone),
    mCarriedEntity           (nullptr),
    mCarriedEntityDestType   (GameEntity::ObjectType::unknown),
    mMoodCooldownTurns       (0),
    mMoodValue               (CreatureMoodLevel::Neutral),
    mMoodPoints              (0),
    mFirstTurnFurious        (-1)

{
    setName(getGameMap()->nextUniqueNameCreature(definition->getClassName()));

    setIsOnMap(false);

    pushAction(CreatureAction::idle, true);

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

    if(mDefinition->getWeaponSpawnL().compare("none") != 0)
        mWeaponL = gameMap->getWeapon(mDefinition->getWeaponSpawnL());

    if(mDefinition->getWeaponSpawnR().compare("none") != 0)
        mWeaponR = gameMap->getWeapon(mDefinition->getWeaponSpawnR());
}

Creature::Creature(GameMap* gameMap) :
    MovableGameEntity        (gameMap),
    mPhysicalAttack          (1.0),
    mMagicalAttack           (0.0),
    mPhysicalDefense         (3.0),
    mMagicalDefense          (1.5),
    mWeaponlessAtkRange      (1.0),
    mAttackWarmupTime        (1.0),
    mWeaponL                 (nullptr),
    mWeaponR                 (nullptr),
    mHomeTile                (nullptr),
    mDefinition              (nullptr),
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
    mDeathCounter            (0),
    mJobCooldown             (0),
    mEatCooldown             (0),
    mGoldFee                 (0),
    mGoldCarried             (0),
    mJobRoom                 (nullptr),
    mEatRoom                 (nullptr),
    mStatsWindow             (nullptr),
    mForceAction             (forcedActionNone),
    mCarriedEntity           (nullptr),
    mCarriedEntityDestType   (GameEntity::ObjectType::unknown),
    mMoodCooldownTurns       (0),
    mMoodValue               (CreatureMoodLevel::Neutral),
    mMoodPoints              (0),
    mFirstTurnFurious        (-1)
{
    setIsOnMap(false);

    pushAction(CreatureAction::idle, true);
}

Creature::~Creature()
{
}

void Creature::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();
    if(!getGameMap()->isServerGameMap())
    {
        RenderManager::getSingleton().rrCreateCreature(this);

        // By default, we set the creature in idle state
        RenderManager::getSingleton().rrSetObjectAnimationState(this, "Idle", true);
    }

    createMeshWeapons();
}

void Creature::destroyMeshLocal()
{
    destroyMeshWeapons();
    MovableGameEntity::destroyMeshLocal();
    if(getGameMap()->isServerGameMap())
        return;

    destroyStatsWindow();
    RenderManager::getSingleton().rrDestroyCreature(this);
}

void Creature::createMeshWeapons()
{
    if(getGameMap()->isServerGameMap())
        return;

    if(mWeaponL != nullptr)
        RenderManager::getSingleton().rrCreateWeapon(this, mWeaponL, "L");

    if(mWeaponR != nullptr)
        RenderManager::getSingleton().rrCreateWeapon(this, mWeaponR, "R");
}

void Creature::destroyMeshWeapons()
{
    if(getGameMap()->isServerGameMap())
        return;

    if(mWeaponL != nullptr)
        RenderManager::getSingleton().rrDestroyWeapon(this, mWeaponL, "L");

    if(mWeaponR != nullptr)
        RenderManager::getSingleton().rrDestroyWeapon(this, mWeaponR, "R");
}

void Creature::addToGameMap()
{
    getGameMap()->addCreature(this);
}

void Creature::removeFromGameMap()
{
    getGameMap()->removeCreature(this);
}


std::string Creature::getFormat()
{
    //NOTE:  When this format changes, other changes to RoomPortal::spawnCreature() may be necessary.
    return "SeatId\tClassName\tName\tLevel\tCurrentXP\tCurrentHP\tCurrentAwakeness\t"
           "CurrentHunger\tGoldToDeposit\tPosX\tPosY\tPosZ\tLeftWeapon\tRightWeapon";
}

void Creature::exportToStream(std::ostream& os) const
{
    int seatId = getSeat()->getId();
    os << seatId;
    os << "\t" << mDefinition->getClassName() << "\t" << getName();
    os << "\t" << getLevel() << "\t" << mExp << "\t";
    if(getHP() < mMaxHP)
        os << getHP();
    else
        os << "max";
    os << "\t" << mAwakeness << "\t" << mHunger << "\t" << mGoldCarried;

    os << "\t" << mPosition.x;
    os << "\t" << mPosition.y;
    os << "\t" << mPosition.z;

    // Check creature weapons
    if(mWeaponL != nullptr)
        os << "\t" << mWeaponL->getName();
    else
        os << "\tnone";

    if(mWeaponR != nullptr)
        os << "\t" << mWeaponR->getName();
    else
        os << "\tnone";

    MovableGameEntity::exportToStream(os);
}

void Creature::importFromStream(std::istream& is)
{
    Ogre::Real xLocation = 0.0, yLocation = 0.0, zLocation = 0.0;
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

    // name
    OD_ASSERT_TRUE(is >> tempString);
    if (tempString.compare("autoname") == 0)
        tempString = getGameMap()->nextUniqueNameCreature(mDefinition->getClassName());
    setName(tempString);

    OD_ASSERT_TRUE(is >> mLevel);

    OD_ASSERT_TRUE(is >> mExp);

    std::string strHp;
    OD_ASSERT_TRUE(is >> strHp);

    OD_ASSERT_TRUE(is >> tempDouble);
    mAwakeness = tempDouble;

    OD_ASSERT_TRUE(is >> tempDouble);
    mHunger = tempDouble;

    OD_ASSERT_TRUE(is >> tempDouble);
    mGoldCarried = static_cast<int>(tempDouble);

    OD_ASSERT_TRUE(is >> xLocation >> yLocation >> zLocation);
    mPosition = Ogre::Vector3(xLocation, yLocation, zLocation);

    OD_ASSERT_TRUE(is >> tempString);
    if(tempString != "none")
    {
        mWeaponL = getGameMap()->getWeapon(tempString);
        OD_ASSERT_TRUE_MSG(mWeaponL != nullptr, "Unknown weapon name=" + tempString);
    }

    OD_ASSERT_TRUE(is >> tempString);
    if(tempString != "none")
    {
        mWeaponR = getGameMap()->getWeapon(tempString);
        OD_ASSERT_TRUE_MSG(mWeaponR != nullptr, "Unknown weapon name=" + tempString);
    }
    mLevel = std::min(MAX_LEVEL, mLevel);

    MovableGameEntity::importFromStream(is);

    buildStats();

    if(strHp.compare("max") == 0)
        mHp = mMaxHP;
    else
        mHp = Helper::toDouble(strHp);
}

void Creature::buildStats()
{
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

    mScale = getDefinition()->getScale();
    Ogre::Real scaleFactor = static_cast<Ogre::Real>(1.0 + 0.02 * static_cast<double>(getLevel()));
    mScale *= scaleFactor;

    // Improve the stats to the current level
    double multiplier = mLevel - 1;
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

void Creature::exportToPacket(ODPacket& os) const
{
    const std::string& className = mDefinition->getClassName();
    os << className;

    const std::string& name = getName();
    os << name;

    int seatId = getSeat()->getId();
    os << mPosition;
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

    if(mWeaponL != nullptr)
        os << mWeaponL->getName();
    else
        os << "none";

    if(mWeaponR != nullptr)
        os << mWeaponR->getName();
    else
        os << "none";

    MovableGameEntity::exportToPacket(os);
}

void Creature::importFromPacket(ODPacket& is)
{
    std::string tempString;

    OD_ASSERT_TRUE(is >> tempString);
    mDefinition = getGameMap()->getClassDescription(tempString);
    OD_ASSERT_TRUE_MSG(mDefinition != nullptr, "Definition=" + tempString);

    OD_ASSERT_TRUE(is >> tempString);
    setName(tempString);

    OD_ASSERT_TRUE(is >> mPosition);

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

    OD_ASSERT_TRUE(is >> tempString);
    if(tempString != "none")
    {
        mWeaponL = getGameMap()->getWeapon(tempString);
        OD_ASSERT_TRUE_MSG(mWeaponL != nullptr, "Unknown weapon name=" + tempString);
    }

    OD_ASSERT_TRUE(is >> tempString);
    if(tempString != "none")
    {
        mWeaponR = getGameMap()->getWeapon(tempString);
        OD_ASSERT_TRUE_MSG(mWeaponR != nullptr, "Unknown weapon name=" + tempString);
    }

    MovableGameEntity::importFromPacket(is);

    buildStats();
}

void Creature::setPosition(const Ogre::Vector3& v, bool isMove)
{
    MovableGameEntity::setPosition(v, isMove);
}

void Creature::drop(const Ogre::Vector3& v)
{
    setIsOnMap(true);
    setPosition(v, false);
    mForceAction = forcedActionSearchAction;
    if(getHasVisualDebuggingEntities())
        computeVisualDebugEntities();
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

void Creature::update(Ogre::Real timeSinceLastFrame)
{
    Tile* previousPositionTile = getPositionTile();
    // Update movements, direction, ...
    MovableGameEntity::update(timeSinceLastFrame);

    // Update the visual debugging entities
    //if we are standing in a different tile than we were last turn
    if (mHasVisualDebuggingEntities &&
        getGameMap()->isServerGameMap() &&
        (getPositionTile() != previousPositionTile))
    {
        computeVisualDebugEntities();
    }

    if (getGameMap()->isServerGameMap())
    {
        // Reduce the attack warmup time left for creatures on the server side
        // When they are attacking
        if (mAttackWarmupTime > 0.0)
            mAttackWarmupTime -= timeSinceLastFrame;
    }
}

void Creature::computeVisibleTiles()
{
    if (getHP() <= 0.0)
        return;

    if (!getIsOnMap())
        return;

    // Look at the surrounding area
    updateTilesInSight();
    for(Tile* tile : mVisibleTiles)
        tile->notifyVision(getSeat());
}

void Creature::setLevel(unsigned int level)
{
    // Reset XP once the level has been acquired.
    mLevel = std::min(MAX_LEVEL, level);
    mExp = 0.0;

    if(!getGameMap()->isServerGameMap())
    {
        refreshCreature();
        return;
    }

    buildStats();

    fireCreatureRefresh();
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
        if (mDeathCounter == 0)
        {
            stopJob();
            stopEating();
            clearDestinations();
            setAnimationState("Die", false);

            // If we are carrying gold, we drop it
            Tile* myTile = getPositionTile();
            OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
            if(myTile != nullptr && mGoldCarried > 0)
            {
                TreasuryObject* obj = new TreasuryObject(getGameMap(), mGoldCarried);
                obj->addToGameMap();
                Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(myTile->getX()),
                                            static_cast<Ogre::Real>(myTile->getY()), 0.0f);
                obj->createMesh();
                obj->setPosition(spawnPosition, false);
            }
        }
        else if (mDeathCounter >= ConfigManager::getSingleton().getCreatureDeathCounter())
        {
            // If the creature has a homeTile where it sleeps, its bed needs to be destroyed.
            if (getHomeTile() != 0)
            {
                RoomDormitory* home = static_cast<RoomDormitory*>(getHomeTile()->getCoveringBuilding());
                home->releaseTileForSleeping(getHomeTile(), this);
            }

            // Remove the creature from the game map and into the deletion queue, it will be deleted
            // when it is safe, i.e. all other pointers to it have been wiped from the program.
            removeFromGameMap();
            deleteYourself();
        }

        ++mDeathCounter;
        return;
    }

    // If we are not standing somewhere on the map, do nothing.
    if (getPositionTile() == nullptr)
        return;

    // Check to see if we have earned enough experience to level up.
    if(checkLevelUp())
        setLevel(mLevel + 1);

    // Heal.
    mHp += mDefinition->getHpHealPerTurn();
    if (mHp > getMaxHp())
        mHp = getMaxHp();

    //Rogue creatures are not affected by awakness/hunger
    if(!getSeat()->isRogueSeat())
    {
        decreaseAwakeness(mDefinition->getAwakenessLostPerTurn());

        increaseHunger(mDefinition->getHungerGrowthPerTurn());
    }

    mVisibleEnemyObjects         = getVisibleEnemyObjects();
    mReachableEnemyObjects       = getReachableAttackableObjects(mVisibleEnemyObjects);
    mReachableEnemyCreatures     = getCreaturesFromList(mReachableEnemyObjects, getDefinition()->isWorker());
    mVisibleAlliedObjects        = getVisibleAlliedObjects();
    mReachableAlliedObjects      = getReachableAttackableObjects(mVisibleAlliedObjects);

    if (mDigRate > 0.0)
        updateVisibleMarkedTiles();

    // Check if we should compute mood
    if(mMoodCooldownTurns > 0)
    {
        --mMoodCooldownTurns;
    }
    // Rogue creatures do not have mood
    else if(!getSeat()->isRogueSeat())
    {
        computeMood();
        mMoodCooldownTurns = Random::Int(0, 5);
    }

    decidePrioritaryAction();

    // The loopback variable allows creatures to begin processing a new
    // action immediately after some other action happens.
    bool loopBack = false;
    unsigned int loops = 0;

    mActionTry.clear();

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
                    loopBack = handleIdleAction(topActionItem);
                    break;

                case CreatureAction::walkToTile:
                    loopBack = handleWalkToTileAction(topActionItem);
                    break;

                case CreatureAction::claimTile:
                    loopBack = handleClaimTileAction(topActionItem);
                    break;

                case CreatureAction::claimWallTile:
                    loopBack = handleClaimWallTileAction(topActionItem);
                    break;

                case CreatureAction::digTile:
                    loopBack = handleDigTileAction(topActionItem);
                    break;

                case CreatureAction::findHome:
                case CreatureAction::findHomeForced:
                    loopBack = handleFindHomeAction(topActionItem);
                    break;

                case CreatureAction::sleep:
                    loopBack = handleSleepAction(topActionItem);
                    break;

                case CreatureAction::jobdecided:
                case CreatureAction::jobforced:
                    loopBack = handleJobAction(topActionItem);
                    break;

                case CreatureAction::eatdecided:
                case CreatureAction::eatforced:
                    loopBack = handleEatingAction(topActionItem);
                    break;

                case CreatureAction::attackObject:
                    loopBack = handleAttackAction(topActionItem);
                    break;

                case CreatureAction::fight:
                    loopBack = handleFightAction(topActionItem);
                    break;

                case CreatureAction::flee:
                    loopBack = handleFleeAction(topActionItem);
                    break;

                case CreatureAction::carryEntity:
                    loopBack = handleCarryableEntities(topActionItem);
                    break;

                case CreatureAction::getFee:
                    loopBack = handleGetFee(topActionItem);
                    break;

                case CreatureAction::leaveDungeon:
                    loopBack = handleLeaveDungeon(topActionItem);
                    break;

                case CreatureAction::fightNaturalEnemy:
                    loopBack = handleFightAlliedNaturalEnemyAction(topActionItem);
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

    for(CreatureAction& creatureAction : mActionQueue)
        creatureAction.increaseNbTurn();

    if(loops >= 20)
    {
        LogManager::getSingleton().logMessage("> 20 loops in Creature::doUpkeep name:" + getName() +
                " seat id: " + Ogre::StringConverter::toString(getSeat()->getId()) + ". Breaking out..");
    }
}

void Creature::decidePrioritaryAction()
{
    // Here, we should decide prioritary actions only (like fighting when an enemy is
    // visible). And if we decide to do something, we should clear the action queue.

    // If a creature is weak and there are foes, it shall flee
    bool isWeak = (mHp < mMaxHP * mDefinition->getWeakCoef());
    if (!mReachableEnemyObjects.empty() && isWeak)
    {
        if(isActionInList(CreatureAction::flee))
            return;

        clearDestinations();
        clearActionQueue();
        pushAction(CreatureAction::flee, true);
        return;
    }

    // If we are weak we do not attack
    if (isWeak)
        return;

    // If a fighter can see enemies that are reachable, he may attack
    if (!mReachableEnemyObjects.empty() && !mDefinition->isWorker())
    {
        // Check if we are already fighting
        if(isActionInList(CreatureAction::fight) || isActionInList(CreatureAction::flee))
            return;

        // Unhappy creatures might flee instead of engaging enemies
        switch(mMoodValue)
        {
            case CreatureMoodLevel::Angry:
            case CreatureMoodLevel::Furious:
            {
                if(Random::Int(0,100) > 80)
                {
                    clearDestinations();
                    clearActionQueue();
                    pushAction(CreatureAction::flee, true);
                    return;
                }
                break;
            }
            default:
                break;
        }

        // If we are not already fighting with a creature then start doing so.
        clearDestinations();
        clearActionQueue();
        pushAction(CreatureAction::fight, true);
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
        pushAction(CreatureAction::fight, true);
        return;
    }

    // Unhappy creatures might engage allied natural enemies
    switch(mMoodValue)
    {
        case CreatureMoodLevel::Upset:
        case CreatureMoodLevel::Angry:
        case CreatureMoodLevel::Furious:
        {
            // We are fighting an allied enemy. We don't consider leaving yet
            if(isActionInList(CreatureAction::fightNaturalEnemy))
                return;

            // We check if we can attack a natural enemy
            if(Random::Int(0, 100) > 80)
            {
                clearDestinations();
                clearActionQueue();
                stopJob();
                stopEating();
                pushAction(CreatureAction::fightNaturalEnemy, true);
                return;
            }

            // If we are furious, we consider leaving the dungeon
            if((mMoodValue != CreatureMoodLevel::Furious) ||
               (isActionInList(CreatureAction::leaveDungeon)))
            {
                return;
            }

            clearDestinations();
            clearActionQueue();
            stopJob();
            stopEating();
            pushAction(CreatureAction::leaveDungeon, true);
            break;
        }
        default:
            break;
    }
}

bool Creature::handleIdleAction(const CreatureAction& actionItem)
{
    double diceRoll = Random::Double(0.0, 1.0);

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
        Tile* tileMarkedDig = nullptr;
        Tile* tileToClaim = nullptr;
        Tile* tileWallNotClaimed = nullptr;
        for (Tile* tile : position->getAllNeighbors())
        {
            if(tileMarkedDig == nullptr &&
                tile->getMarkedForDigging(getGameMap()->getPlayerBySeat(seat))
                )
            {
                tileMarkedDig = tile;
            }
            else if(tileToClaim == nullptr &&
                tile->getType() == Tile::claimed &&
                tile->isClaimedForSeat(seat) &&
                position->isGroundClaimable() &&
                !position->isClaimedForSeat(seat)
                )
            {
                tileToClaim = position;
            }
            else if(tileToClaim == nullptr &&
                position->getType() == Tile::claimed &&
                position->isClaimedForSeat(seat) &&
                tile->isGroundClaimable() &&
                !tile->isClaimedForSeat(seat)
                )
            {
                tileToClaim = tile;
            }
            else if(tileWallNotClaimed == nullptr &&
                position->getType() == Tile::claimed &&
                position->isClaimedForSeat(seat) &&
                tile->isWallClaimable(seat)
                )
            {
                tileWallNotClaimed = tile;
            }
        }

        if((mGoldCarried > 0) && (mDigRate > 0.0) &&
           (position->getCoveringRoom() != nullptr) &&
           (position->getCoveringRoom()->getType() == Room::treasury))
        {
            RoomTreasury* treasury = static_cast<RoomTreasury*>(position->getCoveringRoom());
            int deposited = treasury->depositGold(mGoldCarried, position);
            if(deposited > 0)
            {
                mGoldCarried -= deposited;
                return true;
            }
        }

        std::vector<MovableGameEntity*> carryable;
        position->fillWithCarryableEntities(carryable);
        bool forceCarryObject = false;
        if(!carryable.empty())
            forceCarryObject = true;

        // Now, we can decide
        if((tileMarkedDig != nullptr) && (mDigRate > 0.0))
        {
            mForceAction = forcedActionDigTile;
        }
        else if((tileToClaim != nullptr) && (mClaimRate > 0.0))
        {
            mForceAction = forcedActionClaimTile;
        }
        else if((tileWallNotClaimed != nullptr) && (mClaimRate > 0.0))
        {
            mForceAction = forcedActionClaimWallTile;
        }
        else if(forceCarryObject)
        {
            mForceAction = forcedActionNone;
            pushAction(CreatureAction::carryEntity, true);
            return true;
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
                pushAction(CreatureAction::digTile, true);
                return true;
            }
            case forcedActionClaimTile:
            {
                pushAction(CreatureAction::claimTile, true);
                return true;
            }
            case forcedActionClaimWallTile:
            {
                pushAction(CreatureAction::claimWallTile, true);
                return true;
            }
            default:
                break;
        }
    }

    // Decide to check for diggable tiles
    if (mDigRate > 0.0 && !mVisibleMarkedTiles.empty())
    {
        if(pushAction(CreatureAction::digTile))
            return true;
    }
    // Decide to check for dead creature to carry to the crypt
    if (mDefinition->isWorker() && diceRoll < 0.3)
    {
        if(pushAction(CreatureAction::carryEntity))
            return true;
    }
    // Decide to check for claimable tiles
    if (mClaimRate > 0.0 && diceRoll < 0.9)
    {
        if(pushAction(CreatureAction::claimTile))
            return true;
    }

    // Fighters
    bool isWeak = (mHp < mMaxHP * mDefinition->getWeakCoef());
    // If a fighter is weak, he should try to sleep
    if (isWeak && !mDefinition->isWorker())
    {
        if((mHomeTile != nullptr) && (getGameMap()->pathExists(this, getPositionTile(), mHomeTile)))
        {
            if(pushAction(CreatureAction::sleep))
                return true;
        }

        // If we have no home tile, we try to find one
        if(mHomeTile == nullptr)
        {
            std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::dormitory, getSeat());
            tempRooms = getGameMap()->getReachableRooms(tempRooms, getPositionTile(), this);
            if (!tempRooms.empty())
            {
                clearDestinations();
                clearActionQueue();
                if(pushAction(CreatureAction::sleep) && pushAction(CreatureAction::findHome))
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
        if((tile != nullptr) &&
           (tile->getCoveringRoom() != nullptr))
        {
            Room* room = tile->getCoveringRoom();
            // we see if we are in an hatchery
            if(room->getType() == Room::hatchery)
            {
                pushAction(CreatureAction::eatforced, true);
                return true;
            }
            else if(room->getType() == Room::dormitory)
            {
                pushAction(CreatureAction::sleep, true);
                pushAction(CreatureAction::findHomeForced, true);
                return true;
            }
            // If not, can we work in this room ?
            else if(room->getType() != Room::hatchery)
            {
                pushAction(CreatureAction::jobforced, true);
                return true;
            }
        }
    }

    // We check if we are looking for our fee
    if(!mDefinition->isWorker() && Random::Double(0.0, 1.0) < 0.5 && mGoldFee > 0)
    {
        if(pushAction(CreatureAction::getFee))
            return true;
    }

    // We check if there is a go to war spell reachable
    std::vector<Spell*> callToWars = getGameMap()->getSpellsBySeatAndType(getSeat(), SpellType::callToWar);
    if(!callToWars.empty())
    {
        std::vector<Spell*> reachableCallToWars;
        for(Spell* callToWar : callToWars)
        {
            if(!callToWar->getIsOnMap())
                continue;

            Tile* callToWarTile = callToWar->getPositionTile();
            if(callToWarTile == nullptr)
                continue;

            if (!getGameMap()->pathExists(this, getPositionTile(), callToWarTile))
                continue;

            reachableCallToWars.push_back(callToWar);
        }

        if(!reachableCallToWars.empty())
        {
            // We go there
            uint32_t index = Random::Uint(0,reachableCallToWars.size()-1);
            Spell* callToWar = reachableCallToWars[index];
            Tile* callToWarTile = callToWar->getPositionTile();
            std::list<Tile*> tempPath = getGameMap()->path(this, callToWarTile);
            // If we are 5 tiles from the call to war, we don't go there
            if (setWalkPath(tempPath, 5, false))
            {
                setAnimationState("Walk");
                pushAction(CreatureAction::walkToTile, true);
                return false;
            }
        }
    }

    // Check to see if we have found a "home" tile where we can sleep. Even if we are not sleepy,
    // we want to have a bed
    if (!mDefinition->isWorker() && mHomeTile == nullptr && Random::Double(0.0, 1.0) < 0.5)
    {
        // Check to see if there are any dormitory owned by our color that we can reach.
        std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::dormitory, getSeat());
        tempRooms = getGameMap()->getReachableRooms(tempRooms, getPositionTile(), this);
        if (!tempRooms.empty())
        {
            if(pushAction(CreatureAction::findHome))
                return true;
        }
    }

    // If we are sleepy, we go to sleep
    if (!mDefinition->isWorker() && mHomeTile != nullptr && Random::Double(0.0, 1.0) < 0.2 && Random::Double(0.0, 50.0) >= mAwakeness)
    {
        // Check to see if we can work
        if(pushAction(CreatureAction::sleep))
            return true;
    }

    // If we are hungry, we go to eat
    if (!mDefinition->isWorker() && Random::Double(0.0, 1.0) < 0.2 && Random::Double(50.0, 100.0) <= mHunger)
    {
        // Check to see if we can work
        if(pushAction(CreatureAction::eatdecided))
            return true;
    }

    // Otherwise, we try to work
    if (!mDefinition->isWorker() && Random::Double(0.0, 1.0) < 0.4
        && Random::Double(0.0, 50.0) < mAwakeness && Random::Double(50.0, 100.0) > mHunger)
    {
        // Check to see if we can work
        if(pushAction(CreatureAction::jobdecided))
            return true;
    }

    // Any creature.

    // Workers should move around randomly at large jumps.  Non-workers either wander short distances or follow workers.
    Tile* tileDest = nullptr;
    // Define reachable tiles from the tiles within radius
    std::vector<Tile*> reachableTiles;
    for (Tile* tile: mTilesWithinSightRadius)
    {
        if (getGameMap()->pathExists(this, getPositionTile(), tile))
            reachableTiles.push_back(tile);
    }

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
                    Tile* tempTile = mReachableAlliedObjects[i]->getCoveredTile(0);
                    if (static_cast<Creature*>(mReachableAlliedObjects[i])->peekAction().getType()
                            == CreatureAction::digTile)
                    {
                        // Worker is digging, get near it since it could expose enemies.
                        int x = static_cast<int>(static_cast<double>(tempTile->getX()) + 3.0
                                * Random::gaussianRandomDouble());
                        int y = static_cast<int>(static_cast<double>(tempTile->getY()) + 3.0
                                * Random::gaussianRandomDouble());
                        tileDest = getGameMap()->getTile(x, y);
                    }
                    else
                    {
                        // Worker is not digging, wander a bit farther around the worker.
                        int x = static_cast<int>(static_cast<double>(tempTile->getX()) + 8.0
                                * Random::gaussianRandomDouble());
                        int y = static_cast<int>(static_cast<double>(tempTile->getY()) + 8.0
                                * Random::gaussianRandomDouble());
                        tileDest = getGameMap()->getTile(x, y);
                    }
                    workerFound = true;
                }

                // If there are no workers around, choose tiles far away to "roam" the dungeon.
                if (!workerFound)
                {
                    if (!reachableTiles.empty())
                    {
                        tileDest = reachableTiles[static_cast<unsigned int>(Random::Double(0.6, 0.8)
                                                                           * (reachableTiles.size() - 1))];
                    }
                }
            }
        }
        else
        {
            // Randomly choose a tile near where we are standing to walk to.
            if (!reachableTiles.empty())
            {
                unsigned int tileIndex = static_cast<unsigned int>(reachableTiles.size()
                                                                   * Random::Double(0.1, 0.3));
                tileDest = reachableTiles[tileIndex];
            }
        }
    }
    else
    {
        // Workers only.

        // Choose a tile far away from our current position to wander to.
        if (!reachableTiles.empty())
        {
            tileDest = reachableTiles[Random::Uint(reachableTiles.size() / 2,
                                                   reachableTiles.size() - 1)];
        }
    }

    if(setDestination(tileDest))
        return false;

    return true;
}

bool Creature::handleWalkToTileAction(const CreatureAction& actionItem)
{
    if (mWalkQueue.empty())
    {
        popAction();
        return true;
    }

    // If we are moving during a fight, we do not wait to reach destination to force to compute again what to do
    // Because enemies may have moved, closest creatures could be near...
    if(isActionInList(CreatureAction::fight) && actionItem.getNbTurns() > 1)
    {
        clearDestinations();
        popAction();
        return true;
    }

    return false;
}

bool Creature::handleClaimTileAction(const CreatureAction& actionItem)
{
    Tile* myTile = getPositionTile();
    if (myTile == nullptr)
    {
        popAction();
        return false;
    }

    if(mForceAction != forcedActionClaimTile)
    {
        // Randomly decide to stop claiming with a small probability
        if (Random::Double(0.0, 1.0) < 0.1 + 0.2 * mVisibleMarkedTiles.size())
        {
            // If there are any visible tiles marked for digging start working on that.
            if (!mVisibleMarkedTiles.empty())
            {
                popAction();
                if(pushAction(CreatureAction::digTile))
                    return true;
            }
        }
    }

    // See if the tile we are standing on can be claimed
    if ((!myTile->isClaimedForSeat(getSeat()) || myTile->getClaimedPercentage() < 1.0) && myTile->isGroundClaimable())
    {
        //cout << "\nTrying to claim the tile I am standing on.";
        // Check to see if one of the tile's neighbors is claimed for our color
        for (Tile* tempTile : myTile->getAllNeighbors())
        {
            // Check to see if the current neighbor is a claimed ground tile
            if (tempTile->isClaimedForSeat(getSeat()) && (tempTile->getFullness() == 0.0) && tempTile->getClaimedPercentage() >= 1.0)
            {
                //cout << "\t\tFound a neighbor that is claimed.";
                // If we found a neighbor that is claimed for our side than we can start
                // dancing on this tile.  If there is "left over" claiming that can be done
                // it will spill over into neighboring tiles until it is gone.
                setAnimationState("Claim");
                myTile->claimForSeat(getSeat(), mClaimRate);
                receiveExp(1.5 * (mClaimRate / (0.35 + 0.05 * getLevel())));

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
        if (tempTile != nullptr && tempTile->getFullness() == 0.0
            && (!tempTile->isClaimedForSeat(getSeat()) || tempTile->getClaimedPercentage() < 1.0)
            && tempTile->isGroundClaimable())
        {
            // The neighbor tile is a potential candidate for claiming, to be an actual candidate
            // though it must have a neighbor of its own that is already claimed for our side.
            for (Tile* tempTile2 : tempTile->getAllNeighbors())
            {
                if (tempTile2->isClaimedForSeat(getSeat())
                        && tempTile2->getClaimedPercentage() >= 1.0)
                {
                    clearDestinations();
                    addDestination(static_cast<Ogre::Real>(tempTile->getX()), static_cast<Ogre::Real>(tempTile->getY()));
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
    for (Tile* tempTile : mTilesWithinSightRadius)
    {
        // if this tile is not fully claimed yet or the tile is of another player's color
        if (tempTile != nullptr && tempTile->getFullness() == 0.0
            && (tempTile->getClaimedPercentage() < 1.0 || !tempTile->isClaimedForSeat(getSeat()))
            && tempTile->isGroundClaimable())
        {
            // Check to see if one of the tile's neighbors is claimed for our color
            for (Tile* t : tempTile->getAllNeighbors())
            {
                if (t->isClaimedForSeat(getSeat())
                        && t->getClaimedPercentage() >= 1.0)
                {
                    claimableTiles.push_back(t);
                }
            }
        }
    }

    //cout << "  I see " << claimableTiles.size() << " tiles I can claim.";
    // Randomly pick a claimable tile, plot a path to it and walk to it
    unsigned int tempUnsigned = 0;
    Tile* tempTile = nullptr;
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
            for (Tile* t : tempTile->getAllNeighbors())
            {
                if (t->isClaimedForSeat(getSeat()) && t->getClaimedPercentage() >= 1.0)
                    ++numNeighborsClaimed;
            }

            // Pick a random number in [0:1], if this number is high enough, than use this tile to claim.  The
            // bar for success approaches 0 as numTiles approaches N so this will be guaranteed to succeed at,
            // or before the time we get to the last unclaimed tile.  The bar for success is also lowered
            // according to how many neighbors are already claimed.
            //NOTE: The bar can be negative, when this happens we are guarenteed to use this candidate tile.
            double bar = 1.0 - (numNeighborsClaimed / 4.0) - (tempUnsigned / static_cast<double>(claimableTiles.size() - 1));
            if (Random::Double(0.0, 1.0) >= bar)
                break;

            // Safety catch to prevent infinite loop in case the bar for success is too high and is never met.
            if (tempUnsigned >= claimableTiles.size() - 1)
                break;

            // Increment the counter indicating how many candidate tiles we have rejected so far.
            ++tempUnsigned;
        } while (true);

        if (tempTile != nullptr)
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

bool Creature::handleClaimWallTileAction(const CreatureAction& actionItem)
{
    Tile* myTile = getPositionTile();
    if (myTile == nullptr)
    {
        popAction();
        return false;
    }

    // Randomly decide to stop claiming with a small probability
    if(mForceAction != forcedActionClaimWallTile)
    {
        if (Random::Double(0.0, 1.0) < 0.1 + 0.2 * mVisibleMarkedTiles.size())
        {
            // If there are any visible tiles marked for digging start working on that.
            if (!mVisibleMarkedTiles.empty())
            {
                popAction();
                if(pushAction(CreatureAction::digTile))
                    return true;
            }
        }
    }

    //std::cout << "Claim wall" << std::endl;

    // See if any of the tiles is one of our neighbors
    bool wasANeighbor = false;
    Player* tempPlayer = getGameMap()->getPlayerBySeat(getSeat());
    for (Tile* tempTile : myTile->getAllNeighbors())
    {
        if(wasANeighbor)
            break;

        if (tempPlayer == nullptr)
            break;

        if (!tempTile->isWallClaimable(getSeat()))
            continue;

        // Dig out the tile by decreasing the tile's fullness.
        Ogre::Vector3 walkDirection(tempTile->getX() - getPosition().x, tempTile->getY() - getPosition().y, 0);
        walkDirection.normalise();
        setAnimationState("Claim", true, walkDirection);
        tempTile->claimForSeat(getSeat(), mClaimRate);
        receiveExp(1.5 * mClaimRate / 20.0);

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
        for (Tile* neighborTile : wallTiles[i]->getAllNeighbors())
        {
            if (getGameMap()->pathExists(this, getPositionTile(), neighborTile))
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
                pushAction(CreatureAction::walkToTile, true);
                return false;
            }
        }
    }

    // If we found no path, let's stop doing this
    mForceAction = forcedActionNone;
    popAction();
    return true;
}

bool Creature::handleDigTileAction(const CreatureAction& actionItem)
{
    Tile* myTile = getPositionTile();
    if (myTile == nullptr)
        return false;

    // See if any of the tiles is one of our neighbors
    bool wasANeighbor = false;
    std::vector<Tile*> creatureNeighbors = myTile->getAllNeighbors();
    Player* tempPlayer = getGameMap()->getPlayerBySeat(getSeat());
    for (Tile* tempTile : creatureNeighbors)
    {
        if(wasANeighbor)
            break;

        if (tempPlayer == nullptr)
            break;

        if (!tempTile->getMarkedForDigging(tempPlayer))
            continue;

        // We found a tile marked by our controlling seat, dig out the tile.

        // If the tile is a gold tile accumulate gold for this creature.
        if (tempTile->getType() == Tile::gold)
        {
            double tempDouble = 5 * std::min(mDigRate, tempTile->getFullness());
            mGoldCarried += static_cast<int>(tempDouble);
            getSeat()->addGoldMined(static_cast<int>(tempDouble));
            receiveExp(5.0 * mDigRate / 20.0);
        }

        // Dig out the tile by decreasing the tile's fullness.
        Ogre::Vector3 walkDirection(tempTile->getX() - getPosition().x, tempTile->getY() - getPosition().y, 0);
        walkDirection.normalise();
        setAnimationState("Dig", true, walkDirection);
        double amountDug = tempTile->digOut(mDigRate, true);
        if(amountDug > 0.0)
        {
            receiveExp(1.5 * mDigRate / 20.0);

            // If the tile has been dug out, move into that tile and try to continue digging.
            if (tempTile->getFullness() == 0.0)
            {
                receiveExp(2.5);
                setAnimationState("Walk");

                // Walk to the newly dug out tile.
                addDestination(static_cast<Ogre::Real>(tempTile->getX()), static_cast<Ogre::Real>(tempTile->getY()));
                pushAction(CreatureAction::walkToTile, true);
            }
            //Set sound position and play dig sound.
            fireCreatureSound(CreatureSound::SoundType::DIGGING);
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
    if (mGoldCarried >= MaxGoldCarriedByWorkers)
    {
        // We create the treasury object and push action to deposit it
        TreasuryObject* obj = new TreasuryObject(getGameMap(), mGoldCarried);
        mGoldCarried = 0;
        Ogre::Vector3 pos(static_cast<Ogre::Real>(myTile->getX()), static_cast<Ogre::Real>(myTile->getY()), 0.0f);
        obj->addToGameMap();
        obj->createMesh();
        obj->setPosition(pos, false);

        std::vector<Room*> treasuries = getGameMap()->getRoomsByTypeAndSeat(Room::RoomType::treasury, getSeat());
        treasuries = getGameMap()->getReachableRooms(treasuries, myTile, this);
        bool isTreasuryAvailable = false;
        for(Room* room : treasuries)
        {
            RoomTreasury* treasury = static_cast<RoomTreasury*>(room);
            if(treasury->emptyStorageSpace() <= 0)
                continue;

            isTreasuryAvailable = true;
            break;
        }
        if(isTreasuryAvailable)
        {
            pushAction(CreatureAction::ActionType::carryEntity, true);
        }
        else if((getSeat() != nullptr) &&
                (getSeat()->getPlayer() != nullptr) &&
                (getSeat()->getPlayer()->getIsHuman()))
        {
            getSeat()->getPlayer()->notifyNoTreasuryAvailable();
        }
    }

    // If we successfully dug a tile then we are done for this turn.
    if (wasANeighbor)
        return false;

    // Find paths to all of the neighbor tiles for all of the marked visible tiles.
    std::vector<std::list<Tile*> > possiblePaths;
    for (unsigned int i = 0; i < mVisibleMarkedTiles.size(); ++i)
    {
        for (Tile* neighborTile : mVisibleMarkedTiles[i]->getAllNeighbors())
        {
            if (getGameMap()->pathExists(this, getPositionTile(), neighborTile))
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
                pushAction(CreatureAction::walkToTile, true);
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
        if(mGoldCarried > 0)
        {
            TreasuryObject* obj = new TreasuryObject(getGameMap(), mGoldCarried);
            mGoldCarried = 0;
            Ogre::Vector3 pos(static_cast<Ogre::Real>(myTile->getX()), static_cast<Ogre::Real>(myTile->getY()), 0.0f);
            obj->addToGameMap();
            obj->createMesh();
            obj->setPosition(pos, false);

            std::vector<Room*> treasuries = getGameMap()->getRoomsByTypeAndSeat(Room::RoomType::treasury, getSeat());
            treasuries = getGameMap()->getReachableRooms(treasuries, myTile, this);
            bool isTreasuryAvailable = false;
            for(Room* room : treasuries)
            {
                RoomTreasury* treasury = static_cast<RoomTreasury*>(room);
                if(treasury->emptyStorageSpace() <= 0)
                    continue;

                isTreasuryAvailable = true;
                break;
            }
            if(isTreasuryAvailable)
            {
                pushAction(CreatureAction::ActionType::carryEntity, true);
            }
            else if((getSeat() != nullptr) &&
                    (getSeat()->getPlayer() != nullptr) &&
                    (getSeat()->getPlayer()->getIsHuman()))
            {
                getSeat()->getPlayer()->notifyNoTreasuryAvailable();
            }
        }

        return true;
    }
    return false;
}

bool Creature::handleFindHomeAction(const CreatureAction& actionItem)
{
    // Check to see if we are standing in an open dormitory tile that we can claim as our home.
    Tile* myTile = getPositionTile();
    if (myTile == nullptr)
    {
        popAction();
        return false;
    }

    if((mHomeTile != nullptr) && (actionItem.getType() != CreatureAction::ActionType::findHomeForced))
    {
        popAction();
        return false;
    }

    if((myTile->getCoveringRoom() != nullptr) &&
       (getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())) &&
       (myTile->getCoveringRoom()->getType() == Room::dormitory))
    {
        Room* roomHomeTile = nullptr;
        if(mHomeTile != nullptr)
        {
            roomHomeTile = mHomeTile->getCoveringRoom();
            // Same dormitory nothing to do
            if(roomHomeTile == myTile->getCoveringRoom())
            {
                popAction();
                return true;
            }
        }

        if (static_cast<RoomDormitory*>(myTile->getCoveringBuilding())->claimTileForSleeping(myTile, this))
        {
            // We could install the bed in the dormitory. If we already had one, we remove it
            if(roomHomeTile != nullptr)
                static_cast<RoomDormitory*>(roomHomeTile)->releaseTileForSleeping(mHomeTile, this);

            mHomeTile = myTile;
            popAction();
            return true;
        }

        // The tile where we are is not claimable. We search if there is another in this dormitory
        Tile* tempTile = static_cast<RoomDormitory*>(myTile->getCoveringBuilding())->getLocationForBed(
            mDefinition->getBedDim1(), mDefinition->getBedDim2());
        if(tempTile != nullptr)
        {
            std::list<Tile*> tempPath = getGameMap()->path(this, tempTile);
            if (setWalkPath(tempPath, 1, false))
            {
                setAnimationState("Walk");
                pushAction(CreatureAction::walkToTile, true);
                return false;
            }
        }
    }

    // If we found a tile to claim as our home in the above block
    // If we have been forced, we do not search in another dormitory
    if ((mHomeTile != nullptr) || (actionItem.getType() == CreatureAction::ActionType::findHomeForced))
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
        if (tempTile == nullptr)
            tempTile = static_cast<RoomDormitory*>(tempRooms[i])->getLocationForBed(
                                                                     mDefinition->getBedDim2(), mDefinition->getBedDim1());

        // Check to see if either of the two possible bed orientations tried above resulted in a successful placement.
        if (tempTile != nullptr)
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
            pushAction(CreatureAction::walkToTile, true);
            return false;
        }
    }

    // If we got here there are no reachable dormitory that are unclaimed so we quit trying to find one.
    popAction();
    return true;
}

bool Creature::handleJobAction(const CreatureAction& actionItem)
{
    // Current creature tile position
    Tile* myTile = getPositionTile();
    if (myTile == nullptr)
    {
        popAction();
        return false;
    }

    // If we are unhappy, we stop working
    switch(mMoodValue)
    {
        case CreatureMoodLevel::Upset:
        {
            // 20% chances of not working
            if(Random::Int(0, 100) < 20)
            {
                popAction();
                stopJob();
                return true;
            }
            break;
        }
        case CreatureMoodLevel::Angry:
        case CreatureMoodLevel::Furious:
        {
            // We don't work
            popAction();
            stopJob();
            return true;
        }
        default:
            // We can work
            break;
    }

    // Randomly decide to stop working, we are more likely to stop when we are tired.
    if (Random::Double(10.0, 30.0) > mAwakeness)
    {
        popAction();

        stopJob();
        return true;
    }

    // If we are already working no need to search for a room
    if(mJobRoom != nullptr)
        return false;

    if(actionItem.getType() == CreatureAction::ActionType::jobforced)
    {
        // We check if we can work in the given room
        Room* room = myTile->getCoveringRoom();
        const std::vector<CreatureRoomAffinity>& roomAffinity = mDefinition->getRoomAffinity();
        for(const CreatureRoomAffinity& affinity : roomAffinity)
        {
            if((room != nullptr) &&
               (room->getType() == affinity.getRoomType()) &&
               (affinity.getLikeness() > 0) &&
               (getSeat()->canOwnedCreatureUseRoomFrom(room->getSeat())))
           {
                // If the efficiency is 0 or the room is a hatchery, we only wander in the room
                if((affinity.getEfficiency() <= 0) ||
                   (room->getType() == Room::RoomType::hatchery))
                {
                    int index = Random::Int(0, room->numCoveredTiles() - 1);
                    Tile* tileDest = room->getCoveredTile(index);
                    std::list<Tile*> tempPath = getGameMap()->path(this, tileDest);
                    if (setWalkPath(tempPath, 0, false))
                    {
                        setAnimationState("Walk");
                        pushAction(CreatureAction::walkToTile, true);
                        return false;
                    }

                    popAction();
                    return true;
                }

                // It is the room responsability to test if the creature is suited for working in it
                if(room->hasOpenCreatureSpot(this) && room->addCreatureUsingRoom(this))
                {
                    mJobRoom = room;
                    return false;
                }
                break;
           }
        }

        // If we couldn't work on the room we were forced to, we stop trying
        popAction();
        return true;
    }

    // We get the room we like the most. If we are on such a room, we start working if we can
    const std::vector<CreatureRoomAffinity>& roomAffinity = mDefinition->getRoomAffinity();
    for(const CreatureRoomAffinity& affinity : roomAffinity)
    {
        // See if we are in the room we like the most. If yes and we can work (if possible), we stay. If no,
        // We check if there is such a room somewhere else where we can go
        if((myTile->getCoveringRoom() != nullptr) &&
           (myTile->getCoveringRoom()->getType() == affinity.getRoomType()) &&
           (getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())))
        {
            Room* room = myTile->getCoveringRoom();

            // If the efficiency is 0 or the room is a hatchery, we only wander in the room
            if((affinity.getEfficiency() <= 0) ||
               (room->getType() == Room::RoomType::hatchery))
            {
                int index = Random::Int(0, room->numCoveredTiles() - 1);
                Tile* tileDest = room->getCoveredTile(index);
                std::list<Tile*> tempPath = getGameMap()->path(this, tileDest);
                if (setWalkPath(tempPath, 0, false))
                {
                    setAnimationState("Walk");
                    pushAction(CreatureAction::walkToTile, true);
                    return false;
                }

                popAction();
                return true;
            }

            // It is the room responsability to test if the creature is suited for working in it
            if(room->hasOpenCreatureSpot(this) && room->addCreatureUsingRoom(this))
            {
                mJobRoom = room;
                return false;
            }
        }

        // We are not in a room of the good type or we couldn't use it. We check if there is a reachable room
        // of the good type
        std::vector<Room*> rooms = getGameMap()->getRoomsByTypeAndSeat(affinity.getRoomType(), getSeat());
        rooms = getGameMap()->getReachableRooms(rooms, myTile, this);
        std::random_shuffle(rooms.begin(), rooms.end());
        for(Room* room : rooms)
        {
            // If efficiency is 0, we just want to wander so no need to check if the room
            // is available
            if((affinity.getEfficiency() <= 0) ||
               room->hasOpenCreatureSpot(this))
            {
                int index = Random::Int(0, room->numCoveredTiles() - 1);
                Tile* tileDest = room->getCoveredTile(index);
                std::list<Tile*> tempPath = getGameMap()->path(this, tileDest);
                if (setWalkPath(tempPath, 0, false))
                {
                    setAnimationState("Walk");
                    pushAction(CreatureAction::walkToTile, true);
                    return false;
                }

                popAction();
                return true;
            }
        }
    }

    // Default action
    popAction();
    stopJob();
    return true;
}

bool Creature::handleEatingAction(const CreatureAction& actionItem)
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

    if (((actionItem.getType() == CreatureAction::ActionType::eatforced) && mHunger < 5.0) ||
        ((actionItem.getType() != CreatureAction::ActionType::eatforced) && mHunger <= Random::Double(0.0, 15.0)))
    {
        popAction();

        stopEating();
        return true;
    }

    // If we are in a hatchery, we go to the closest chicken in it. If we are not
    // in a hatchery, we check if there is a free chicken and eat it if we see it
    Tile* closestChickenTile = nullptr;
    double closestChickenDist = 0.0;
    for(Tile* tile : mTilesWithinSightRadius)
    {
        std::vector<GameEntity*> chickens;
        tile->fillWithChickenEntities(chickens);
        if(chickens.empty())
            continue;

        if((mEatRoom == nullptr) &&
           (tile->getCoveringRoom() != nullptr) &&
           (tile->getCoveringRoom()->getType() == Room::RoomType::hatchery))
        {
            // We are not in a hatchery and the currently processed tile is a hatchery. We
            // cannot eat any chicken there
            continue;
        }

        if((mEatRoom != nullptr) && (tile->getCoveringBuilding() != mEatRoom))
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
            std::vector<GameEntity*> chickens;
            closestChickenTile->fillWithChickenEntities(chickens);
            OD_ASSERT_TRUE(!chickens.empty());
            ChickenEntity* chicken = static_cast<ChickenEntity*>(chickens.at(0));
            chicken->eatChicken(this);
            foodEaten(ConfigManager::getSingleton().getRoomConfigDouble("HatcheryHungerPerChicken"));
            mEatCooldown = Random::Int(ConfigManager::getSingleton().getRoomConfigUInt32("HatcheryCooldownChickenMin"),
                ConfigManager::getSingleton().getRoomConfigUInt32("HatcheryCooldownChickenMax"));
            mHp += ConfigManager::getSingleton().getRoomConfigDouble("HatcheryHpRecoveredPerChicken");
            Ogre::Vector3 walkDirection = Ogre::Vector3(closestChickenTile->getX(), closestChickenTile->getY(), 0) - getPosition();
            walkDirection.normalise();
            setAnimationState("Attack1", false, walkDirection);
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
            pushAction(CreatureAction::walkToTile, true);
            return false;
        }
    }

    if(mEatRoom != nullptr)
        return false;

    // See if we are in a hatchery. If so, we try to add the creature. If it is ok, the room
    // will handle the creature from here to make it go where it should
    if((myTile->getCoveringRoom() != nullptr) &&
       (getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())) &&
       (myTile->getCoveringRoom()->getType() == Room::hatchery) &&
       (myTile->getCoveringRoom()->hasOpenCreatureSpot(this)))
    {
        Room* tempRoom = myTile->getCoveringRoom();
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
    Room* tempRoom = nullptr;
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
        pushAction(CreatureAction::walkToTile, true);
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
    if (mJobRoom == nullptr)
        return;

    mJobRoom->removeCreatureUsingRoom(this);
    mJobRoom = nullptr;
}

void Creature::stopEating()
{
    if (mEatRoom == nullptr)
        return;

    mEatRoom->removeCreatureUsingRoom(this);
    mEatRoom = nullptr;
}

bool Creature::isJobRoom(Room* room)
{
    return mJobRoom == room;
}

bool Creature::isEatRoom(Room* room)
{
    return mEatRoom == room;
}

bool Creature::handleAttackAction(const CreatureAction& actionItem)
{
    // We always pop action to make sure next time we will try to find if a closest foe is there
    // or if we need to hit and run
    popAction();

    if (actionItem.getTile() == nullptr)
        return true;

    // The warmup time isn't yet finished.
    if (mAttackWarmupTime > 0.0)
        return true;

    // Reset the warmup time
    mAttackWarmupTime = mDefinition->getAttackWarmupTime();

    Tile* attackedTile = actionItem.getTile();
    GameEntity* attackedObject = nullptr;
    switch(actionItem.getEntityType())
    {
        case GameEntity::ObjectType::creature:
            attackedObject = getGameMap()->getCreature(actionItem.getEntityName());
            break;
        case GameEntity::ObjectType::room:
            attackedObject = getGameMap()->getRoomByName(actionItem.getEntityName());
            break;
        case GameEntity::ObjectType::trap:
            attackedObject = getGameMap()->getTrapByName(actionItem.getEntityName());
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "entityType=" + Ogre::StringConverter::toString(static_cast<int>(actionItem.getEntityType()))
                + ", name=" + getName());
            break;
    }
    // attackedObject can be nullptr if the entity died between the time we started to chase it and the time we strike
    if(attackedObject == nullptr)
        return true;

    // We check what we are attacking.

    // Turn to face the creature we are attacking and set the animation state to Attack.
    Ogre::Vector3 walkDirection(attackedTile->getX() - getPosition().x, attackedTile->getY() - getPosition().y, 0);
    walkDirection.normalise();
    setAnimationState("Attack1", false, walkDirection);

    fireCreatureSound(CreatureSound::SoundType::ATTACK);

    // Calculate how much damage we do.
    Tile* myTile = getPositionTile();
    Ogre::Real range = getGameMap()->crowDistance(myTile, attackedTile);
    // Do the damage and award experience points to both creatures.
    double damageDone = attackedObject->takeDamage(this, getPhysicalDamage(range), getMagicalDamage(range), attackedTile);
    double expGained;
    expGained = 1.0 + 0.2 * std::pow(damageDone, 1.3);

    decreaseAwakeness(0.5);

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

bool Creature::handleFightAction(const CreatureAction& actionItem)
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
        GameEntity* entityAttack = nullptr;
        Tile* tileAttack = nullptr;
        if (fightClosestObjectInList(mReachableEnemyCreatures, entityAttack, tileAttack))
        {
            if(entityAttack != nullptr)
            {
                pushAction(CreatureAction(CreatureAction::attackObject, entityAttack->getObjectType(), entityAttack->getName(), tileAttack), true);
            }
            return (entityAttack != nullptr);
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
    GameEntity* entityAttack = nullptr;
    Tile* tileAttack = nullptr;
    if (!mReachableEnemyCreatures.empty() && fightClosestObjectInList(mReachableEnemyCreatures, entityAttack, tileAttack))
    {
        if(entityAttack != nullptr)
        {
            pushAction(CreatureAction(CreatureAction::attackObject, entityAttack->getObjectType(), entityAttack->getName(), tileAttack), true);
        }
        return (entityAttack != nullptr);
    }

    // If no creature, we attack the rest
    if (fightInRangeObjectInList(mReachableEnemyObjects, entityAttack, tileAttack))
    {
        if(entityAttack != nullptr)
        {
            pushAction(CreatureAction(CreatureAction::attackObject, entityAttack->getObjectType(), entityAttack->getName(), tileAttack), true);
        }
        return (entityAttack != nullptr);
    }

    // We should not come here.
    OD_ASSERT_TRUE(false);
    return false;
}

bool Creature::handleSleepAction(const CreatureAction& actionItem)
{
    Tile* myTile = getPositionTile();
    if (mHomeTile == nullptr)
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

bool Creature::handleFleeAction(const CreatureAction& actionItem)
{
    // We try to go as far as possible from the enemies within visible tiles. We will quit flee mode when there will be no more
    // enemy objects nearby or if we have already flee for too much time
    if ((mReachableEnemyObjects.empty()) || (actionItem.getNbTurns() > NB_TURN_FLEE_MAX))
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
        Tile* tile = room->getCoveredTile(0);
        std::list<Tile*> result = getGameMap()->path(this, tile);
        // If we are not too near from the dungeon temple, we go there
        if(result.size() > 5)
        {
            result.resize(5);
            if (setWalkPath(result, 2, false))
            {
                setAnimationState("Flee");
                pushAction(CreatureAction::walkToTile, true);
                return true;
            }
        }
    }

    // No dungeon temple is acessible or we are too near. We will wander randomly
    wanderRandomly("Flee");
    return false;
}

bool Creature::handleCarryableEntities(const CreatureAction& actionItem)
{
    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
    {
        popAction();
        return true;
    }

    // If we are not carrying anything, we check if there is something carryable around
    if(mCarriedEntity == nullptr)
    {
        std::vector<Building*> buildings = getGameMap()->getReachableBuildingsPerSeat(getSeat(), myTile, this);
        std::vector<MovableGameEntity*> carryableEntities = getGameMap()->getVisibleCarryableEntities(mVisibleTiles);
        std::vector<MovableGameEntity*> availableEntities;
        Building* buildingWants = nullptr;
        Tile* tileDest = nullptr;
        for(MovableGameEntity* entity : carryableEntities)
        {
            // We check if a buildings wants this entity
            buildingWants = nullptr;
            for(Building* building : buildings)
            {
                if(building->hasCarryEntitySpot(entity))
                {
                    buildingWants = building;
                    break;
                }
            }
            if(buildingWants == nullptr)
                continue;

            Tile* carryableEntTile = entity->getPositionTile();

            // We are on the same tile. If we can book a spot, we start carrying
            if(carryableEntTile == myTile)
            {
                tileDest = buildingWants->askSpotForCarriedEntity(entity);
                if(tileDest == nullptr)
                {
                    // The building doesn't want the entity after all
                    buildingWants = nullptr;
                    continue;
                }
                carryEntity(entity);
                break;
            }

            if(!getGameMap()->pathExists(this, myTile, carryableEntTile))
                continue;

            availableEntities.push_back(entity);
        }

        if(mCarriedEntity == nullptr)
        {
            // If there are no carryable entity, we do something else
            if(availableEntities.empty())
            {
                popAction();
                return true;
            }

            uint32_t index = Random::Uint(0,availableEntities.size()-1);
            GameEntity* entity = availableEntities[index];
            Tile* t = entity->getPositionTile();
            OD_ASSERT_TRUE_MSG(t != nullptr, "entity=" + entity->getName());
            if(!setDestination(t))
            {
                popAction();
                return true;
            }

            return false;
        }

        // We have carried something. Now, we go to the building
        mCarriedEntityDestType = buildingWants->getObjectType();
        mCarriedEntityDestName = buildingWants->getName();

        if(setDestination(tileDest))
            return false;

        // Problem while setting destination
        releaseCarriedEntity();
        popAction();
        return true;
    }

    // If we are in this state while carrying something, we should be at the destination
    releaseCarriedEntity();
    popAction();
    return true;
}

bool Creature::handleGetFee(const CreatureAction& actionItem)
{
    Tile* myTile = getPositionTile();
    if(myTile == nullptr)
    {
        popAction();
        return true;
    }
    // We check if we are on a treasury. If yes, we try to take our fee
    if((myTile->getCoveringRoom() != nullptr) &&
       (myTile->getCoveringRoom()->getType() == Room::RoomType::treasury) &&
       (getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())))
    {
        RoomTreasury* treasury = static_cast<RoomTreasury*>(myTile->getCoveringRoom());
        int gold = treasury->getTotalGold();
        if(gold > 0)
        {
            int goldTaken = treasury->withdrawGold(std::min(gold, mGoldFee));
            mGoldFee -= goldTaken;
            if((getSeat()->getPlayer() != nullptr) &&
               (getSeat()->getPlayer()->getIsHuman()))
            {
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotification::chatServer, getSeat()->getPlayer());
                std::string msg;
                // We don't display the same message if we have taken all our fee or only a part of it
                if(mGoldFee <= 0)
                    msg = getName() + " took its fee: " + Ogre::StringConverter::toString(goldTaken);
                else
                    msg = getName() + " took " + Ogre::StringConverter::toString(goldTaken) + " from its fee";

                serverNotification->mPacket << msg;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }

            // We cannot carry more than our fee
            mGoldCarried = std::min(mGoldCarried + goldTaken, mDefinition->getFee(getLevel()));
            if(mGoldFee <= 0)
            {
                // We were able to take all the gold. We can do something else
                mGoldFee = 0;
                popAction();
                return true;
            }
        }
    }

    // We try to go to some treasury were there is still some gold
    std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::RoomType::treasury, getSeat());
    tempRooms = getGameMap()->getReachableRooms(tempRooms, getPositionTile(), this);
    while(!tempRooms.empty())
    {
        // We can go to one treasury
        int index = Random::Int(0, tempRooms.size() - 1);
        Room* room = tempRooms[index];
        tempRooms.erase(tempRooms.begin() + index);
        RoomTreasury* treasury = static_cast<RoomTreasury*>(room);
        if(treasury->getTotalGold() > 0)
        {
            Tile* tile = room->getCoveredTile(0);
            std::list<Tile*> result = getGameMap()->path(this, tile);
            if (setWalkPath(result, 0, false))
            {
                setAnimationState("Walk");
                pushAction(CreatureAction::walkToTile, true);
                return true;
            }
        }
    }

    // No available treasury
    popAction();
    return true;
}

bool Creature::handleLeaveDungeon(const CreatureAction& actionItem)
{
    Tile* myTile = getPositionTile();
    if(myTile == nullptr)
    {
        popAction();
        return true;
    }

    // Check if we are on the central tile of a portal
    if((myTile->getCoveringRoom() != nullptr) &&
       (myTile->getCoveringRoom()->getType() == Room::RoomType::portal) &&
       (getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())))
    {
        if(myTile == myTile->getCoveringRoom()->getCentralTile())
        {
            // We are on the central tile. We can leave the dungeon
            // If the creature has a homeTile where it sleeps, its bed needs to be destroyed.
            stopJob();
            stopEating();
            clearDestinations();
            setIsOnMap(false);
            if (getHomeTile() != nullptr)
            {
                RoomDormitory* home = static_cast<RoomDormitory*>(getHomeTile()->getCoveringBuilding());
                home->releaseTileForSleeping(getHomeTile(), this);
            }

            // Remove the creature from the game map and into the deletion queue, it will be deleted
            // when it is safe, i.e. all other pointers to it have been wiped from the program.
            removeFromGameMap();
            deleteYourself();
        }
    }

    // We try to go to the portal
    std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(Room::RoomType::portal, getSeat());
    tempRooms = getGameMap()->getReachableRooms(tempRooms, myTile, this);
    while(!tempRooms.empty())
    {
        int index = Random::Int(0, tempRooms.size() - 1);
        Room* room = tempRooms[index];
        tempRooms.erase(tempRooms.begin() + index);
        Tile* tile = room->getCentralTile();
        std::list<Tile*> result = getGameMap()->path(this, tile);
        if (setWalkPath(result, 0, false))
        {
            setAnimationState("Walk");
            pushAction(CreatureAction::walkToTile, true);
            return true;
        }
    }

    popAction();
    return true;
}

bool Creature::handleFightAlliedNaturalEnemyAction(const CreatureAction& actionItem)
{
    // We look for a reachable allied natural enemy
    GameEntity* entityAttack = nullptr;
    Tile* tileAttack = nullptr;
    std::vector<GameEntity*> alliedNaturalEnemies = getGameMap()->getNaturalEnemiesInList(this, mReachableAlliedObjects);
    if (!alliedNaturalEnemies.empty() && fightClosestObjectInList(alliedNaturalEnemies, entityAttack, tileAttack))
    {
        if(entityAttack != nullptr)
        {
            OD_ASSERT_TRUE_MSG(entityAttack->getObjectType() == GameEntity::ObjectType::creature, "attacker=" + getName() + ", attacked=" + entityAttack->getName());
            Creature* attackedCreature = static_cast<Creature*>(entityAttack);
            attackedCreature->engageAlliedNaturalEnemy(this);
            pushAction(CreatureAction(CreatureAction::attackObject, entityAttack->getObjectType(), entityAttack->getName(), tileAttack), true);
        }
        return (entityAttack != nullptr);
    }

    // No reachable allied natural enemy. We pop action
    popAction();
    return true;
}

void Creature::engageAlliedNaturalEnemy(Creature* attackerCreature)
{
    clearActionQueue();
    clearDestinations();
    stopJob();
    stopEating();
    std::vector<GameEntity*> attacker;
    GameEntity* entityAttack = nullptr;
    Tile* tileAttack = nullptr;
    attacker.push_back(attackerCreature);
    if (fightClosestObjectInList(attacker, entityAttack, tileAttack))
    {
        pushAction(CreatureAction(CreatureAction::attackObject, entityAttack->getObjectType(), entityAttack->getName(), tileAttack), true);
    }
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

    if (mWeaponL != nullptr && mWeaponL->getRange() >= range)
        hitroll += mWeaponL->getPhysicalDamage();
    if (mWeaponR != nullptr && mWeaponR->getRange() >= range)
        hitroll += mWeaponR->getPhysicalDamage();

    return hitroll;
}

double Creature::getMagicalDamage(double range)
{
    double hitroll = 0.0;

    if (mWeaponlessAtkRange >= range)
        hitroll += Random::Uint(0.0, mMagicalAttack);

    if (mWeaponL != nullptr && mWeaponL->getRange() >= range)
        hitroll += mWeaponL->getMagicalDamage();
    if (mWeaponR != nullptr && mWeaponR->getRange() >= range)
        hitroll += mWeaponR->getMagicalDamage();

    return hitroll;
}

double Creature::getPhysicalDefense() const
{
    double defense = mPhysicalDefense;
    if (mWeaponL != nullptr)
        defense += mWeaponL->getPhysicalDefense();
    if (mWeaponR != nullptr)
        defense += mWeaponR->getPhysicalDefense();

    return defense;
}

double Creature::getMagicalDefense() const
{
    double defense = mMagicalDefense;
    if (mWeaponL != nullptr)
        defense += mWeaponL->getMagicalDefense();
    if (mWeaponR != nullptr)
        defense += mWeaponR->getMagicalDefense();

    return defense;
}

double Creature::getBestAttackRange() const
{
    double range = mWeaponlessAtkRange;

    // Note: The damage check is here to avoid taking defense equipment in account.
    if (mWeaponL != nullptr && mWeaponL->getRange() > range && (mWeaponL->getPhysicalDamage() > 0.0 && mWeaponL->getMagicalDamage() > 0.0))
        range = mWeaponL->getRange();
    if (mWeaponR != nullptr && mWeaponR->getRange() > range && (mWeaponR->getPhysicalDamage() > 0.0 && mWeaponR->getMagicalDamage() > 0.0))
        range = mWeaponR->getRange();

    return range;
}

bool Creature::checkLevelUp()
{
    if (getLevel() >= MAX_LEVEL)
        return false;

    // Check the returned value.
    double newXP = mDefinition->getXPNeededWhenLevel(getLevel());

    // An error occured
    OD_ASSERT_TRUE(newXP > 0.0);
    if (newXP <= 0.0)
        return false;

    if (mExp < newXP)
        return false;

    return true;
}

void Creature::refreshCreature()
{
    RenderManager::getSingleton().rrScaleEntity(this);
}

void Creature::updateTilesInSight()
{
    Tile* posTile = getPositionTile();
    if (posTile == nullptr)
        return;

    // The tiles with sight radius without constraints
    mTilesWithinSightRadius = getGameMap()->circularRegion(posTile->getX(), posTile->getY(), mDefinition->getSightRadius());

    // Only the tiles the creature can "see".
    mVisibleTiles = getGameMap()->visibleTiles(posTile->getX(), posTile->getY(), mDefinition->getSightRadius());
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
        if(entity->getHP(nullptr) <= 0)
            continue;

        Tile* objectTile = entity->getCoveredTile(0);
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

void Creature::updateVisibleMarkedTiles()
{
    mVisibleMarkedTiles.clear();
    Player *player = getGameMap()->getPlayerBySeat(getSeat());
    if (player == nullptr)
        return;

    // Loop over all the visible tiles.
    for (Tile* tile : mTilesWithinSightRadius)
    {
        // Check to see whether the tile is marked for digging
        if (tile == nullptr || !tile->getMarkedForDigging(player))
            continue;

        // and can be reached by the creature
        for (Tile* neighborTile : tile->getAllNeighbors())
        {
            if (neighborTile == nullptr)
                continue;

            if (getGameMap()->pathExists(this, getPositionTile(), neighborTile))
            {
                mVisibleMarkedTiles.push_back(tile);
                break;
            }
        }
    }
}

std::vector<Tile*> Creature::getVisibleClaimableWallTiles()
{
    std::vector<Tile*> claimableWallTiles;

    // Loop over all the visible tiles.
    for (Tile* tile : mTilesWithinSightRadius)
    {
        // Check to see whether the tile is a claimable wall
        if (tile == nullptr || !tile->isWallClaimable(getSeat()))
            continue;

        // and can be reached by the creature
        for (Tile* neighborTile : tile->getAllNeighbors())
        {
            if (neighborTile == nullptr)
                continue;

            if (getGameMap()->pathExists(this, getPositionTile(), neighborTile))
            {
                claimableWallTiles.push_back(tile);
                break;
            }
        }
    }

    return claimableWallTiles;
}

std::vector<GameEntity*> Creature::getVisibleForce(Seat* seat, bool invert)
{
    return getGameMap()->getVisibleForce(mVisibleTiles, seat, invert);
}

void Creature::computeVisualDebugEntities()
{
    if(!getGameMap()->isServerGameMap())
        return;

    mHasVisualDebuggingEntities = true;

    updateTilesInSight();

    ServerNotification *serverNotification = new ServerNotification(
        ServerNotification::refreshCreatureVisDebug, nullptr);

    const std::string& name = getName();
    serverNotification->mPacket << name;
    serverNotification->mPacket << true;
    if(getIsOnMap())
    {
        uint32_t nbTiles = mVisibleTiles.size();
        serverNotification->mPacket << nbTiles;

        for (Tile* tile : mVisibleTiles)
            getGameMap()->tileToPacket(serverNotification->mPacket, tile);
    }
    else
    {
        uint32_t nbTiles = 0;
        serverNotification->mPacket << nbTiles;
    }

    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void Creature::refreshVisualDebugEntities(const std::vector<Tile*>& tiles)
{
    if(getGameMap()->isServerGameMap())
        return;

    mHasVisualDebuggingEntities = true;

    for (Tile* tile : tiles)
    {
        // We check if the visual debug is already on this tile
        if(std::find(mVisualDebugEntityTiles.begin(), mVisualDebugEntityTiles.end(), tile) != mVisualDebugEntityTiles.end())
            continue;

        RenderManager::getSingleton().rrCreateCreatureVisualDebug(this, tile);

        mVisualDebugEntityTiles.push_back(tile);
    }

    // now, we check if visual debug should be removed from a tile
    for (std::vector<Tile*>::iterator it = mVisualDebugEntityTiles.begin(); it != mVisualDebugEntityTiles.end();)
    {
        Tile* tile = *it;
        if(std::find(tiles.begin(), tiles.end(), tile) != tiles.end())
        {
            ++it;
            continue;
        }

        it = mVisualDebugEntityTiles.erase(it);

        RenderManager::getSingleton().rrDestroyCreatureVisualDebug(this, tile);
    }
}

void Creature::stopComputeVisualDebugEntities()
{
    if(!getGameMap()->isServerGameMap())
        return;

    mHasVisualDebuggingEntities = false;

    ServerNotification *serverNotification = new ServerNotification(
        ServerNotification::refreshCreatureVisDebug, nullptr);
    const std::string& name = getName();
    serverNotification->mPacket << name;
    serverNotification->mPacket << false;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void Creature::destroyVisualDebugEntities()
{
    if(getGameMap()->isServerGameMap())
        return;

    mHasVisualDebuggingEntities = false;

    for (Tile* tile : mVisualDebugEntityTiles)
    {
        if (tile == nullptr)
            continue;

        RenderManager::getSingleton().rrDestroyCreatureVisualDebug(this, tile);
    }
    mVisualDebugEntityTiles.clear();
}

std::vector<Tile*> Creature::getCoveredTiles()
{
    std::vector<Tile*> tempVector;
    tempVector.push_back(getPositionTile());
    return tempVector;
}

Tile* Creature::getCoveredTile(int index)
{
    OD_ASSERT_TRUE_MSG(index == 0, "name=" + getName()
        + ", index=" + Ogre::StringConverter::toString(index));

    if(index > 0)
        return nullptr;

    return getPositionTile();
}

uint32_t Creature::numCoveredTiles()
{
    if(getPositionTile() == nullptr)
        return 0;

    return 1;
}

bool Creature::CloseStatsWindow(const CEGUI::EventArgs& /*e*/)
{
    destroyStatsWindow();
    return true;
}

void Creature::createStatsWindow()
{
    if (mStatsWindow != nullptr)
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
    mStatsWindow->setSize(CEGUI::USize(CEGUI::UDim(0, 380), CEGUI::UDim(0, 400)));

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
    if (mStatsWindow != nullptr)
    {
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotification::askCreatureInfos);
        std::string name = getName();
        clientNotification->mPacket << name << false;
        ODClient::getSingleton().queueClientNotification(clientNotification);

        mStatsWindow->destroy();
        mStatsWindow = nullptr;
    }
}

void Creature::updateStatsWindow(const std::string& txt)
{
    if (mStatsWindow == nullptr)
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
    tempSS << "Atk(Phy/Mag): " << mPhysicalAttack << "/" << mMagicalAttack << ", Range: " << mWeaponlessAtkRange << std::endl;
    tempSS << "Weapons:" << std::endl;
    if(mWeaponL == nullptr)
        tempSS << "Left hand: none" << std::endl;
    else
        tempSS << "Left hand: " << mWeaponL->getName() << "-Atk(Phy/Mag): " << mWeaponL->getPhysicalDamage() << "/" << mWeaponL->getMagicalDamage() << ", Range: " << mWeaponL->getRange() << std::endl;
    if(mWeaponR == nullptr)
        tempSS << "Right hand: none" << std::endl;
    else
        tempSS << "Right hand: " << mWeaponR->getName() << "-Atk(Phy/Mag): " << mWeaponR->getPhysicalDamage() << "/" << mWeaponR->getMagicalDamage() << ", Range: " << mWeaponR->getRange() << std::endl;
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
    tempSS << "Mood: " << CreatureMood::toString(mMoodValue) << std::endl;
    tempSS << "MoodPoints: " << Helper::toString(mMoodPoints) << std::endl;
    return tempSS.str();
}

double Creature::takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage)
{
    physicalDamage = std::max(physicalDamage - getPhysicalDefense(), 0.0);
    magicalDamage = std::max(magicalDamage - getMagicalDefense(), 0.0);
    double damageDone = std::min(mHp, physicalDamage + magicalDamage);
    mHp -= damageDone;
    if(!getGameMap()->isServerGameMap())
        return damageDone;

    Player* player = getGameMap()->getPlayerBySeat(getSeat());
    if (player == nullptr)
        return damageDone;

    // Tells the server game map the player is under attack.
    getGameMap()->playerIsFighting(player);

    // If we are a worker attacked by a worker, we fight. Otherwise, we flee (if it is a fighter, a trap,
    // or whatever)
    if(!getDefinition()->isWorker())
        return damageDone;

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
        pushAction(CreatureAction::flee, true);
        return damageDone;
    }
    return damageDone;
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
    mActionQueue.clear();
    stopJob();
    stopEating();
    if(mCarriedEntity != nullptr)
        releaseCarriedEntity();

    mActionQueue.push_front(CreatureAction::idle);
}

bool Creature::pushAction(CreatureAction action, bool forcePush)
{
    if(std::find(mActionTry.begin(), mActionTry.end(), action.getType()) == mActionTry.end())
    {
        mActionTry.push_back(action.getType());
    }
    else
    {
        if(!forcePush)
            return false;
    }

    mActionQueue.push_front(action);
    return true;
}

void Creature::popAction()
{
    mActionQueue.pop_front();
}

CreatureAction Creature::peekAction()
{
    return mActionQueue.front();
}

bool Creature::tryPickup(Seat* seat, bool isEditorMode)
{
    if(!getIsOnMap())
        return false;

    if ((getHP() <= 0.0) && !isEditorMode)
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
    if(tile != nullptr)
        tile->removeEntity(this);

    if(getHasVisualDebuggingEntities())
        computeVisualDebugEntities();
}

bool Creature::canGoThroughTile(const Tile* tile) const
{
    if(tile == nullptr)
        return false;

    switch(tile->getType())
    {
        case Tile::dirt:
        case Tile::gold:
        case Tile::claimed:
        {
            // Note: We don't care about water or lava fullness.
            if (tile->getFullness() > 0.0)
                return false;

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

    // we cannot drop a creature on a tile we don't see
    if(!seat->hasVisionOnTile(tile))
        return false;

    // If it is a worker, he can be dropped on dirt
    if (getDefinition()->isWorker() && (tile->getType() == Tile::dirt || tile->getType() == Tile::gold))
        return true;

    // Every creature can be dropped on allied claimed tiles
    if(tile->getType() == Tile::claimed && tile->getSeat() != nullptr && tile->getSeat()->isAlliedSeat(getSeat()))
        return true;

    return false;
}

bool Creature::setDestination(Tile* tile)
{
    if(tile == nullptr)
        return false;

    Tile *posTile = getPositionTile();
    if(posTile == nullptr)
        return false;

    std::list<Tile*> result = getGameMap()->path(this, tile);

    if (setWalkPath(result, 2, false))
    {
        setAnimationState("Walk");
        pushAction(CreatureAction::walkToTile, true);
        return true;
    }
    return false;
}

bool Creature::fightClosestObjectInList(const std::vector<GameEntity*>& listObjects, GameEntity*& attackedEntity, Tile*& attackedTile)
{
    if(listObjects.empty())
        return false;

    // We check if we are at the best range of our foe. That will allow ranged units to hit and run
    Tile* tileCreature = getPositionTile();
    if(tileCreature == nullptr)
        return false;

    // We try to find the closest enemy object
    Tile* tile = nullptr;
    GameEntity* gameEntity = getGameMap()->getClosestTileWhereGameEntityFromList(listObjects, tileCreature, tile);
    if(gameEntity == nullptr)
        return false;

    // Now that we found the closest enemy, we move to attack
    std::list<Tile*> tempPath;
    if(!getGameMap()->pathToBestFightingPosition(tempPath, this, tile))
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
        attackedTile = tile;
        attackedEntity = gameEntity;
        return true;
    }

    // We have to move to the attacked tile. If we are 1 tile from our foe (tempPath contains 2 values), before
    // moving, we check if he is moving to the same tile as we are. If yes, we don't move
    // to avoid 2 creatures going to each others tiles for ages
    if((tempPath.size() == 2) && (gameEntity->getObjectType() == ObjectType::creature))
    {
        Creature* attackedCreature = static_cast<Creature*>(gameEntity);
        if(!attackedCreature->mWalkQueue.empty())
        {
            Ogre::Vector3 attackedCreatureDest = attackedCreature->mWalkQueue.front();
            int x = Helper::round(attackedCreatureDest.x);
            int y = Helper::round(attackedCreatureDest.y);
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
        pushAction(CreatureAction::walkToTile, true);
    }

    return true;
}

bool Creature::fightInRangeObjectInList(const std::vector<GameEntity*>& listObjects, GameEntity*& attackedEntity, Tile*& attackedTile)
{
    if(listObjects.empty())
        return false;

    // We check if we are at the best range of our foe. That will allow ranged units to hit and run
    Tile* tileCreature = getPositionTile();
    if(tileCreature == nullptr)
        return false;

    // We try to find the closest enemy object within attack range
    GameEntity* closestEnemyEntity = nullptr;
    Tile* closestEnemyTile = nullptr;
    double closestEnemyDist = 0.0;
    Tile* closestNotInRangeEnemyTile = nullptr;
    double closestNotInRangeEnemyDist = 0.0;

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
            if (tempTile == nullptr)
                continue;

            double rSquared = std::pow(tileCreature->getX() - tempTile->getX(), 2.0)
                            + std::pow(tileCreature->getY() - tempTile->getY(), 2.0);

            if (rSquared <= weaponRangeSquared)
            {
                if((closestEnemyTile == nullptr) ||
                   (rSquared < closestEnemyDist))
                {
                    closestEnemyDist = rSquared;
                    closestEnemyTile = tempTile;
                    closestEnemyEntity = gameEntity;
                }
            }
            else
            {
                if((closestNotInRangeEnemyTile == nullptr) ||
                   (rSquared < closestNotInRangeEnemyDist))
                {
                    closestNotInRangeEnemyDist = rSquared;
                    closestNotInRangeEnemyTile = tempTile;
                }
            }
        }
    }

    if(closestEnemyEntity != nullptr)
    {
        attackedEntity = closestEnemyEntity;
        attackedTile = closestEnemyTile;
        return true;
    }

    // There is no enemy in range. We move to the closest non reachable
    std::list<Tile*> tempPath;
    if(!getGameMap()->pathToBestFightingPosition(tempPath, this, closestNotInRangeEnemyTile))
    {
        // We couldn't find a way to the foe. We wander somewhere else
        popAction();
        wanderRandomly("Walk");
        return true;
    }

    if (setWalkPath(tempPath, 1, false))
    {
        setAnimationState("Walk");
        pushAction(CreatureAction::walkToTile, true);
    }

    return true;
}

bool Creature::wanderRandomly(const std::string& animationState)
{
    // We pick randomly a visible tile far away (at the end of visible tiles)
    if(mTilesWithinSightRadius.empty())
        return false;

    // Add reachable tiles only before searching for one of them
    std::vector<Tile*> reachableTiles;
    for (Tile* tile: mTilesWithinSightRadius)
    {
        if (getGameMap()->pathExists(this, getPositionTile(), tile))
            reachableTiles.push_back(tile);
    }

    if (reachableTiles.empty())
        return false;

    Tile* tileDestination = reachableTiles[Random::Uint(0, reachableTiles.size() - 1)];

    std::list<Tile*> result = getGameMap()->path(this, tileDestination);
    if (setWalkPath(result, 1, false))
    {
        setAnimationState(animationState);
        pushAction(CreatureAction::walkToTile, true);
        return true;
    }

    return false;
}

bool Creature::isAttackable(Tile* tile, Seat* seat) const
{
    if(mHp <= 0.0)
        return false;

    return true;
}

bool Creature::tryEntityCarryOn()
{
    // Dead creatures are carryable
    if(getHP() > 0.0)
        return false;

    return true;
}

void Creature::notifyEntityCarryOn()
{
    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
        return;

    setIsOnMap(false);
    myTile->removeEntity(this);
}

void Creature::notifyEntityCarryOff(const Ogre::Vector3& position)
{
    mPosition = position;
    setIsOnMap(true);

    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
        return;

    myTile->addEntity(this);
}

void Creature::carryEntity(MovableGameEntity* carriedEntity)
{
    if(!getGameMap()->isServerGameMap())
        return;

    OD_ASSERT_TRUE(carriedEntity != nullptr);
    OD_ASSERT_TRUE(mCarriedEntity == nullptr);
    mCarriedEntity = nullptr;
    if(carriedEntity == nullptr)
        return;

    carriedEntity->notifyEntityCarryOn();

    // We remove the carried entity from the clients gamemaps as well as the carrier
    // and we send the carrier creation message (that will embed the carried)
    carriedEntity->fireRemoveEntityToSeatsWithVision();
    // We only notify seats that already had vision. We copy the seats with vision list
    // because fireRemoveEntityToSeatsWithVision will empty it.
    std::vector<Seat*> seatsWithVision = mSeatsWithVisionNotified;
    // We remove ourself and send the creation
    fireRemoveEntityToSeatsWithVision();
    mCarriedEntity = carriedEntity;
    notifySeatsWithVision(seatsWithVision);
}

void Creature::releaseCarriedEntity()
{
    if(!getGameMap()->isServerGameMap())
        return;

    MovableGameEntity* carriedEntity = mCarriedEntity;
    GameEntity::ObjectType carriedEntityDestType = mCarriedEntityDestType;
    std::string carriedEntityDestName = mCarriedEntityDestName;

    mCarriedEntity = nullptr;
    mCarriedEntityDestType = GameEntity::ObjectType::unknown;
    mCarriedEntityDestName.clear();

    OD_ASSERT_TRUE_MSG(carriedEntity != nullptr, "name=" + getName());
    if(carriedEntity == nullptr)
        return;

    const Ogre::Vector3& pos = getPosition();
    carriedEntity->notifyEntityCarryOff(pos);

    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        ServerNotification* serverNotification = new ServerNotification(
            ServerNotification::releaseCarriedEntity, seat->getPlayer());
        serverNotification->mPacket << getName() << carriedEntity->getObjectType();
        serverNotification->mPacket << carriedEntity->getName();
        serverNotification->mPacket << mPosition;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }

    Building* dest = nullptr;
    switch(carriedEntityDestType)
    {
        case GameEntity::ObjectType::room:
        {
            dest = getGameMap()->getRoomByName(carriedEntityDestName);
            break;
        }
        case GameEntity::ObjectType::trap:
        {
            dest = getGameMap()->getTrapByName(carriedEntityDestName);
            break;
        }
        default:
            break;
    }
    if(dest == nullptr)
    {
        // We couldn't find the destination. This can happen if it has been destroyed while
        // we were carrying the entity
        LogManager::getSingleton().logMessage("Couldn't carry entity=" + carriedEntity->getName()
            + " to entity name=" + carriedEntityDestName);
        return;
    }

    dest->notifyCarryingStateChanged(this, carriedEntity);
}

bool Creature::canSlap(Seat* seat, bool isEditorMode)
{
    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return false;

    if(getHP() <= 0.0)
        return false;

    if(isEditorMode)
        return true;

    // Only the owning player can slap a creature
    if(getSeat() != seat)
        return false;

    return true;
}

void Creature::slap(bool isEditorMode)
{
    if(!getGameMap()->isServerGameMap())
        return;

    // In editor mode, we remove the creature
    if(isEditorMode)
    {
        removeFromGameMap();
        deleteYourself();
        return;
    }

    // TODO : on server side, we should boost speed for a time and decrease mood/hp
    mHp -= mMaxHP * ConfigManager::getSingleton().getSlapDamagePercent() / 100.0;

}

void Creature::fireAddEntity(Seat* seat, bool async)
{
    if(async)
    {
        ServerNotification serverNotification(
            ServerNotification::addCreature, seat->getPlayer());
        exportToPacket(serverNotification.mPacket);
        ODServer::getSingleton().sendAsyncMsg(serverNotification);

        OD_ASSERT_TRUE_MSG(mCarriedEntity == nullptr, "Trying to fire add creature in async mode name="
            + getName() + " while carrying " + mCarriedEntity->getName());
    }
    else
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotification::addCreature, seat->getPlayer());
        exportToPacket(serverNotification->mPacket);
        ODServer::getSingleton().queueServerNotification(serverNotification);

        if(mCarriedEntity != nullptr)
        {
            mCarriedEntity->addSeatWithVision(seat, false);

            serverNotification = new ServerNotification(
                ServerNotification::carryEntity, seat->getPlayer());
            serverNotification->mPacket << getName() << mCarriedEntity->getObjectType();
            serverNotification->mPacket << mCarriedEntity->getName();
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }

    }
}

void Creature::fireRemoveEntity(Seat* seat)
{
    // If we are carrying an entity, we release it first, then we can remove it and us
    if(mCarriedEntity != nullptr)
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotification::releaseCarriedEntity, seat->getPlayer());
        serverNotification->mPacket << getName() << mCarriedEntity->getObjectType();
        serverNotification->mPacket << mCarriedEntity->getName();
        serverNotification->mPacket << mPosition;
        ODServer::getSingleton().queueServerNotification(serverNotification);

        mCarriedEntity->removeSeatWithVision(seat);
    }

    const std::string& name = getName();
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotification::removeCreature, seat->getPlayer());
    serverNotification->mPacket << name;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void Creature::fireCreatureRefresh()
{
    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getName();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::creatureRefresh, seat->getPlayer());
        serverNotification->mPacket << name;
        serverNotification->mPacket << mLevel;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Creature::fireCreatureSound(CreatureSound::SoundType sound)
{
    Tile* posTile = getPositionTile();
    if(posTile == nullptr)
        return;

    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getDefinition()->getClassName();
        const Ogre::Vector3& position = getPosition();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::playCreatureSound, seat->getPlayer());
        serverNotification->mPacket << name << sound << position;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Creature::itsPayDay()
{
    // Rogue creatures do not have to be payed
    if(getSeat()->isRogueSeat())
        return;

    mGoldFee += mDefinition->getFee(getLevel());
}

void Creature::increaseHunger(double value)
{
    if(getSeat()->isRogueSeat())
        return;

    mHunger = std::min(100.0, mHunger + value);
}

void Creature::decreaseAwakeness(double value)
{
    if(getSeat()->isRogueSeat())
        return;

    mAwakeness = std::max(0.0, mAwakeness - value);
}

void Creature::computeMood()
{
    mMoodPoints = getGameMap()->computeCreatureMoodModifiers(this);

    CreatureMoodLevel newMoodValue = ConfigManager::getSingleton().getCreatureMoodLevel(mMoodPoints);
    if((newMoodValue > CreatureMoodLevel::Neutral) &&
       (mMoodValue <= CreatureMoodLevel::Neutral))
    {
        // We became unhappy
        if((getSeat()->getPlayer() != nullptr) &&
           (getSeat()->getPlayer()->getIsHuman()))
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::chatServer, getSeat()->getPlayer());
            std::string msg = getName() + " is unhappy";
            serverNotification->mPacket << msg;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
    if(newMoodValue != CreatureMoodLevel::Furious)
    {
        mMoodValue = newMoodValue;
        mFirstTurnFurious = -1;
        return;
    }

    if(mMoodValue != CreatureMoodLevel::Furious)
    {
        mMoodValue = newMoodValue;
        mFirstTurnFurious = getGameMap()->getTurnNumber();

        if((getSeat()->getPlayer() != nullptr) &&
           (getSeat()->getPlayer()->getIsHuman()))
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::chatServer, getSeat()->getPlayer());
            std::string msg = getName() + " wants to leave your dungeon";
            serverNotification->mPacket << msg;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
    else if(getGameMap()->getTurnNumber() > (mFirstTurnFurious + ConfigManager::getSingleton().getNbTurnsFuriousMax()))
    {
        // We couldn't leave the dungeon in time, we become rogue
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::chatServer, getSeat()->getPlayer());
        std::string msg = getName() + " is not under your control anymore !";
        serverNotification->mPacket << msg;
        ODServer::getSingleton().queueServerNotification(serverNotification);

        Seat* rogueSeat = getGameMap()->getSeatRogue();
        setSeat(rogueSeat);
        mMoodValue = CreatureMoodLevel::Neutral;
        mMoodPoints = 0;
        mAwakeness = 100;
        mHunger = 0;
        clearDestinations();
        clearActionQueue();
        stopJob();
        stopEating();
        if (getHomeTile() != nullptr)
        {
            RoomDormitory* home = static_cast<RoomDormitory*>(getHomeTile()->getCoveringBuilding());
            home->releaseTileForSleeping(getHomeTile(), this);
        }
    }
}
