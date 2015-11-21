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

#include "creatureaction/CreatureAction.h"
#include "creatureaction/CreatureActionClaimGroundTile.h"
#include "creatureaction/CreatureActionClaimWallTile.h"
#include "creatureaction/CreatureActionDigTile.h"
#include "creatureaction/CreatureActionSearchFood.h"
#include "creatureaction/CreatureActionFight.h"
#include "creatureaction/CreatureActionFightArena.h"
#include "creatureaction/CreatureActionFindHome.h"
#include "creatureaction/CreatureActionFlee.h"
#include "creatureaction/CreatureActionGetFee.h"
#include "creatureaction/CreatureActionGrabEntity.h"
#include "creatureaction/CreatureActionLeaveDungeon.h"
#include "creatureaction/CreatureActionSearchEntityToCarry.h"
#include "creatureaction/CreatureActionSearchGroundTileToClaim.h"
#include "creatureaction/CreatureActionSearchJob.h"
#include "creatureaction/CreatureActionSearchTileToDig.h"
#include "creatureaction/CreatureActionSearchWallTileToClaim.h"
#include "creatureaction/CreatureActionSleep.h"
#include "creatureaction/CreatureActionUseHatchery.h"
#include "creatureaction/CreatureActionWalkToTile.h"
#include "creaturebehaviour/CreatureBehaviour.h"
#include "creatureeffect/CreatureEffect.h"
#include "creatureeffect/CreatureEffectManager.h"
#include "creatureeffect/CreatureEffectSlap.h"
#include "creaturemood/CreatureMood.h"
#include "creaturemood/CreatureMoodManager.h"
#include "creatureskill/CreatureSkill.h"
#include "entities/ChickenEntity.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "entities/TreasuryObject.h"
#include "entities/Weapon.h"
#include "game/Player.h"
#include "game/Skill.h"
#include "game/SkillType.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "giftboxes/GiftBoxSkill.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/CreatureOverlayStatus.h"
#include "render/RenderManager.h"
#include "rooms/RoomCrypt.h"
#include "rooms/RoomDormitory.h"
#include "sound/SoundEffectsManager.h"
#include "spells/Spell.h"
#include "spells/SpellType.h"
#include "traps/Trap.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
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

static const Ogre::Real CANNON_MISSILE_HEIGHT = 0.3;

const int32_t Creature::NB_TURNS_BEFORE_CHECKING_TASK = 15;
const uint32_t Creature::NB_OVERLAY_HEALTH_VALUES = 8;

enum CreatureMoodEnum
{
    Angry = 0x0001,
    Furious = 0x0002,
    GetFee = 0x0004,
    LeaveDungeon = 0x0008,
    KoDeath = 0x0010,
    Hungry = 0x0020,
    Tired = 0x0040,
    KoTemp = 0x0080,
    InJail = 0x0100,
    // To know if a creature is KO
    KoDeathOrTemp = KoTemp | KoDeath,
    // Mood filters for creatures in prison that every player will see
    MoodPrisonFiltersAllPlayers = InJail,
    // Mood filters for creatures in prison that prison allied will see
    MoodPrisonFiltersPrisonAllies = KoTemp | InJail
};

CreatureParticuleEffect::~CreatureParticuleEffect()
{
    if(mEffect != nullptr)
        delete mEffect;

    mEffect = nullptr;
}

Creature::Creature(GameMap* gameMap, bool isOnServerMap, const CreatureDefinition* definition, Seat* seat, Ogre::Vector3 position) :
    MovableGameEntity        (gameMap, isOnServerMap),
    mPhysicalDefense         (3.0),
    mMagicalDefense          (1.5),
    mElementDefense          (0.0),
    mModifierStrength        (1.0),
    mWeaponL                 (nullptr),
    mWeaponR                 (nullptr),
    mHomeTile                (nullptr),
    mDefinition              (definition),
    mHasVisualDebuggingEntities (false),
    mWakefulness             (100.0),
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
    mSkillTypeDropDeath      (SkillType::nullSkillType),
    mWeaponDropDeath         ("none"),
    mStatsWindow             (nullptr),
    mNbTurnsWithoutBattle    (0),
    mCarriedEntity           (nullptr),
    mMoodCooldownTurns       (0),
    mMoodValue               (CreatureMoodLevel::Neutral),
    mMoodPoints              (0),
    mNbTurnFurious           (-1),
    mOverlayHealthValue      (0),
    mOverlayMoodValue        (0),
    mOverlayStatus           (nullptr),
    mNeedFireRefresh         (false),
    mDropCooldown            (0),
    mSpeedModifier           (1.0),
    mKoTurnCounter           (0),
    mSeatPrison              (nullptr)

{
    //TODO: This should be set in initialiser list in parent classes
    setSeat(seat);
    mPosition = position;
    setMeshName(definition->getMeshName());
    setName(getGameMap()->nextUniqueNameCreature(definition->getClassName()));

    mMaxHP = mDefinition->getMinHp();
    setHP(mMaxHP);

    mGroundSpeed = mDefinition->getMoveSpeedGround();
    mWaterSpeed = mDefinition->getMoveSpeedWater();
    mLavaSpeed = mDefinition->getMoveSpeedLava();

    mDigRate = mDefinition->getDigRate();
    mClaimRate = mDefinition->getClaimRate();

    // Fighting stats
    mPhysicalDefense = mDefinition->getPhysicalDefense();
    mMagicalDefense = mDefinition->getMagicalDefense();
    mElementDefense = mDefinition->getElementDefense();

    if(mDefinition->getWeaponSpawnL().compare("none") != 0)
        mWeaponL = gameMap->getWeapon(mDefinition->getWeaponSpawnL());

    if(mDefinition->getWeaponSpawnR().compare("none") != 0)
        mWeaponR = gameMap->getWeapon(mDefinition->getWeaponSpawnR());

    setupDefinition(*gameMap, *ConfigManager::getSingleton().getCreatureDefinitionDefaultWorker());
}

Creature::Creature(GameMap* gameMap, bool isOnServerMap) :
    MovableGameEntity        (gameMap, isOnServerMap),
    mPhysicalDefense         (3.0),
    mMagicalDefense          (1.5),
    mElementDefense          (0.0),
    mModifierStrength        (1.0),
    mWeaponL                 (nullptr),
    mWeaponR                 (nullptr),
    mHomeTile                (nullptr),
    mDefinition              (nullptr),
    mHasVisualDebuggingEntities (false),
    mWakefulness             (100.0),
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
    mSkillTypeDropDeath      (SkillType::nullSkillType),
    mWeaponDropDeath         ("none"),
    mStatsWindow             (nullptr),
    mNbTurnsWithoutBattle    (0),
    mCarriedEntity           (nullptr),
    mMoodCooldownTurns       (0),
    mMoodValue               (CreatureMoodLevel::Neutral),
    mMoodPoints              (0),
    mNbTurnFurious           (-1),
    mOverlayHealthValue      (0),
    mOverlayMoodValue        (0),
    mOverlayStatus           (nullptr),
    mNeedFireRefresh         (false),
    mDropCooldown            (0),
    mSpeedModifier           (1.0),
    mKoTurnCounter           (0),
    mSeatPrison              (nullptr)
{
}

Creature::~Creature()
{
}

void Creature::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();
    if(!getIsOnServerMap())
    {
        RenderManager::getSingleton().rrCreateCreature(this);

        // By default, we set the creature in idle state
        RenderManager::getSingleton().rrSetObjectAnimationState(this, EntityAnimation::idle_anim, true);
    }

    createMeshWeapons();
}

void Creature::destroyMeshLocal()
{
    destroyMeshWeapons();
    MovableGameEntity::destroyMeshLocal();
    if(getIsOnServerMap())
        return;

    destroyStatsWindow();
    RenderManager::getSingleton().rrDestroyCreature(this);
}

void Creature::createMeshWeapons()
{
    if(getIsOnServerMap())
        return;

    if(mWeaponL != nullptr)
        RenderManager::getSingleton().rrCreateWeapon(this, mWeaponL, "L");

    if(mWeaponR != nullptr)
        RenderManager::getSingleton().rrCreateWeapon(this, mWeaponR, "R");
}

void Creature::destroyMeshWeapons()
{
    if(getIsOnServerMap())
        return;

    if(mWeaponL != nullptr)
        RenderManager::getSingleton().rrDestroyWeapon(this, mWeaponL, "L");

    if(mWeaponR != nullptr)
        RenderManager::getSingleton().rrDestroyWeapon(this, mWeaponR, "R");
}

void Creature::addToGameMap()
{
    getGameMap()->addCreature(this);
    getGameMap()->addAnimatedObject(this);
    getGameMap()->addClientUpkeepEntity(this);

    if(!getIsOnServerMap())
        return;

    getGameMap()->addActiveObject(this);
}

void Creature::removeFromGameMap()
{
    fireEntityRemoveFromGameMap();
    removeEntityFromPositionTile();
    getGameMap()->removeCreature(this);
    getGameMap()->removeAnimatedObject(this);
    getGameMap()->removeClientUpkeepEntity(this);

    if(!getIsOnServerMap())
        return;

    // If the creature has a homeTile where it sleeps, its bed needs to be destroyed.
    if (getHomeTile() != nullptr)
    {
        RoomDormitory* home = static_cast<RoomDormitory*>(getHomeTile()->getCoveringBuilding());
        home->releaseTileForSleeping(getHomeTile(), this);
    }

    fireRemoveEntityToSeatsWithVision();
    getGameMap()->removeActiveObject(this);
}

std::string Creature::getCreatureStreamFormat()
{
    std::string format = MovableGameEntity::getMovableGameEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "ClassName\tLevel\tCurrentXP\tCurrentHP\tCurrentWakefulness"
            "\tCurrentHunger\tGoldToDeposit\tLeftWeapon\tRightWeapon\tCarriedSkill\tCarriedWeapon"
            "\tNbCreatureEffects\tN*CreatureEffects";

    return format;
}

void Creature::exportToStream(std::ostream& os) const
{
    MovableGameEntity::exportToStream(os);
    os << mDefinition->getClassName() << "\t";
    os << getLevel() << "\t" << mExp << "\t";
    if(getHP() < mMaxHP)
        os << getHP();
    else
        os << "max";
    os << "\t" << mWakefulness << "\t" << mHunger << "\t" << mGoldCarried;

    // Check creature weapons
    if(mWeaponL != nullptr)
        os << "\t" << mWeaponL->getName();
    else
        os << "\tnone";

    if(mWeaponR != nullptr)
        os << "\t" << mWeaponR->getName();
    else
        os << "\tnone";

    os << "\t" << Skills::toString(mSkillTypeDropDeath);

    os << "\t" << mWeaponDropDeath;

    uint32_t nbEffects = mEntityParticleEffects.size();
    os << "\t" << nbEffects;
    for(EntityParticleEffect* effect : mEntityParticleEffects)
    {
        CreatureParticuleEffect* creatureParticuleEffect = static_cast<CreatureParticuleEffect*>(effect);
        os << "\t";
        CreatureEffectManager::write(*creatureParticuleEffect->mEffect, os);
    }
}

bool Creature::importFromStream(std::istream& is)
{
    // Beware: A generic class name might be used here so we shouldn't use mDefinition
    // here as it is not set yet (for example, default worker will be available only after
    // seat lobby configuration)
    if(!MovableGameEntity::importFromStream(is))
        return false;
    std::string tempString;

    if(!(is >> mDefinitionString))
        return false;
    if(!(is >> mLevel))
        return false;
    if(!(is >> mExp))
        return false;
    if(!(is >> mHpString))
        return false;
    if(!(is >> mWakefulness))
        return false;
    if(!(is >> mHunger))
        return false;
    if(!(is >> mGoldCarried))
        return false;
    if(!(is >> tempString))
        return false;
    if(tempString != "none")
    {
        mWeaponL = getGameMap()->getWeapon(tempString);
        if(mWeaponL == nullptr)
        {
            OD_LOG_ERR("Unknown weapon name=" + tempString);
        }
    }

    if(!(is >> tempString))
        return false;
    if(tempString != "none")
    {
        mWeaponR = getGameMap()->getWeapon(tempString);
        if(mWeaponR == nullptr)
        {
            OD_LOG_ERR("Unknown weapon name=" + tempString);
        }
    }

    if(!(is >> tempString))
        return false;
    mSkillTypeDropDeath = Skills::fromString(tempString);

    if(!(is >> mWeaponDropDeath))
        return false;

    mLevel = std::min(MAX_LEVEL, mLevel);

    uint32_t nbEffects;
    if(!(is >> nbEffects))
        return false;
    while(nbEffects > 0)
    {
        --nbEffects;
        CreatureEffect* effect = CreatureEffectManager::load(is);
        if(effect == nullptr)
            continue;

        addCreatureEffect(effect);
    }

    return true;
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

    mPhysicalDefense = mDefinition->getPhysicalDefense();
    mMagicalDefense = mDefinition->getMagicalDefense();
    mElementDefense = mDefinition->getElementDefense();

    updateScale();

    // Improve the stats to the current level
    double multiplier = mLevel - 1;
    if (multiplier <= 0.0)
        return;

    mMaxHP += mDefinition->getHpPerLevel() * multiplier;
    mDigRate += mDefinition->getDigRatePerLevel() * multiplier;
    mClaimRate += mDefinition->getClaimRatePerLevel() * multiplier;
    mGroundSpeed += mDefinition->getGroundSpeedPerLevel() * multiplier;
    mWaterSpeed += mDefinition->getWaterSpeedPerLevel() * multiplier;
    mLavaSpeed += mDefinition->getLavaSpeedPerLevel() * multiplier;

    mPhysicalDefense += mDefinition->getPhysicalDefPerLevel() * multiplier;
    mMagicalDefense += mDefinition->getMagicalDefPerLevel() * multiplier;
    mElementDefense += mDefinition->getElementDefPerLevel() * multiplier;
}

void Creature::updateScale()
{
    mScale = getDefinition()->getScale();
    Ogre::Real scaleFactor = static_cast<Ogre::Real>(1.0 + 0.02 * static_cast<double>(getLevel()));
    mScale *= scaleFactor;
}

Creature* Creature::getCreatureFromStream(GameMap* gameMap, std::istream& is)
{
    //TODO - Handle load errors
    Creature* creature = new Creature(gameMap, true);
    creature->importFromStream(is);
    return creature;
}

Creature* Creature::getCreatureFromPacket(GameMap* gameMap, ODPacket& is)
{
    Creature* creature = new Creature(gameMap, false);
    creature->importFromPacket(is);
    return creature;
}

void Creature::exportToPacket(ODPacket& os, const Seat* seat) const
{
    MovableGameEntity::exportToPacket(os, seat);
    const std::string& className = mDefinition->getClassName();
    os << className;
    os << mLevel;
    os << mExp;

    os << mHp;
    os << mMaxHP;

    os << mDigRate;
    os << mClaimRate;
    os << mWakefulness;
    os << mHunger;

    os << mGroundSpeed;
    os << mWaterSpeed;
    os << mLavaSpeed;

    os << mPhysicalDefense;
    os << mMagicalDefense;
    os << mElementDefense;
    os << mOverlayHealthValue;

    // Only allied players should see creature mood (except some states)
    uint32_t moodValue = 0;
    if(seat->isAlliedSeat(getSeat()))
        moodValue = mOverlayMoodValue;
    else if(mSeatPrison != nullptr)
    {
        if(mSeatPrison->isAlliedSeat(seat))
            moodValue = mOverlayMoodValue & CreatureMoodEnum::MoodPrisonFiltersPrisonAllies;
        else
            moodValue = mOverlayMoodValue & CreatureMoodEnum::MoodPrisonFiltersAllPlayers;
    }

    os << moodValue;
    os << mSpeedModifier;

    if(mWeaponL != nullptr)
        os << mWeaponL->getName();
    else
        os << "none";

    if(mWeaponR != nullptr)
        os << mWeaponR->getName();
    else
        os << "none";

    uint32_t nbEffects = mEntityParticleEffects.size();
    os << nbEffects;
    for(EntityParticleEffect* effect : mEntityParticleEffects)
    {
        os << effect->mName;
        os << effect->mScript;
        os << effect->mNbTurnsEffect;
    }
}

void Creature::importFromPacket(ODPacket& is)
{
    MovableGameEntity::importFromPacket(is);
    std::string tempString;

    OD_ASSERT_TRUE(is >> mDefinitionString);

    OD_ASSERT_TRUE(is >> mLevel);
    OD_ASSERT_TRUE(is >> mExp);

    OD_ASSERT_TRUE(is >> mHp);
    OD_ASSERT_TRUE(is >> mMaxHP);

    OD_ASSERT_TRUE(is >> mDigRate);
    OD_ASSERT_TRUE(is >> mClaimRate);
    OD_ASSERT_TRUE(is >> mWakefulness);
    OD_ASSERT_TRUE(is >> mHunger);

    OD_ASSERT_TRUE(is >> mGroundSpeed);
    OD_ASSERT_TRUE(is >> mWaterSpeed);
    OD_ASSERT_TRUE(is >> mLavaSpeed);

    OD_ASSERT_TRUE(is >> mPhysicalDefense);
    OD_ASSERT_TRUE(is >> mMagicalDefense);
    OD_ASSERT_TRUE(is >> mElementDefense);

    OD_ASSERT_TRUE(is >> mOverlayHealthValue);
    OD_ASSERT_TRUE(is >> mOverlayMoodValue);
    OD_ASSERT_TRUE(is >> mSpeedModifier);

    OD_ASSERT_TRUE(is >> tempString);
    if(tempString != "none")
    {
        mWeaponL = getGameMap()->getWeapon(tempString);
        if(mWeaponL == nullptr)
        {
            OD_LOG_ERR("Unknown weapon name=" + tempString);
        }
    }

    OD_ASSERT_TRUE(is >> tempString);
    if(tempString != "none")
    {
        mWeaponR = getGameMap()->getWeapon(tempString);
        if(mWeaponR == nullptr)
        {
            OD_LOG_ERR("Unknown weapon name=" + tempString);
        }
    }

    uint32_t nbEffects;
    OD_ASSERT_TRUE(is >> nbEffects);

    while(nbEffects > 0)
    {
        --nbEffects;

        std::string effectName;
        std::string effectScript;
        uint32_t nbTurns;
        OD_ASSERT_TRUE(is >> effectName >> effectScript >> nbTurns);
        CreatureParticuleEffectClient* effect = new CreatureParticuleEffectClient(effectName, effectScript, nbTurns);
        mEntityParticleEffects.push_back(effect);
    }
    setupDefinition(*getGameMap(), *ConfigManager::getSingleton().getCreatureDefinitionDefaultWorker());
}

void Creature::setPosition(const Ogre::Vector3& v)
{
    MovableGameEntity::setPosition(v);
    if(mCarriedEntity != nullptr)
        mCarriedEntity->notifyCarryMove(v);
}

void Creature::setHP(double nHP)
{
    if (nHP > mMaxHP)
        mHp = mMaxHP;
    else
        mHp = nHP;

    computeCreatureOverlayHealthValue();
}

void Creature::heal(double hp)
{
    mHp = std::min(mHp + hp, mMaxHP);

    computeCreatureOverlayHealthValue();
}

bool Creature::isAlive() const
{
    if(!getIsOnServerMap())
        return mOverlayHealthValue < (NB_OVERLAY_HEALTH_VALUES - 1);

    return mHp > 0.0;
}

void Creature::update(Ogre::Real timeSinceLastFrame)
{
    Tile* previousPositionTile = getPositionTile();
    // Update movements, direction, ...
    MovableGameEntity::update(timeSinceLastFrame);

    // Update the visual debugging entities
    //if we are standing in a different tile than we were last turn
    if (mHasVisualDebuggingEntities &&
        getIsOnServerMap() &&
        (getPositionTile() != previousPositionTile))
    {
        computeVisualDebugEntities();
    }

    if(getOverlayStatus() != nullptr)
    {
        getOverlayStatus()->update(timeSinceLastFrame);
    }
}

void Creature::computeVisibleTiles()
{
    // dead Creatures do not give vision
    if (getHP() <= 0.0)
        return;

    // KO Creatures do not give vision
    if (isKo())
        return;

    // creatures in jail do not give vision
    if (mSeatPrison != nullptr)
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

    buildStats();

    mNeedFireRefresh = true;
}

void Creature::dropCarriedEquipment()
{
    fireCreatureSound(CreatureSound::Die);
    clearActionQueue();
    clearDestinations(EntityAnimation::die_anim, false);

    // We drop what we are carrying
    Tile* myTile = getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + getName() + ", position=" + Helper::toString(getPosition()));
        return;
    }

    if(mGoldCarried > 0)
    {
        TreasuryObject* obj = new TreasuryObject(getGameMap(), true, mGoldCarried);
        obj->addToGameMap();
        Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(myTile->getX()),
                                    static_cast<Ogre::Real>(myTile->getY()), 0.0f);
        obj->createMesh();
        obj->setPosition(spawnPosition);
        mGoldCarried = 0;
    }

    if(mSkillTypeDropDeath != SkillType::nullSkillType)
    {
        GiftBoxSkill* skillEntity = new GiftBoxSkill(getGameMap(), getIsOnServerMap(),
            "DroppedBy" + getName(), mSkillTypeDropDeath);
        skillEntity->addToGameMap();
        Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(myTile->getX()),
                                    static_cast<Ogre::Real>(myTile->getY()), 0.0f);
        skillEntity->createMesh();
        skillEntity->setPosition(spawnPosition);
        mSkillTypeDropDeath = SkillType::nullSkillType;
    }

    // TODO: drop weapon when available
}

void Creature::doUpkeep()
{
    // If the creature is in jail, we check if it is still standing on it (if not picked up). If
    // not, it is free
    if((mSeatPrison != nullptr) &&
       getIsOnMap())
    {
        Tile* myTile = getPositionTile();
        if(myTile == nullptr)
        {
            OD_LOG_ERR("name=" + getName() + ", position=" + Helper::toString(getPosition()));
            return;
        }

        Room* roomPrison = myTile->getCoveringRoom();
        if((roomPrison == nullptr) ||
           (roomPrison->getType() != RoomType::prison) ||
           (roomPrison->getSeat() != mSeatPrison))
        {
            // it is not standing on a jail. It is free
            mSeatPrison = nullptr;
            mNeedFireRefresh = true;
        }
    }

    // The creature may be killed while temporary KO
    if((mKoTurnCounter != 0) && !isAlive())
        mKoTurnCounter = 0;

    // If the creature is KO to death or dead, we remove its particle effects
    if(!mEntityParticleEffects.empty() &&
       ((mKoTurnCounter < 0) || !isAlive()))
    {
        for(EntityParticleEffect* effect : mEntityParticleEffects)
        {
            delete effect;
        }
        mEntityParticleEffects.clear();
    }

    // We apply creature effects if any
    for(auto it = mEntityParticleEffects.begin(); it != mEntityParticleEffects.end();)
    {
        CreatureParticuleEffect* effect = static_cast<CreatureParticuleEffect*>(*it);
        if(effect->mEffect->upkeepEffect(*this))
        {
            ++it;
            continue;
        }

        delete effect;
        it = mEntityParticleEffects.erase(it);
    }

    // if creature is not on map (picked up or being carried), we do nothing
    if(!getIsOnMap())
        return;

    // If the creature is temporary KO, it should do nothing
    if(mKoTurnCounter > 0)
    {
        --mKoTurnCounter;
        if(mKoTurnCounter > 0)
            return;

        computeCreatureOverlayMoodValue();
        return;
    }

    if(mKoTurnCounter < 0)
    {
        // If the counter reaches 0, the creature is dead
        ++mKoTurnCounter;
        if(mKoTurnCounter < 0)
            return;

        mHp = 0;
        computeCreatureOverlayHealthValue();
        computeCreatureOverlayMoodValue();
    }

    // Handle creature death
    if (!isAlive())
    {
        // Let the creature lay dead on the ground for a few turns before removing it from the GameMap.
        if (mDeathCounter == 0)
        {
            OD_LOG_INF("Creature=" + getName() + " RIP");

            dropCarriedEquipment();
        }
        else if (mDeathCounter >= ConfigManager::getSingleton().getCreatureDeathCounter())
        {
            // Remove the creature from the game map and into the deletion queue, it will be deleted
            // when it is safe, i.e. all other pointers to it have been wiped from the program.
            removeFromGameMap();
            deleteYourself();
        }

        ++mDeathCounter;
        return;
    }

    // If the creature is in jail, it should not auto heal or do anything
    if(mSeatPrison != nullptr)
        return;

    // If we are not standing somewhere on the map, do nothing.
    if (getPositionTile() == nullptr)
    {
        OD_LOG_ERR("creature=" + getName() + " not on map position=" + Helper::toString(getPosition()));
        return;
    }

    // Check to see if we have earned enough experience to level up.
    checkLevelUp();


    // Heal.
    mHp += mDefinition->getHpHealPerTurn();
    if (mHp > getMaxHp())
        mHp = getMaxHp();

    computeCreatureOverlayHealthValue();

    // Rogue creatures are not affected by wakefulness/hunger
    if(!getSeat()->isRogueSeat())
    {
        decreaseWakefulness(mDefinition->getWakefulnessLostPerTurn());

        increaseHunger(mDefinition->getHungerGrowthPerTurn());
    }

    mVisibleEnemyObjects         = getVisibleEnemyObjects();
    mVisibleAlliedObjects        = getVisibleAlliedObjects();
    mReachableAlliedObjects      = getReachableAttackableObjects(mVisibleAlliedObjects);

    // Check if we should compute mood
    if(mMoodCooldownTurns > 0)
    {
        --mMoodCooldownTurns;
    }
    // Rogue creatures do not have mood
    else if(!getSeat()->isRogueSeat())
    {
        computeMood();
        computeCreatureOverlayMoodValue();
        mMoodCooldownTurns = Random::Int(0, 5);
    }

    if(mMoodValue < CreatureMoodLevel::Furious)
    {
        mNbTurnFurious = -1;
    }
    else
    {
        // If the creature is furious for too long, it will become rogue
        if(mNbTurnFurious < 0)
            mNbTurnFurious = 0;
        else
            ++mNbTurnFurious;

        if(mNbTurnFurious >= ConfigManager::getSingleton().getNbTurnsFuriousMax())
        {
            // We couldn't leave the dungeon in time, we become rogue
            if((getSeat()->getPlayer() != nullptr) &&
               (getSeat()->getPlayer()->getIsHuman()))
            {
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotificationType::chatServer, getSeat()->getPlayer());
                std::string msg = getName() + " is not under your control anymore !";
                serverNotification->mPacket << msg << EventShortNoticeType::aboutCreatures;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }

            Seat* rogueSeat = getGameMap()->getSeatRogue();
            setSeat(rogueSeat);
            mMoodValue = CreatureMoodLevel::Neutral;
            mMoodPoints = 0;
            mWakefulness = 100;
            mHunger = 0;
            clearDestinations(EntityAnimation::idle_anim, true);
            clearActionQueue();
            mNeedFireRefresh = true;
            if (getHomeTile() != nullptr)
            {
                RoomDormitory* home = static_cast<RoomDormitory*>(getHomeTile()->getCoveringBuilding());
                home->releaseTileForSleeping(getHomeTile(), this);
            }
        }
    }

    ++mNbTurnsWithoutBattle;

    bool isWarmUp = false;
    // We use creature skills if we can
    for(CreatureSkillData& skillData : mSkillData)
    {
        if(skillData.mWarmup > 0)
        {
            --skillData.mWarmup;
            isWarmUp = true;
        }

        if(skillData.mCooldown > 0)
        {
            --skillData.mCooldown;
            continue;
        }

        if(!skillData.mSkill->canBeUsedBy(this))
            continue;

        if(!skillData.mSkill->tryUseSupport(*getGameMap(), this))
            continue;

        skillData.mCooldown = skillData.mSkill->getCooldownNbTurns();
        skillData.mWarmup = skillData.mSkill->getWarmupNbTurns();

        if(skillData.mWarmup > 0)
            isWarmUp = true;
    }

    // If a warmup is active, we do nothing
    if(isWarmUp)
        return;

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

        if (mActions.empty())
            loopBack = handleIdleAction();
        else
        {
            std::function<bool()> func = mActions.back().get()->action();
            loopBack = func();
        }
    } while (loopBack && loops < 20);

    if(!mActions.empty())
        mActions.back().get()->increaseNbTurnActive();

    for(std::unique_ptr<CreatureAction>& creatureAction : mActions)
        creatureAction.get()->increaseNbTurn();

    if(loops >= 20)
    {
        OD_LOG_INF("> 20 loops in Creature::doUpkeep name:" + getName() +
                " seat id: " + Helper::toString(getSeat()->getId()) + ". Breaking out..");
    }
}

void Creature::decidePrioritaryAction()
{
    for(const CreatureBehaviour* behaviour : getDefinition()->getCreatureBehaviours())
    {
        if(!behaviour->processBehaviour(*this))
            return;
    }
}

bool Creature::handleIdleAction()
{
    setAnimationState(EntityAnimation::idle_anim);

    if (mDefinition->isWorker())
    {
        // Decide what to do
        std::vector<CreatureActionType> workerActions = getSeat()->getPlayer()->getWorkerPreferredActions(*this);
        for(CreatureActionType actionType : workerActions)
        {
            if(hasActionBeenTried(actionType))
                continue;

            switch(actionType)
            {
                case CreatureActionType::searchEntityToCarry:
                    pushAction(Utils::make_unique<CreatureActionSearchEntityToCarry>(*this, false));
                    return true;
                case CreatureActionType::searchGroundTileToClaim:
                    pushAction(Utils::make_unique<CreatureActionSearchGroundTileToClaim>(*this, false));
                    return true;
                case CreatureActionType::searchTileToDig:
                    pushAction(Utils::make_unique<CreatureActionSearchTileToDig>(*this, false));
                    return true;
                case CreatureActionType::searchWallTileToClaim:
                    pushAction(Utils::make_unique<CreatureActionSearchWallTileToClaim>(*this, false));
                    return true;
                default:
                    OD_LOG_ERR("name=" + getName() + ", unexpected worker action=" + CreatureAction::toString(actionType));
                    break;
            }
        }
    }

    // We check if we are looking for our fee
    if(!mDefinition->isWorker() &&
       !hasActionBeenTried(CreatureActionType::getFee) &&
       (Random::Double(0.0, 1.0) < 0.5) &&
       (mGoldFee > 0))
    {
        pushAction(Utils::make_unique<CreatureActionGetFee>(*this));
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
            if(tempPath.size() >= 5)
            {
                std::vector<Ogre::Vector3> path;
                tileToVector3(tempPath, path, true, 0.0);
                setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, path);
                pushAction(Utils::make_unique<CreatureActionWalkToTile>(*this));
                return false;
            }
        }
    }

    // Check to see if we have found a "home" tile where we can sleep. Even if we are not sleepy,
    // we want to have a bed
    if (!mDefinition->isWorker() &&
        !hasActionBeenTried(CreatureActionType::findHome) &&
        (mHomeTile == nullptr) &&
        (Random::Double(0.0, 1.0) < 0.5))
    {
        // Check to see if there are any dormitory owned by our color that we can reach.
        std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndSeat(RoomType::dormitory, getSeat());
        tempRooms = getGameMap()->getReachableRooms(tempRooms, getPositionTile(), this);
        if (!tempRooms.empty())
        {
            pushAction(Utils::make_unique<CreatureActionFindHome>(*this, false));
            return true;
        }
    }

    // If we are sleepy, we go to sleep
    if (!mDefinition->isWorker() &&
        !hasActionBeenTried(CreatureActionType::sleep) &&
        (mHomeTile != nullptr) &&
        (Random::Double(20.0, 30.0) > mWakefulness))
    {
        pushAction(Utils::make_unique<CreatureActionSleep>(*this));
        return true;
    }

    // If we are hungry, we go to eat
    if (!mDefinition->isWorker() &&
        !hasActionBeenTried(CreatureActionType::searchFood) &&
        (Random::Double(70.0, 80.0) < mHunger))
    {
        pushAction(Utils::make_unique<CreatureActionSearchFood>(*this, false));
        return true;
    }

    // Otherwise, we try to work
    if (!mDefinition->isWorker() &&
        !hasActionBeenTried(CreatureActionType::searchJob) &&
        (Random::Double(0.0, 1.0) < 0.4))
    {
        pushAction(Utils::make_unique<CreatureActionSearchJob>(*this, false));
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
        if (r < 0.7)
        {
            bool workerFound = false;
            // Try to find a worker to follow around.
            for (unsigned int i = 0; !workerFound && i < mReachableAlliedObjects.size(); ++i)
            {
                // Check to see if we found a worker.
                if (mReachableAlliedObjects[i]->getObjectType() == GameEntityType::creature
                    && static_cast<Creature*>(mReachableAlliedObjects[i])->mDefinition->isWorker())
                {
                    // We found a worker so find a tile near the worker to walk to.  See if the worker is digging.
                    Tile* tempTile = mReachableAlliedObjects[i]->getCoveredTile(0);
                    if (static_cast<Creature*>(mReachableAlliedObjects[i])->isActionInList(CreatureActionType::digTile))
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

bool Creature::searchBestTargetInList(const std::vector<GameEntity*>& listObjects, GameEntity*& attackedEntity, Tile*& attackedTile, Tile*& positionTile, CreatureSkillData*& creatureSkillData)
{
    Tile* myTile = getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + getName() + ", position=" + Helper::toString(getPosition()));
        return false;
    }

    GameEntity* entityFlee = nullptr;
    // Closest creature
    GameEntity* entityAttack = nullptr;
    Tile* tileAttack = nullptr;
    CreatureSkillData* skillData = nullptr;
    Tile* tilePosition = nullptr;
    int closestDist = -1;
    // We try to attack creatures first
    for(GameEntity* entity : listObjects)
    {
        GameEntity* entityAttackCheck = nullptr;
        Tile* tileAttackCheck = nullptr;
        CreatureSkillData* skillDataCheck = nullptr;
        int closestDistCheck = closestDist;
        // We check if this creature is closer than the other one (if any)
        std::vector<Tile*> coveredTiles = entity->getCoveredTiles();
        for(Tile* tile : coveredTiles)
        {
            if(std::find(mVisibleTiles.begin(), mVisibleTiles.end(), tile) == mVisibleTiles.end())
                continue;

            int dist = Pathfinding::squaredDistanceTile(*tile, *myTile);
            if((closestDistCheck != -1) && (dist >= closestDistCheck))
                continue;

            // We found a tile closer
            // Note that we don't break because if this entity is on more than 1 tile,
            // we want to attack the closest tile
            closestDistCheck = dist;
            entityAttackCheck = entity;
            tileAttackCheck = tile;
        }

        if((entityAttackCheck == nullptr) || (tileAttackCheck == nullptr))
            continue;

        // We check if we are supposed to flee from this entity
        if((entityFlee == nullptr) && entityAttackCheck->isDangerous(this, closestDistCheck))
            entityFlee = entityAttackCheck;

        // If we found a suitable enemy, we check if we can attack it
        double skillRangeMax = 0.0;
        for(CreatureSkillData& skillDataTmp : mSkillData)
        {
            if(skillDataTmp.mCooldown > 0)
                continue;

            if(!skillDataTmp.mSkill->canBeUsedBy(this))
                continue;

            double skillRange = skillDataTmp.mSkill->getRangeMax(this, entityAttackCheck);
            if(skillRange <= 0)
                continue;
            if(skillRange < skillRangeMax)
                continue;

            skillRangeMax = skillRange;
            skillDataCheck = &skillDataTmp;
        }
        if(skillRangeMax <= 0)
            continue;

        // Check if we can attack from our position
        int rangeTarget = Pathfinding::squaredDistanceTile(*tileAttackCheck, *myTile);
        if(rangeTarget <= (skillRangeMax * skillRangeMax))
        {
             // We can attack
             if((closestDist == -1) || (rangeTarget < closestDist))
             {
                tilePosition = myTile;
                entityAttack = entityAttackCheck;
                tileAttack = tileAttackCheck;
                skillData = skillDataCheck;
                closestDist = rangeTarget;
             }
             continue;
        }

        // We check if we can attack from somewhere. To do that, we check
        // from the target point of view if there is a tile with visibility within range
        int skillRangeMaxInt = static_cast<int>(skillRangeMax);
        int skillRangeMaxIntSquared = skillRangeMaxInt * skillRangeMaxInt;
        int bestScoreAttack = -1;
        std::vector<Tile*> tiles = getAccessibleVisibleTiles(tileAttackCheck, skillRangeMaxInt);
        for(Tile* tile : tiles)
        {
            if(tile->isFullTile())
                continue;

            if(!getGameMap()->pathExists(this, myTile, tile))
                continue;

            int distFoeTmp = Pathfinding::squaredDistanceTile(*tile, *tileAttackCheck);
            int distAttackTmp = Pathfinding::squaredDistanceTile(*tile, *myTile);
            // We compute a score for each tile. We will choose the best one. Note that we try to be as close as possible
            // from the fightIdleDist but by walking the less possible. We need to find a compromise
            int scoreAttack = std::abs(skillRangeMaxIntSquared - distFoeTmp) * 2 + distAttackTmp;
            if((bestScoreAttack != -1) && (bestScoreAttack <= scoreAttack))
                continue;

            // We found a better target
            bestScoreAttack = scoreAttack;
            tilePosition = tile;
            entityAttack = entityAttackCheck;
            tileAttack = tileAttackCheck;
            skillData = skillDataCheck;
            closestDist = closestDistCheck;
            // We don't break because there might be a better spot
        }
    }

    // If there is a dangerous entity and we cannot attack, we should try to get away
    if((entityFlee != nullptr) && (skillData == nullptr))
    {
        // Let's try to run to the closest spot at the distance closest to the fight idle distance
        Tile* tileEntityFlee = entityFlee->getPositionTile();
        if(tileEntityFlee == nullptr)
        {
            OD_LOG_ERR("entity=" + entityFlee->getName() + ", position=" + Helper::toString(entityFlee->getPosition()));
            return false;
        }

        int bestScoreFlee = -1;
        int32_t fightIdleDist = getDefinition()->getFightIdleDist();
        Tile* fleeTile = nullptr;
        std::vector<Tile*> tiles = getAccessibleVisibleTiles(tileEntityFlee, fightIdleDist);
        int32_t fightIdleDistSquared = fightIdleDist * fightIdleDist;
        for(Tile* tile : tiles)
        {
            if(tile->isFullTile())
                continue;

            if(!getGameMap()->pathExists(this, myTile, tile))
                continue;

            int distFoeTmp = Pathfinding::squaredDistanceTile(*tile, *tileEntityFlee);
            int fleeDistTmp = Pathfinding::squaredDistanceTile(*tile, *myTile);
            // We compute a score for each tile. We will choose the best one. Note that we try to be as close as possible
            // from the fightIdleDist but by walking the less possible. We need to find a compromise
            int scoreFlee = std::abs(fightIdleDistSquared - distFoeTmp) * 2 + fleeDistTmp;
            if((bestScoreFlee != -1) && (bestScoreFlee <= scoreFlee))
                continue;

            bestScoreFlee = scoreFlee;
            fleeTile = tile;
        }

        if(fleeTile == nullptr)
            return false;

        attackedEntity = nullptr;
        attackedTile = nullptr;
        positionTile = fleeTile;
    }
    else if ((entityAttack == nullptr) ||
        (tileAttack == nullptr) ||
        (tilePosition == nullptr))
    {
        // We couldn't find an entity to attack
        return false;
    }
    else
    {
        attackedEntity = entityAttack;
        attackedTile = tileAttack;
        creatureSkillData = skillData;
        positionTile = tilePosition;
    }

    return true;
}

void Creature::engageAlliedNaturalEnemy(Creature& attackerCreature)
{
    Tile* myTile = getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + getName() + ", pos=" + Helper::toString(getPosition()));
        return;
    }

    // If we are already fighting, do nothing
    if(isActionInList(CreatureActionType::fight))
        return;

    fightCreature(attackerCreature);
}

double Creature::getMoveSpeed() const
{
    return getMoveSpeed(getPositionTile());
}

double Creature::getMoveSpeed(Tile* tile) const
{
    if(tile == nullptr)
    {
        OD_LOG_ERR("creature=" + getName());
        return 1.0;
    }

    if(getIsOnServerMap())
    {
        // Check if the covering building allows this creature to go through
        if(tile->getCoveringBuilding() != nullptr)
            return tile->getCoveringBuilding()->getCreatureSpeed(this, tile);
        else
            return tile->getCreatureSpeedDefault(this);
    }
    else
    {
        if(tile->getHasBridge())
            return getMoveSpeedGround();
        else
            return tile->getCreatureSpeedDefault(this);
    }
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

double Creature::getElementDefense() const
{
    double defense = mElementDefense;
    if (mWeaponL != nullptr)
        defense += mWeaponL->getElementDefense();
    if (mWeaponR != nullptr)
        defense += mWeaponR->getElementDefense();

    return defense;
}

void Creature::checkLevelUp()
{
    if (getLevel() >= MAX_LEVEL)
        return;

    // Check the returned value.
    double newXP = mDefinition->getXPNeededWhenLevel(getLevel());

    // An error occurred
    if (newXP <= 0.0)
    {
        OD_LOG_ERR("creature=" + getName() + ", newXP=" + Helper::toString(newXP));
        return;
    }

    if (mExp < newXP)
        return;

    setLevel(mLevel + 1);
}

void Creature::exportToPacketForUpdate(ODPacket& os, const Seat* seat) const
{
    int seatId = getSeat()->getId();
    os << mLevel;
    os << seatId;
    os << mOverlayHealthValue;

    // Only allied players should see creature mood (except some states)
    uint32_t moodValue = 0;
    if(seat->isAlliedSeat(getSeat()))
        moodValue = mOverlayMoodValue;
    else if(mSeatPrison != nullptr)
    {
        if(mSeatPrison->isAlliedSeat(seat))
            moodValue = mOverlayMoodValue & CreatureMoodEnum::MoodPrisonFiltersPrisonAllies;
        else
            moodValue = mOverlayMoodValue & CreatureMoodEnum::MoodPrisonFiltersAllPlayers;
    }

    os << moodValue;
    os << mGroundSpeed;
    os << mWaterSpeed;
    os << mLavaSpeed;
    os << mSpeedModifier;

    uint32_t nbCreatureEffect = mEntityParticleEffects.size();
    os << nbCreatureEffect;
    for(EntityParticleEffect* effect : mEntityParticleEffects)
    {
        os << effect->mName;
        os << effect->mScript;
        os << effect->mNbTurnsEffect;
    }

    int seatPrisonId = -1;
    if(mSeatPrison != nullptr)
        seatPrisonId = mSeatPrison->getId();

    os << seatPrisonId;
}

void Creature::updateFromPacket(ODPacket& is)
{
    int seatId;
    OD_ASSERT_TRUE(is >> mLevel);
    OD_ASSERT_TRUE(is >> seatId);
    OD_ASSERT_TRUE(is >> mOverlayHealthValue);
    OD_ASSERT_TRUE(is >> mOverlayMoodValue);
    OD_ASSERT_TRUE(is >> mGroundSpeed);
    OD_ASSERT_TRUE(is >> mWaterSpeed);
    OD_ASSERT_TRUE(is >> mLavaSpeed);
    OD_ASSERT_TRUE(is >> mSpeedModifier);

    // We do not scale the creature if it is picked up (because it is already not at its normal size). It will be
    // resized anyway when dropped
    updateScale();
    if(getIsOnMap())
        RenderManager::getSingleton().rrScaleEntity(this);

    if(getSeat()->getId() != seatId)
    {
        Seat* seat = getGameMap()->getSeatById(seatId);
        if(seat == nullptr)
        {
            OD_LOG_ERR("Creature " + getName() + ", wrong seatId=" + Helper::toString(seatId));
        }
        else
        {
            setSeat(seat);
        }
    }

    uint32_t nbEffects;
    OD_ASSERT_TRUE(is >> nbEffects);

    // We copy the list of effects currently on this creature. That will allow to
    // check if the effect is already known and only display the effect if it is not
    std::vector<EntityParticleEffect*> currentEffects = mEntityParticleEffects;
    while(nbEffects > 0)
    {
        --nbEffects;

        std::string effectName;
        std::string effectScript;
        uint32_t nbTurns;
        OD_ASSERT_TRUE(is >> effectName >> effectScript >> nbTurns);
        bool isEffectAlreadyDisplayed = false;
        for(EntityParticleEffect* effect : currentEffects)
        {
            if(effect->mName.compare(effectName) != 0)
                continue;

            isEffectAlreadyDisplayed = true;
            break;
        }

        if(isEffectAlreadyDisplayed)
            continue;

        CreatureParticuleEffectClient* effect = new CreatureParticuleEffectClient(effectName, effectScript, nbTurns);
        effect->mParticleSystem = RenderManager::getSingleton().rrEntityAddParticleEffect(this,
            effect->mName, effect->mScript);
        mEntityParticleEffects.push_back(effect);
    }

    OD_ASSERT_TRUE(is >> seatId);
    if(seatId == -1)
        mSeatPrison = nullptr;
    else
    {
        mSeatPrison = getGameMap()->getSeatById(seatId);
        if(mSeatPrison == nullptr)
        {
            OD_LOG_ERR("Creature " + getName() + ", wrong seatId=" + Helper::toString(seatId));
        }
    }
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

std::vector<GameEntity*> Creature::getCreaturesFromList(const std::vector<GameEntity*> &objectsToCheck, bool workersOnly)
{
    std::vector<GameEntity*> tempVector;

    // Loop over the vector of objects we are supposed to check.
    for (std::vector<GameEntity*>::const_iterator it = objectsToCheck.begin(); it != objectsToCheck.end(); ++it)
    {
        // Try to find a valid path from the tile this creature is in to the nearest tile where the current target object is.
        GameEntity* entity = *it;
        // We only consider alive objects
        if(entity->getObjectType() != GameEntityType::creature)
            continue;

        if(workersOnly && !static_cast<Creature*>(entity)->getDefinition()->isWorker())
            continue;

        tempVector.push_back(entity);
    }

    return tempVector;
}

std::vector<GameEntity*> Creature::getVisibleAlliedObjects()
{
    return getVisibleForce(getSeat(), false);
}

std::vector<GameEntity*> Creature::getVisibleForce(Seat* seat, bool invert)
{
    return getGameMap()->getVisibleForce(mVisibleTiles, seat, invert);
}

void Creature::computeVisualDebugEntities()
{
    if(!getIsOnServerMap())
        return;

    mHasVisualDebuggingEntities = true;

    updateTilesInSight();

    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::refreshCreatureVisDebug, nullptr);

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
    if(getIsOnServerMap())
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
    if(!getIsOnServerMap())
        return;

    mHasVisualDebuggingEntities = false;

    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::refreshCreatureVisDebug, nullptr);
    const std::string& name = getName();
    serverNotification->mPacket << name;
    serverNotification->mPacket << false;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void Creature::destroyVisualDebugEntities()
{
    if(getIsOnServerMap())
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
    if(index > 0)
    {
        OD_LOG_ERR("name=" + getName() + ", index=" + Helper::toString(index));
        return nullptr;
    }

    return getPositionTile();
}

uint32_t Creature::numCoveredTiles() const
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
        ClientNotificationType::askCreatureInfos);
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
            ClientNotificationType::askCreatureInfos);
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
        tempSS << "Wakefulness: " << mWakefulness << std::endl;
        tempSS << "Hunger: " << mHunger << std::endl;
    }
    tempSS << "Move speed (Ground/Water/Lava): " << getMoveSpeedGround() << "/"
        << getMoveSpeedWater() << "/" << getMoveSpeedLava() << std::endl;
    tempSS << "Weapons:" << std::endl;
    if(mWeaponL == nullptr)
        tempSS << "Left hand: none" << std::endl;
    else
        tempSS << "Left hand: " << mWeaponL->getName() << "-Atk(Phy/Mag): " << mWeaponL->getPhysicalDamage() << "/" << mWeaponL->getMagicalDamage() << std::endl;
    if(mWeaponR == nullptr)
        tempSS << "Right hand: none" << std::endl;
    else
        tempSS << "Right hand: " << mWeaponR->getName() << "-Atk(Phy/Mag): " << mWeaponR->getPhysicalDamage() << "/" << mWeaponR->getMagicalDamage() << std::endl;
    tempSS << "Total Defense (Phys/Mag): " << getPhysicalDefense() << "/" << getMagicalDefense() << std::endl;
    if (getDefinition()->isWorker())
    {
        tempSS << "Dig Rate: : " << getDigRate() << std::endl;
        tempSS << "Dance Rate: : " << mClaimRate << std::endl;
    }
    tempSS << "Actions:";
    for(const std::unique_ptr<CreatureAction>& ca : mActions)
    {
        tempSS << " " << CreatureAction::toString(ca.get()->getType());
    }
    tempSS << std::endl;
    tempSS << "Destinations:";
    for(const Ogre::Vector3& dest : mWalkQueue)
    {
        tempSS << Helper::toString(dest) << "/";
    }
    tempSS << std::endl;
    tempSS << "Seat id: " << getSeat()->getId() << std::endl;
    tempSS << "Team id: " << getSeat()->getTeamId() << std::endl;
    tempSS << "Position: " << Helper::toString(getPosition()) << std::endl;
    tempSS << "Mood: " << CreatureMood::toString(mMoodValue) << std::endl;
    tempSS << "MoodPoints: " << Helper::toString(mMoodPoints) << std::endl;
    return tempSS.str();
}

double Creature::takeDamage(GameEntity* attacker, double absoluteDamage, double physicalDamage, double magicalDamage, double elementDamage,
        Tile *tileTakingDamage, bool ko)
{
    mNbTurnsWithoutBattle = 0;
    physicalDamage = std::max(physicalDamage - getPhysicalDefense(), 0.0);
    magicalDamage = std::max(magicalDamage - getMagicalDefense(), 0.0);
    elementDamage = std::max(elementDamage - getElementDefense(), 0.0);
    double damageDone = std::min(mHp, absoluteDamage + physicalDamage + magicalDamage + elementDamage);
    mHp -= damageDone;
    if(mHp <= 0)
    {
        // If the attacking entity is a creature and its seat is configured to KO creatures
        // instead of killing, we KO
        if(ko)
        {
            mHp = 1.0;
            mKoTurnCounter = -ConfigManager::getSingleton().getNbTurnsKoCreatureAttacked();
            OD_LOG_INF("creature=" + getName() + " has been KO by " + attacker->getName());
            dropCarriedEquipment();
        }
    }

    computeCreatureOverlayHealthValue();
    computeCreatureOverlayMoodValue();

    if(!isAlive())
        fireEntityDead();

    if(!getIsOnServerMap())
        return damageDone;

    Player* player = getGameMap()->getPlayerBySeat(getSeat());
    if (player == nullptr)
        return damageDone;

    // Tells the server game map the player is under attack.
    getGameMap()->playerIsFighting(player, tileTakingDamage);

    // If we are a worker attacked by a worker, we fight. Otherwise, we flee (if it is a fighter, a trap,
    // or whatever)
    if(!getDefinition()->isWorker())
        return damageDone;

    bool shouldFlee = true;
    if((attacker != nullptr) &&
       (attacker->getObjectType() == GameEntityType::creature))
    {
        Creature* creatureAttacking = static_cast<Creature*>(attacker);
        if(creatureAttacking->getDefinition()->isWorker())
        {
            // We do not flee because of this attack
            shouldFlee = false;
        }
    }

    if(shouldFlee)
    {
        flee();
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

void Creature::useAttack(CreatureSkillData& skillData, GameEntity& entityAttack,
        Tile& tileAttack, bool ko)
{
    // Turn to face the entity we are attacking and set the animation state to Attack.
    const Ogre::Vector3& pos = getPosition();
    Ogre::Vector3 walkDirection(tileAttack.getX() - pos.x, tileAttack.getY() - pos.y, 0);
    walkDirection.normalise();
    setAnimationState(EntityAnimation::attack_anim, false, walkDirection);
    fireCreatureSound(CreatureSound::Attack);
    setNbTurnsWithoutBattle(0);

    // Calculate how much damage we do.
    Tile* myTile = getPositionTile();
    float range = Pathfinding::distanceTile(*myTile, tileAttack);

    // We use the skill
    skillData.mSkill->tryUseFight(*getGameMap(), this, range,
        &entityAttack, &tileAttack, ko);
    skillData.mWarmup = skillData.mSkill->getWarmupNbTurns();
    skillData.mCooldown = skillData.mSkill->getCooldownNbTurns();

    // Fighting is tiring
    decreaseWakefulness(0.5);
    // but gives experience
    receiveExp(1.5);
}

bool Creature::isActionInList(CreatureActionType action) const
{
    for (const std::unique_ptr<CreatureAction>& ca : mActions)
    {
        if (ca.get()->getType() == action)
            return true;
    }
    return false;
}

void Creature::clearActionQueue()
{
    mActions.clear();
}

bool Creature::hasActionBeenTried(CreatureActionType actionType) const
{
    if(std::find(mActionTry.begin(), mActionTry.end(), actionType) == mActionTry.end())
        return false;

    return true;
}

void Creature::pushAction(std::unique_ptr<CreatureAction>&& action)
{
    CreatureActionType actionType = action.get()->getType();
    if(std::find(mActionTry.begin(), mActionTry.end(), actionType) == mActionTry.end())
    {
        mActionTry.push_back(actionType);
    }

    mActions.emplace_back(std::move(action));
}

void Creature::popAction()
{
    if(mActions.empty())
    {
        OD_LOG_ERR("name=" + getName() + ", trying to pop empty action list");
        return;
    }

    mActions.pop_back();
}

bool Creature::tryPickup(Seat* seat)
{
    if(!getIsOnMap())
        return false;

    // Cannot pick up dead creatures
    if (!getGameMap()->isInEditorMode() && !isAlive())
        return false;

    if(!getGameMap()->isInEditorMode() && (mSeatPrison == nullptr) && !getSeat()->canOwnedCreatureBePickedUpBy(seat))
        return false;

    if(!getGameMap()->isInEditorMode() && (mSeatPrison != nullptr) && !mSeatPrison->canOwnedCreatureBePickedUpBy(seat))
        return false;

    // KO creatures cannot be picked up
    if(isKo())
        return false;

    return true;
}

void Creature::pickup()
{
    // Stop the creature walking and set it off the map to prevent the AI from running on it.
    removeEntityFromPositionTile();
    clearDestinations(EntityAnimation::idle_anim, true);
    clearActionQueue();

    if(!getIsOnServerMap())
        return;

    if(getHasVisualDebuggingEntities())
        computeVisualDebugEntities();

    fireCreatureSound(CreatureSound::Pickup);
}

bool Creature::canGoThroughTile(Tile* tile) const
{
    if(tile == nullptr)
        return false;

    return getMoveSpeed(tile) > 0.0;
}

bool Creature::tryDrop(Seat* seat, Tile* tile)
{
    // check whether the tile is a ground tile ...
    if(tile->isFullTile())
        return false;

    // In editor mode, we allow creatures to be dropped anywhere they can walk
    if(getGameMap()->isInEditorMode() && canGoThroughTile(tile))
        return true;

    // we cannot drop a creature on a tile we don't see
    if(!seat->hasVisionOnTile(tile))
        return false;

    // If it is a worker, he can be dropped on dirt
    if (getDefinition()->isWorker() && (tile->getTileVisual() == TileVisual::dirtGround || tile->getTileVisual() == TileVisual::goldGround))
        return true;

    // Every creature can be dropped on allied claimed tiles
    if(tile->isClaimedForSeat(seat))
        return true;

    return false;
}

void Creature::drop(const Ogre::Vector3& v)
{
    setPosition(v);
    if(!getIsOnServerMap())
    {
        mDropCooldown = 2;
        return;
    }

    if(getHasVisualDebuggingEntities())
        computeVisualDebugEntities();

    fireCreatureSound(CreatureSound::Drop);

    // The creature is temporary KO
    mKoTurnCounter = mDefinition->getTurnsStunDropped();
    computeCreatureOverlayMoodValue();

    // Action queue should be empty but it shouldn't hurt
    clearActionQueue();

    // In editor mode, we do not check for forced actions
    if(getGameMap()->isInEditorMode())
        return;

    if(mDefinition->isWorker())
    {
        // If a worker is dropped, he will search in the tile he is and in the 4 neighboor tiles.
        // 1 - If the tile he is in a treasury and he is carrying gold, he should deposit it
        // 2 - if one of the 4 neighboor tiles is marked, he will dig
        // 3 - if there is a carryable entity where it is dropped, it should try to carry it
        // 4 - if the the tile he is in is not claimed and one of the neigbboor tiles is claimed, he will claim
        // 5 - if the the tile he is in is claimed and one of the neigbboor tiles is not claimed, he will claim
        // 6 - If the tile he is in is claimed and one of the neigbboor tiles is a not claimed wall, he will claim
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
                tile->isClaimedForSeat(seat) &&
                position->isGroundClaimable(seat)
                )
            {
                tileToClaim = position;
            }
            else if(tileToClaim == nullptr &&
                position->isClaimedForSeat(seat) &&
                tile->isGroundClaimable(seat)
                )
            {
                tileToClaim = tile;
            }
            else if(tileWallNotClaimed == nullptr &&
                position->isClaimedForSeat(seat) &&
                tile->isWallClaimable(seat)
                )
            {
                tileWallNotClaimed = tile;
            }
        }

        // We try to deposit gold if we are on a room while carrying gold
        if((mGoldCarried > 0) && (mDigRate > 0.0) &&
           (position->getCoveringRoom() != nullptr))
        {
            int deposited = position->getCoveringRoom()->depositGold(mGoldCarried, position);
            if(deposited > 0)
            {
                mGoldCarried -= deposited;
                return;
            }
        }

        std::vector<GameEntity*> carryable;
        position->fillWithCarryableEntities(this, carryable);

        // Now, we can decide
        if((tileMarkedDig != nullptr) && (mDigRate > 0.0))
        {
            pushAction(Utils::make_unique<CreatureActionSearchTileToDig>(*this, true));
            pushAction(Utils::make_unique<CreatureActionDigTile>(*this, *tileMarkedDig));
            return;
        }

        if(!carryable.empty())
        {
            // We look for the most important entity to carry
            GameEntity* entityToCarry = carryable[0];
            for(GameEntity* entity : carryable)
            {
                // We check that the entity is free to be carried
                if(entity->getCarryLock(*this))
                    continue;

                if(entity->getEntityCarryType(this) <= entityToCarry->getEntityCarryType(this))
                    continue;

                entityToCarry = entity;
            }

            pushAction(Utils::make_unique<CreatureActionGrabEntity>(*this, *entityToCarry));
            return;
        }

        if((tileToClaim != nullptr) && (mClaimRate > 0.0))
        {
            pushAction(Utils::make_unique<CreatureActionSearchGroundTileToClaim>(*this, true));
            pushAction(Utils::make_unique<CreatureActionClaimGroundTile>(*this, *tileToClaim));
            return;
        }

        if((tileWallNotClaimed != nullptr) && (mClaimRate > 0.0))
        {
            pushAction(Utils::make_unique<CreatureActionSearchWallTileToClaim>(*this, true));
            pushAction(Utils::make_unique<CreatureActionClaimWallTile>(*this, *tileWallNotClaimed));
            return;
        }

        // We couldn't find why we were dropped here. Let's behave as usual
        return;
    }

    // Fighters
    Tile* tile = getPositionTile();
    if((tile != nullptr) &&
       (tile->getCoveringRoom() != nullptr))
    {
        Room* room = tile->getCoveringRoom();
        // we see if we are in an hatchery
        if(room->getType() == RoomType::hatchery)
        {
            pushAction(Utils::make_unique<CreatureActionSearchFood>(*this, true));
            return;
        }

        if(room->getType() == RoomType::dormitory)
        {
            pushAction(Utils::make_unique<CreatureActionSleep>(*this));
            pushAction(Utils::make_unique<CreatureActionFindHome>(*this, true));
            return;
        }

        // If not, can we work in this room ?
        if(room->getType() != RoomType::hatchery)
        {
            pushAction(Utils::make_unique<CreatureActionSearchJob>(*this, true));
            return;
        }
    }
}

bool Creature::setDestination(Tile* tile)
{
    if(tile == nullptr)
        return false;

    Tile *posTile = getPositionTile();
    if(posTile == nullptr)
        return false;

    std::list<Tile*> result = getGameMap()->path(this, tile);

    std::vector<Ogre::Vector3> path;
    tileToVector3(result, path, true, 0.0);
    setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, path);
    pushAction(Utils::make_unique<CreatureActionWalkToTile>(*this));
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
    setDestination(tileDestination);
    return false;
}

bool Creature::isAttackable(Tile* tile, Seat* seat) const
{
    if(mHp <= 0.0)
        return false;

    // KO Creature to death creatures are not a threat and cannot be attacked. However, temporary KO can be
    if(mKoTurnCounter < 0)
        return false;

    // Creatures in prison are not a treat and cannot be attacked
    if(mSeatPrison != nullptr)
        return false;

    return true;
}

EntityCarryType Creature::getEntityCarryType(Creature* carrier)
{
    // KO to death entities can be carried
    if(mKoTurnCounter < 0)
        return EntityCarryType::koCreature;

    // Dead creatures are carryable
    if(getHP() <= 0.0)
        return EntityCarryType::corpse;

    return EntityCarryType::notCarryable;
}

void Creature::notifyEntityCarryOn(Creature* carrier)
{
    removeEntityFromPositionTile();
}

void Creature::notifyEntityCarryOff(const Ogre::Vector3& position)
{
    mPosition = position;
    addEntityToPositionTile();
}

bool Creature::canBeCarriedToBuilding(const Building* building) const
{
    // If the creature is dead, it can be carried to any crypt
    if(!isAlive() &&
       (building->getObjectType() == GameEntityType::room) &&
       (static_cast<const Room*>(building)->getType() == RoomType::crypt))
    {
        return true;
    }

    // If the creature is ko to death, it can be carried to an enemy prison
    if((mKoTurnCounter < 0) &&
       (building->getObjectType() == GameEntityType::room) &&
       (!building->getSeat()->isAlliedSeat(getSeat())) &&
       (static_cast<const Room*>(building)->getType() == RoomType::prison))
    {
        return true;
    }

    // If the creature is ko to death, it can be carried to its bed
    if((mKoTurnCounter < 0) &&
       (mHomeTile != nullptr) &&
       (mHomeTile->getCoveringBuilding() == building))
    {
        return true;
    }

    return false;
}

void Creature::carryEntity(GameEntity* carriedEntity)
{
    if(!getIsOnServerMap())
        return;

    OD_ASSERT_TRUE(carriedEntity != nullptr);
    OD_ASSERT_TRUE(mCarriedEntity == nullptr);
    mCarriedEntity = nullptr;
    if(carriedEntity == nullptr)
        return;

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
    if(!getIsOnServerMap())
        return;

    GameEntity* carriedEntity = mCarriedEntity;
    mCarriedEntity = nullptr;
    if(carriedEntity == nullptr)
    {
        OD_LOG_ERR("name=" + getName());
        return;
    }

    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        ServerNotification* serverNotification = new ServerNotification(
            ServerNotificationType::releaseCarriedEntity, seat->getPlayer());
        serverNotification->mPacket << getName() << carriedEntity->getObjectType();
        serverNotification->mPacket << carriedEntity->getName();
        serverNotification->mPacket << mPosition;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

bool Creature::canSlap(Seat* seat)
{
    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        return false;
    }

    if(mDropCooldown > 0)
        return false;

    if(getGameMap()->isInEditorMode())
        return true;

    if(getHP() <= 0.0)
        return false;

    // If the creature is in prison, it can be slapped by the jail owner only
    if(mSeatPrison != nullptr)
        return (mSeatPrison == seat);

    // Only the owning player can slap a creature
    if(getSeat() != seat)
        return false;

    return true;
}

void Creature::slap()
{
    if(!getIsOnServerMap())
        return;

    fireCreatureSound(CreatureSound::Slap);

    // In editor mode, we remove the creature
    if(getGameMap()->isInEditorMode())
    {
        removeFromGameMap();
        deleteYourself();
        return;
    }

    CreatureEffectSlap* effect = new CreatureEffectSlap(
        ConfigManager::getSingleton().getSlapEffectDuration(), "");
    addCreatureEffect(effect);
    mHp -= mMaxHP * ConfigManager::getSingleton().getSlapDamagePercent() / 100.0;
    computeCreatureOverlayHealthValue();
}

void Creature::fireAddEntity(Seat* seat, bool async)
{
    if(async)
    {
        ServerNotification serverNotification(
            ServerNotificationType::addEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification.mPacket);
        exportToPacket(serverNotification.mPacket, seat);
        ODServer::getSingleton().sendAsyncMsg(serverNotification);

        if(mCarriedEntity != nullptr)
        {
            OD_LOG_ERR("Trying to fire add creature in async mode name=" + getName() + " while carrying " + mCarriedEntity->getName());
        }
        return;
    }

    ServerNotification* serverNotification = new ServerNotification(
        ServerNotificationType::addEntity, seat->getPlayer());
    exportHeadersToPacket(serverNotification->mPacket);
    exportToPacket(serverNotification->mPacket, seat);
    ODServer::getSingleton().queueServerNotification(serverNotification);

    if(mCarriedEntity != nullptr)
    {
        mCarriedEntity->addSeatWithVision(seat, false);

        serverNotification = new ServerNotification(
            ServerNotificationType::carryEntity, seat->getPlayer());
        serverNotification->mPacket << getName() << mCarriedEntity->getObjectType();
        serverNotification->mPacket << mCarriedEntity->getName();
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Creature::fireRemoveEntity(Seat* seat)
{
    // If we are carrying an entity, we release it first, then we can remove it and us
    if(mCarriedEntity != nullptr)
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotificationType::releaseCarriedEntity, seat->getPlayer());
        serverNotification->mPacket << getName() << mCarriedEntity->getObjectType();
        serverNotification->mPacket << mCarriedEntity->getName();
        serverNotification->mPacket << mPosition;
        ODServer::getSingleton().queueServerNotification(serverNotification);

        mCarriedEntity->removeSeatWithVision(seat);
    }

    const std::string& name = getName();
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::removeEntity, seat->getPlayer());
    GameEntityType type = getObjectType();
    serverNotification->mPacket << type;
    serverNotification->mPacket << name;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void Creature::fireCreatureRefreshIfNeeded()
{
    if(!mNeedFireRefresh)
        return;

    mNeedFireRefresh = false;
    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getName();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::entitiesRefresh, seat->getPlayer());
        uint32_t nbCreature = 1;
        serverNotification->mPacket << nbCreature;
        serverNotification->mPacket << GameEntityType::creature;
        serverNotification->mPacket << name;
        exportToPacketForUpdate(serverNotification->mPacket, seat);
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Creature::setupDefinition(GameMap& gameMap, const CreatureDefinition& defaultWorkerCreatureDefinition)
{
    bool setHpToStrHp = false;
    if(mDefinition == nullptr)
    {
        // If the classname corresponds to the default worker CreatureDefinition, we use
        // the dedicated class. The correct one will be set after the seat is initialized
        if(!mDefinitionString.empty() &&  mDefinitionString.compare(ConfigManager::DefaultWorkerCreatureDefinition) != 0)
        {
            mDefinition = gameMap.getClassDescription(mDefinitionString);
        }
        else
        {
            // If we are in editor mode, we take the default worker class. Otherwise, we take
            // the default worker from the seat faction
            if(gameMap.isInEditorMode() || !mSeat)
                mDefinition = &defaultWorkerCreatureDefinition;
            else
                mDefinition = getSeat()->getWorkerClassToSpawn();
        }

        if(mDefinition == nullptr)
        {
            OD_LOG_ERR("Definition=" + mDefinitionString);
            return;
        }

        if(getIsOnServerMap())
        {
            setHpToStrHp = true;

            // name
            if (getName().compare("autoname") == 0)
            {
                std::string name = getGameMap()->nextUniqueNameCreature(mDefinition->getClassName());
                setName(name);
            }
        }
    }

    if(getIsOnServerMap())
    {
        for(const CreatureSkill* skill : mDefinition->getCreatureSkills())
        {
            CreatureSkillData skillData(skill, skill->getCooldownNbTurns(), 0);
            mSkillData.push_back(skillData);
        }
    }

    buildStats();

    // Now, the max hp is known. If needed, we set it
    if(setHpToStrHp)
    {
        if(mHpString.compare("max") == 0)
            mHp = mMaxHP;
        else
            mHp = Helper::toDouble(mHpString);

        computeCreatureOverlayHealthValue();
    }
}

void Creature::fireCreatureSound(CreatureSound sound)
{
    Tile* posTile = getPositionTile();
    if(posTile == nullptr)
        return;

    std::string soundFamily;
    switch(sound)
    {
        case CreatureSound::Pickup:
            soundFamily = getDefinition()->getSoundFamilyPickup();
            break;
        case CreatureSound::Drop:
            soundFamily = getDefinition()->getSoundFamilyDrop();
            break;
        case CreatureSound::Attack:
            soundFamily = getDefinition()->getSoundFamilyAttack();
            break;
        case CreatureSound::Die:
            soundFamily = getDefinition()->getSoundFamilyDie();
            break;
        case CreatureSound::Slap:
            soundFamily = getDefinition()->getSoundFamilySlap();
            break;
        case CreatureSound::Dig:
            soundFamily = "Default/Dig";
            break;
        default:
            OD_LOG_ERR("Wrong CreatureSound value=" + Helper::toString(static_cast<uint32_t>(sound)));
            return;
    }

    getGameMap()->fireSpatialSound(mSeatsWithVisionNotified, SpatialSoundType::Creatures,
        soundFamily, posTile);
}

void Creature::itsPayDay()
{
    // Rogue creatures do not have to be paid
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

void Creature::decreaseWakefulness(double value)
{
    if(getSeat()->isRogueSeat())
        return;

    mWakefulness = std::max(0.0, mWakefulness - value);
}

void Creature::computeMood()
{
    mMoodPoints = CreatureMoodManager::computeCreatureMoodModifiers(*this);

    CreatureMoodLevel oldMoodValue = mMoodValue;
    mMoodValue = CreatureMoodManager::getCreatureMoodLevel(mMoodPoints);
    if(mMoodValue == oldMoodValue)
        return;

    if((mMoodValue >= CreatureMoodLevel::Furious) &&
       (oldMoodValue < CreatureMoodLevel::Furious))
    {
        // We became unhappy
        if((getSeat()->getPlayer() != nullptr) &&
           (getSeat()->getPlayer()->getIsHuman()))
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::chatServer, getSeat()->getPlayer());
            std::string msg = getName() + " is furious !";
            serverNotification->mPacket << msg << EventShortNoticeType::aboutCreatures;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
    else if((mMoodValue > CreatureMoodLevel::Neutral) &&
       (oldMoodValue <= CreatureMoodLevel::Neutral))
    {
        // We became unhappy
        if((getSeat()->getPlayer() != nullptr) &&
           (getSeat()->getPlayer()->getIsHuman()))
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::chatServer, getSeat()->getPlayer());
            std::string msg = getName() + " is unhappy !";
            serverNotification->mPacket << msg << EventShortNoticeType::aboutCreatures;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
}

void Creature::computeCreatureOverlayHealthValue()
{
    if(!getIsOnServerMap())
        return;

    uint32_t value = 0;
    double hp = getHP();
    // Note that we make a special case for hp = 0 to avoid errors due to roundness
    if(hp <= 0)
    {
        value = NB_OVERLAY_HEALTH_VALUES - 1;
    }
    else
    {
        uint32_t nbSteps = NB_OVERLAY_HEALTH_VALUES - 2;
        double healthStep = getMaxHp() / static_cast<double>(nbSteps);
        double tmpHealth = getMaxHp();
        for(value = 0; value < nbSteps; ++value)
        {
            if(hp >= tmpHealth)
                break;

            tmpHealth -= healthStep;
        }
    }

    if(mOverlayHealthValue != value)
    {
        mOverlayHealthValue = value;
        mNeedFireRefresh = true;
    }
}

void Creature::computeCreatureOverlayMoodValue()
{
    if(!getIsOnServerMap())
        return;

    uint32_t value = 0;
    // The creature mood applies only if the creature is alive
    if(isAlive())
    {
        if(mMoodValue == CreatureMoodLevel::Angry)
            value |= CreatureMoodEnum::Angry;
        else if(mMoodValue == CreatureMoodLevel::Furious)
            value |= CreatureMoodEnum::Furious;

        if(isActionInList(CreatureActionType::getFee))
            value |= CreatureMoodEnum::GetFee;

        if(isActionInList(CreatureActionType::leaveDungeon))
            value |= CreatureMoodEnum::LeaveDungeon;

        if(mKoTurnCounter < 0)
            value |= CreatureMoodEnum::KoDeath;
        else if(mKoTurnCounter > 0)
            value |= CreatureMoodEnum::KoTemp;

        if(isHungry())
            value |= CreatureMoodEnum::Hungry;

        if(isTired())
            value |= CreatureMoodEnum::Tired;

        if(mSeatPrison != nullptr)
            value |= CreatureMoodEnum::InJail;
    }

    if(mOverlayMoodValue != value)
    {
        mOverlayMoodValue = value;
        mNeedFireRefresh = true;
    }
}

void Creature::addCreatureEffect(CreatureEffect* effect)
{
    std::string effectName = nextParticleSystemsName();

    OD_LOG_INF("Added CreatureEffect name=" + effectName + " on creature=" + getName());

    CreatureParticuleEffect* particleEffect = new CreatureParticuleEffect(effectName, effect->getParticleEffectScript(),
        effect->getNbTurnsEffect(), effect);
    mEntityParticleEffects.push_back(particleEffect);

    mNeedFireRefresh = true;
}

bool Creature::isForcedToWork() const
{
    for(EntityParticleEffect* effect : mEntityParticleEffects)
    {
        if(effect->getEntityParticleEffectType() != EntityParticleEffectType::creature)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_LOG_ERR("Wrong effect on creature name=" + getName() + ", effectName=" + effect->mName + ", effectScript=" + effect->mScript);
            }
            continue;
        }

        CreatureParticuleEffect* creatureEffect = static_cast<CreatureParticuleEffect*>(effect);
        if(!creatureEffect->mEffect->isForcedToWork(*this))
            continue;

        return true;
    }
    return false;
}

bool Creature::isHurt() const
{
    //On server side, we test HP
    if(getIsOnServerMap())
        return getHP() < getMaxHp();

    // On client side, we test overlay value. 0 represents full health
    return mOverlayHealthValue > 0;
}

bool Creature::isKo() const
{
    if(getIsOnServerMap())
        return mKoTurnCounter != 0;

    // On client side, we test mood overlay value
    return (mOverlayMoodValue & KoDeathOrTemp) != 0;
}

bool Creature::isKoDeath() const
{
    if(getIsOnServerMap())
        return mKoTurnCounter < 0;

    // On client side, we test mood overlay value
    return (mOverlayMoodValue & CreatureMoodEnum::KoDeath) != 0;
}

bool Creature::isKoTemp() const
{
    if(getIsOnServerMap())
        return mKoTurnCounter > 0;

    // On client side, we test mood overlay value
    return (mOverlayMoodValue & CreatureMoodEnum::KoTemp) != 0;
}

bool Creature::isInPrison() const
{
    return mSeatPrison != nullptr;
}

void Creature::correctEntityMovePosition(Ogre::Vector3& position)
{
    static const double offset = 0.3;
    if(position.x > 0)
        position.x += Random::Double(-offset, offset);

    if(position.y > 0)
        position.y += Random::Double(-offset, offset);

    if(position.z > 0)
        position.z += Random::Double(-offset, offset);
}

void Creature::checkWalkPathValid()
{
    bool stop = false;
    for(const Ogre::Vector3& dest : mWalkQueue)
    {
        Tile* tile = getGameMap()->getTile(Helper::round(dest.x), Helper::round(dest.y));
        if(tile == nullptr)
        {
            stop = true;
            break;
        }

        if(!canGoThroughTile(tile))
        {
            stop = true;
            break;
        }
    }

    if(!stop)
        return;

    // There is an unpassable tile in our way. We stop what we are doing
    clearDestinations(EntityAnimation::idle_anim, true);
}

void Creature::setJobCooldown(int val)
{
    // If the creature has been slapped, its cooldown is decreased
    if(isForcedToWork())
        val = Helper::round(static_cast<float>(val) * 0.8f);

    mJobCooldown = val;
}

bool Creature::isTired() const
{
    if(getIsOnServerMap())
        return mWakefulness <= 20.0;

    return (mOverlayMoodValue & CreatureMoodEnum::Tired) != 0;
}

bool Creature::isHungry() const
{
    if(getIsOnServerMap())
        return mHunger >= 80.0;

    return (mOverlayMoodValue & CreatureMoodEnum::Hungry) != 0;
}

void Creature::releasedInBed()
{
    // The creature was released in its bed. Let's set its KO state to 0
    mKoTurnCounter = 0;
    computeCreatureOverlayMoodValue();
    // And sleep a bit
    sleep();
}

void Creature::setSeatPrison(Seat* seat)
{
    if(mSeatPrison == seat)
        return;

    // We reset KO to death counter
    if(seat != nullptr)
        mKoTurnCounter = 0;

    mSeatPrison = seat;
    mNeedFireRefresh = true;
}

bool Creature::isDangerous(const Creature* creature, int distance) const
{
    if(getDefinition()->isWorker())
        return false;

    return true;
}

void Creature::clientUpkeep()
{
    MovableGameEntity::clientUpkeep();
    if(mDropCooldown > 0)
        --mDropCooldown;
}

void Creature::setMoveSpeedModifier(double modifier)
{
    mSpeedModifier = modifier;

    mGroundSpeed = mDefinition->getMoveSpeedGround();
    mWaterSpeed = mDefinition->getMoveSpeedWater();
    mLavaSpeed  = mDefinition->getMoveSpeedLava();

    double multiplier = mLevel - 1;
    if (multiplier > 0.0)
    {
        mGroundSpeed += mDefinition->getGroundSpeedPerLevel() * multiplier;
        mWaterSpeed += mDefinition->getWaterSpeedPerLevel() * multiplier;
        mLavaSpeed += mDefinition->getLavaSpeedPerLevel() * multiplier;
    }

    mGroundSpeed *= mSpeedModifier;
    mWaterSpeed *= mSpeedModifier;
    mLavaSpeed *= mSpeedModifier;
    mNeedFireRefresh = true;
}

void Creature::clearMoveSpeedModifier()
{
    setMoveSpeedModifier(1.0);
}

void Creature::setDefenseModifier(double phy, double mag, double ele)
{
    mPhysicalDefense = mDefinition->getPhysicalDefense();
    mMagicalDefense = mDefinition->getMagicalDefense();
    mElementDefense = mDefinition->getElementDefense();

    mPhysicalDefense += phy;
    mMagicalDefense += mag;
    mElementDefense += ele;

    // Improve the stats to the current level
    double multiplier = mLevel - 1;
    if (multiplier <= 0.0)
        return;

    mPhysicalDefense += mDefinition->getPhysicalDefPerLevel() * multiplier;
    mMagicalDefense += mDefinition->getMagicalDefPerLevel() * multiplier;
    mElementDefense += mDefinition->getElementDefPerLevel() * multiplier;

    mNeedFireRefresh = true;
}

void Creature::clearDefenseModifier()
{
    setDefenseModifier(0.0, 0.0, 0.0);
}

void Creature::setStrengthModifier(double modifier)
{
    mModifierStrength = modifier;
    // Since strength is not used on client side, no need to send it
}

void Creature::clearStrengthModifier()
{
    setStrengthModifier(1.0);
}

std::vector<Tile*> Creature::getAccessibleVisibleTiles(Tile* center, int radius) const
{
    // If we are fighting in the arena, the accessible tiles are the arena
    if(!isActionInList(CreatureActionType::fightArena))
        return getGameMap()->visibleTiles(center->getX(), center->getY(), radius);

    // We check if we are in the arena
    Tile* myTile = getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + getName() + ", position=" + Helper::toString(getPosition()));
        return getGameMap()->visibleTiles(center->getX(), center->getY(), radius);
    }

    Room* room = myTile->getCoveringRoom();
    if(room == nullptr)
    {
        OD_LOG_ERR("name=" + getName() + ", not on an arena tile=" + Tile::displayAsString(myTile));
        return getGameMap()->visibleTiles(center->getX(), center->getY(), radius);
    }
    if(room->getType() != RoomType::arena)
    {
        OD_LOG_ERR("name=" + getName() + ", not on an arena tile=" + Tile::displayAsString(myTile) + ", room=" + room->getName());
        return getGameMap()->visibleTiles(center->getX(), center->getY(), radius);
    }

    float radiusSquared = radius * radius;
    std::vector<Tile*> allowedTiles;
    for(Tile* tile : room->getCoveredTiles())
    {
        float dist = Pathfinding::squaredDistanceTile(*center, *tile);
        if(dist > radiusSquared)
            continue;

        allowedTiles.push_back(tile);
    }

    return allowedTiles;
}

void Creature::fightInArena(Creature& opponent)
{
    pushAction(Utils::make_unique<CreatureActionFightArena>(*this, opponent));
}

bool Creature::isWarmup() const
{
    for(const CreatureSkillData& skillData : mSkillData)
    {
        if(skillData.mWarmup > 0)
            return true;
    }

    return false;
}

void Creature::fight()
{
    clearDestinations(EntityAnimation::idle_anim, true);
    clearActionQueue();
    pushAction(Utils::make_unique<CreatureActionFight>(*this, nullptr));
}

void Creature::fightCreature(Creature& creature)
{
    clearDestinations(EntityAnimation::idle_anim, true);
    clearActionQueue();
    pushAction(Utils::make_unique<CreatureActionWalkToTile>(*this));
}

void Creature::flee()
{
    clearDestinations(EntityAnimation::idle_anim, true);
    clearActionQueue();
    pushAction(Utils::make_unique<CreatureActionFlee>(*this));
}

void Creature::sleep()
{
    clearDestinations(EntityAnimation::idle_anim, true);
    clearActionQueue();
    pushAction(Utils::make_unique<CreatureActionSleep>(*this));
}

void Creature::leaveDungeon()
{
    OD_LOG_INF("creature=" + getName() + " wants to leave its dungeon");
    clearDestinations(EntityAnimation::idle_anim, true);
    clearActionQueue();
    pushAction(Utils::make_unique<CreatureActionLeaveDungeon>(*this));
}

void Creature::jobRoomAbsorbed(Room& newJobRoom)
{
    // If the job room is absorbed, we force the creatures working on the old room to search
    // a job. If there is space in the new one, they will use it. If not, they
    // will do something else
    clearDestinations(EntityAnimation::idle_anim, true);
    clearActionQueue();
    pushAction(Utils::make_unique<CreatureActionSearchJob>(*this, true));
}

void Creature::eatRoomAbsorbed(Room& newHatchery)
{
    // If the job room is absorbed, we force the creatures eating on the old room to use
    // the new one. If there is space in the new one, they will use it. If not, they
    // will do something else
    clearDestinations(EntityAnimation::idle_anim, true);
    clearActionQueue();
    pushAction(Utils::make_unique<CreatureActionSearchFood>(*this, true));
    pushAction(Utils::make_unique<CreatureActionUseHatchery>(*this, newHatchery, true));
}
