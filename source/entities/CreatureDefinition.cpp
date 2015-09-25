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

#include "entities/CreatureDefinition.h"

#include "creatureskill/CreatureSkill.h"
#include "network/ODPacket.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

static CreatureRoomAffinity EMPTY_AFFINITY(RoomType::nullRoomType, 0, 0);

CreatureDefinition::CreatureDefinition(
            const std::string&      className,
            CreatureJob             job,
            const std::string&      meshName,
            const Ogre::Vector3&    scale,
            const std::string&      bedMeshName,
            int                     bedDim1,
            int                     bedDim2,
            int                     bedPosX,
            int                     bedPosY,
            double                  bedOrientX,
            double                  bedOrientY,
            double                  sightRadius,
            int                     maxGoldCarryable,
            double                  digRate,
            double                  digRatePerLevel,
            double                  claimRate,
            double                  claimRatePerLevel,
            double                  minHP,
            double                  hpPerLevel,
            double                  hpHealPerTurn,
            double                  awakenessLostPerTurn,
            double                  hungerGrowthPerTurn,
            double                  moveSpeedGround,
            double                  moveSpeedWater,
            double                  moveSpeedLava,
            double                  groundSpeedPerLevel,
            double                  waterSpeedPerLevel,
            double                  lavaSpeedPerLevel,
            double                  phyAtkMel,
            double                  phyAtkMelPerLvl,
            double                  magAtkMel,
            double                  magAtkMelPerLvl,
            double                  physicalDefense,
            double                  physicalDefPerLevel,
            double                  magicalDefense,
            double                  magicalDefPerLevel,
            double                  attackRange,
            double                  atkRangePerLevel,
            double                  phyAtkRan,
            double                  phyAtkRanPerLvl,
            double                  magAtkRan,
            double                  magAtkRanPerLvl,
            const std::string&      ranAtkMesh,
            const std::string&      ranAtkPartScript,
            double                  attackWarmupTime,
            double                  weakCoef,
            int32_t                 feeBase,
            int32_t                 feePerLevel,
            int32_t                 sleepHeal,
            int32_t                 turnsStunDropped) :
        mCreatureJob (job),
        mClassName   (className),
        mMeshName    (meshName),
        mBedMeshName (bedMeshName),
        mBedDim1     (bedDim1),
        mBedDim2     (bedDim2),
        mBedPosX     (bedPosX),
        mBedPosY     (bedPosY),
        mBedOrientX  (bedOrientX),
        mBedOrientY  (bedOrientY),
        mScale       (scale),
        mSightRadius (sightRadius),
        mMaxGoldCarryable (maxGoldCarryable),
        mDigRate     (digRate),
        mDigRatePerLevel (digRatePerLevel),
        mClaimRate   (claimRate),
        mClaimRatePerLevel(claimRatePerLevel),
        mMinHP       (minHP),
        mHpPerLevel  (hpPerLevel),
        mHpHealPerTurn      (hpHealPerTurn),
        mAwakenessLostPerTurn(awakenessLostPerTurn),
        mHungerGrowthPerTurn(hungerGrowthPerTurn),
        mMoveSpeedGround    (moveSpeedGround),
        mMoveSpeedWater     (moveSpeedWater),
        mMoveSpeedLava      (moveSpeedLava),
        mGroundSpeedPerLevel(groundSpeedPerLevel),
        mWaterSpeedPerLevel (waterSpeedPerLevel),
        mLavaSpeedPerLevel  (lavaSpeedPerLevel),
        mPhyAtkMel          (phyAtkMel),
        mPhyAtkMelPerLvl    (phyAtkMelPerLvl),
        mMagAtkMel          (magAtkMel),
        mMagAtkMelPerLvl    (magAtkMelPerLvl),
        mPhysicalDefense    (physicalDefense),
        mPhysicalDefPerLevel(physicalDefPerLevel),
        mMagicalDefense     (magicalDefense),
        mMagicalDefPerLevel (magicalDefPerLevel),
        mAttackRange        (attackRange),
        mAtkRangePerLevel   (atkRangePerLevel),
        mPhyAtkRan          (phyAtkRan),
        mPhyAtkRanPerLvl    (phyAtkRanPerLvl),
        mMagAtkRan          (magAtkRan),
        mMagAtkRanPerLvl    (magAtkRanPerLvl),
        mRanAtkMesh         (ranAtkMesh),
        mRanAtkPartScript   (ranAtkPartScript),
        mAttackWarmupTime   (attackWarmupTime),
        mWeakCoef           (weakCoef),
        mFeeBase            (feeBase),
        mFeePerLevel        (feePerLevel),
        mSleepHeal          (sleepHeal),
        mTurnsStunDropped   (turnsStunDropped),
        mWeaponSpawnL       ("none"),
        mWeaponSpawnR       ("none"),
        mSoundFamilyPickup  ("Default/Pickup"),
        mSoundFamilyDrop    ("Default/Drop"),
        mSoundFamilyAttack  ("Default/Attack"),
        mSoundFamilyDie     ("Default/Die"),
        mSoundFamilySlap    ("Default/Slap")
{
    mXPTable.assign(MAX_LEVEL - 1, 100.0);
}

CreatureDefinition::CreatureDefinition(const CreatureDefinition& def) :
        mCreatureJob(def.mCreatureJob),
        mClassName(def.mClassName),
        mMeshName(def.mMeshName),
        mBedMeshName(def.mBedMeshName),
        mBedDim1(def.mBedDim1),
        mBedDim2(def.mBedDim2),
        mBedPosX(def.mBedPosX),
        mBedPosY(def.mBedPosY),
        mBedOrientX(def.mBedOrientX),
        mBedOrientY(def.mBedOrientY),
        mScale(def.mScale),
        mSightRadius(def.mSightRadius),
        mMaxGoldCarryable(def.mMaxGoldCarryable),
        mDigRate(def.mDigRate),
        mDigRatePerLevel(def.mDigRatePerLevel),
        mClaimRate(def.mClaimRate),
        mClaimRatePerLevel(def.mClaimRatePerLevel),
        mMinHP(def.mMinHP),
        mHpPerLevel(def.mHpPerLevel),
        mHpHealPerTurn(def.mHpHealPerTurn),
        mAwakenessLostPerTurn(def.mAwakenessLostPerTurn),
        mHungerGrowthPerTurn(def.mHungerGrowthPerTurn),
        mMoveSpeedGround(def.mMoveSpeedGround),
        mMoveSpeedWater(def.mMoveSpeedWater),
        mMoveSpeedLava(def.mMoveSpeedLava),
        mGroundSpeedPerLevel(def.mGroundSpeedPerLevel),
        mWaterSpeedPerLevel(def.mWaterSpeedPerLevel),
        mLavaSpeedPerLevel(def.mLavaSpeedPerLevel),
        mPhyAtkMel(def.mPhyAtkMel),
        mPhyAtkMelPerLvl(def.mPhyAtkMelPerLvl),
        mMagAtkMel(def.mMagAtkMel),
        mMagAtkMelPerLvl(def.mMagAtkMelPerLvl),
        mPhysicalDefense(def.mPhysicalDefense),
        mPhysicalDefPerLevel(def.mPhysicalDefPerLevel),
        mMagicalDefense(def.mMagicalDefense),
        mMagicalDefPerLevel(def.mMagicalDefPerLevel),
        mAttackRange(def.mAttackRange),
        mAtkRangePerLevel(def.mAtkRangePerLevel),
        mPhyAtkRan(def.mPhyAtkRan),
        mPhyAtkRanPerLvl(def.mPhyAtkRanPerLvl),
        mMagAtkRan(def.mMagAtkRan),
        mMagAtkRanPerLvl(def.mMagAtkRanPerLvl),
        mRanAtkMesh(def.mRanAtkMesh),
        mRanAtkPartScript(def.mRanAtkPartScript),
        mAttackWarmupTime(def.mAttackWarmupTime),
        mWeakCoef(def.mWeakCoef),
        mFeeBase(def.mFeeBase),
        mFeePerLevel(def.mFeePerLevel),
        mSleepHeal(def.mSleepHeal),
        mTurnsStunDropped(def.mTurnsStunDropped),
        mWeaponSpawnL(def.mWeaponSpawnL),
        mWeaponSpawnR(def.mWeaponSpawnR),
        mSoundFamilyPickup(def.mSoundFamilyPickup),
        mSoundFamilyDrop(def.mSoundFamilyDrop),
        mSoundFamilyAttack(def.mSoundFamilyAttack),
        mSoundFamilyDie(def.mSoundFamilyDie),
        mSoundFamilySlap(def.mSoundFamilySlap)
{
    for(const double& xp : def.mXPTable)
    {
        mXPTable.push_back(xp);
    }
    for(const CreatureRoomAffinity& aff : def.mRoomAffinity)
    {
        mRoomAffinity.push_back(aff);
    }
    for(const CreatureSkill* skill : def.mCreatureSkills)
    {
        mCreatureSkills.push_back(skill->clone());
    }
}

CreatureDefinition::~CreatureDefinition()
{
    for(const CreatureSkill* skill : mCreatureSkills)
    {
        delete skill;
    }
    mCreatureSkills.clear();
}

double CreatureDefinition::getXPNeededWhenLevel(unsigned int level) const
{
    // Return 0.0, meaning there is an error.
    if (level >= MAX_LEVEL || level < 1 || level >= mXPTable.size())
    {
        // This should never happen
        OD_LOG_ERR("level=" + Helper::toString(level));
        return 0.0;
    }

    return mXPTable[level - 1];
}

CreatureDefinition::CreatureJob CreatureDefinition::creatureJobFromString(const std::string& s)
{
    if (s.compare("Worker") == 0)
        return Worker;

    // Use Fighter as a default value
    return Fighter;
}

std::string CreatureDefinition::creatureJobToString(CreatureJob c)
{
    switch (c)
    {
    case Worker:
        return "Worker";

    default:
    case Fighter:
        return "Fighter";
    }
}

int32_t CreatureDefinition::getFee(unsigned int level) const
{
    return mFeeBase + (mFeePerLevel * level);
}

ODPacket& operator<<(ODPacket& os, const CreatureDefinition* c)
{
    std::string creatureJob = CreatureDefinition::creatureJobToString(c->mCreatureJob);
    os << c->mClassName
       << creatureJob
       << c->mMeshName;
    os << c->mBedMeshName << c->mBedDim1 << c->mBedDim2 << c->mBedPosX << c->mBedPosY << c->mBedOrientX << c->mBedOrientY;
    os << c->mScale.x << c->mScale.y << c->mScale.z;
    os << c->mMinHP;
    os << c->mHpPerLevel;
    os << c->mHpHealPerTurn;
    os << c->mAwakenessLostPerTurn;
    os << c->mHungerGrowthPerTurn;
    os << c->mSightRadius;
    os << c->mMaxGoldCarryable;
    os << c->mDigRate << c->mDigRatePerLevel;
    os << c->mClaimRate << c->mClaimRatePerLevel;
    os << c->mMoveSpeedGround << c->mMoveSpeedWater << c->mMoveSpeedLava;
    os << c->mGroundSpeedPerLevel << c->mWaterSpeedPerLevel << c->mLavaSpeedPerLevel;
    os << c->mPhyAtkMel << c->mPhyAtkMelPerLvl;
    os << c->mMagAtkMel << c->mMagAtkMelPerLvl;
    os << c->mPhysicalDefense << c->mPhysicalDefPerLevel;
    os << c->mMagicalDefense << c->mMagicalDefPerLevel;
    os << c->mAttackRange << c->mAtkRangePerLevel;
    os << c->mPhyAtkRan << c->mPhyAtkRanPerLvl;
    os << c->mMagAtkRan << c->mMagAtkRanPerLvl;
    os << c->mRanAtkMesh << c->mRanAtkPartScript;
    os << c->mAttackWarmupTime;
    os << c->mWeakCoef;
    os << c->mFeeBase;
    os << c->mFeePerLevel;
    os << c->mSleepHeal;
    os << c->mTurnsStunDropped;
    os << c->mMoodModifierName;
    os << c->mWeaponSpawnL;
    os << c->mWeaponSpawnR;
    os << c->mSoundFamilyPickup;
    os << c->mSoundFamilyDrop;
    os << c->mSoundFamilyAttack;
    os << c->mSoundFamilyDie;
    os << c->mSoundFamilySlap;

    for (unsigned int i = 0; i < c->mXPTable.size(); ++i)
        os << c->mXPTable[i];

    return os;
}

ODPacket& operator>>(ODPacket& is, CreatureDefinition* c)
{
    std::string tempString;
    is >> c->mClassName >> tempString;
    c->mCreatureJob = CreatureDefinition::creatureJobFromString(tempString);
    is >> c->mMeshName;
    is >> c->mBedMeshName >> c->mBedDim1 >> c->mBedDim2 >>c->mBedPosX >> c->mBedPosY >> c->mBedOrientX >> c->mBedOrientY;
    is >> c->mScale.x >> c->mScale.y >> c->mScale.z;
    is >> c->mMinHP >> c->mHpPerLevel >> c->mHpHealPerTurn;
    is >> c->mAwakenessLostPerTurn >> c->mHungerGrowthPerTurn;
    is >> c->mSightRadius;
    is >> c->mMaxGoldCarryable;
    is >> c->mDigRate >> c->mDigRatePerLevel;
    is >> c->mClaimRate >> c->mClaimRatePerLevel;
    is >> c->mMoveSpeedGround >> c->mMoveSpeedWater >> c->mMoveSpeedLava;
    is >> c->mGroundSpeedPerLevel >> c->mWaterSpeedPerLevel >> c->mLavaSpeedPerLevel;
    is >> c->mPhyAtkMel >> c->mPhyAtkMelPerLvl;
    is >> c->mMagAtkMel >> c->mMagAtkMelPerLvl;
    is >> c->mPhysicalDefense >> c->mPhysicalDefPerLevel;
    is >> c->mMagicalDefense >> c->mMagicalDefPerLevel;
    is >> c->mAttackRange >> c->mAtkRangePerLevel;
    is >> c->mPhyAtkRan >> c->mPhyAtkRanPerLvl;
    is >> c->mMagAtkRan >> c->mMagAtkRanPerLvl;
    is >> c->mRanAtkMesh >> c->mRanAtkPartScript;
    is >> c->mAttackWarmupTime;
    is >> c->mWeakCoef;
    is >> c->mFeeBase;
    is >> c->mFeePerLevel;
    is >> c->mSleepHeal;
    is >> c->mTurnsStunDropped;
    is >> c->mMoodModifierName;
    is >> c->mWeaponSpawnL;
    is >> c->mWeaponSpawnR;
    is >> c->mSoundFamilyPickup;
    is >> c->mSoundFamilyDrop;
    is >> c->mSoundFamilyAttack;
    is >> c->mSoundFamilyDie;
    is >> c->mSoundFamilySlap;

    for (unsigned int i = 0; i < c->mXPTable.size(); ++i)
    {
        double xpValue;
        is >> xpValue;
        c->mXPTable[i] = xpValue;
    }

    return is;
}

CreatureDefinition* CreatureDefinition::load(std::stringstream& defFile, const std::map<std::string, CreatureDefinition*>& defMap)
{
    if (!defFile.good())
        return nullptr;

    CreatureDefinition* creatureDef = new CreatureDefinition();
    if(!update(creatureDef, defFile, defMap))
    {
        delete creatureDef;
        creatureDef = nullptr;
    }

    return creatureDef;

}

bool CreatureDefinition::update(CreatureDefinition* creatureDef, std::stringstream& defFile, const std::map<std::string, CreatureDefinition*>& defMap)
{
    std::string nextParam;
    bool exit = false;
    // Parameters that should not be overriden if a Creature definition is extended. They will be set after
    // the class is copied if there is a base class
    std::string name = creatureDef->mClassName;
    std::string baseDefinition;
    while (defFile.good())
    {
        if (exit)
            break;

        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/Creature]" || nextParam == "[/CreatureDefinitions]")
        {
            exit = true;
            break;
        }

        if (nextParam == "Name")
        {
            defFile >> name;
            continue;
        }

        if (nextParam == "BaseDefinition")
        {
            defFile >> baseDefinition;
            auto it = defMap.find(baseDefinition);
            if(it == defMap.end())
            {
                OD_LOG_ERR("Couldn't find base class " + baseDefinition);
                return false;
            }
            *creatureDef = *(it->second);
            continue;
        }

        if (nextParam == "[XP]")
        {
            loadXPTable(defFile, creatureDef);
            continue;
        }

        if (nextParam == "[CreatureSkills]")
        {
            loadCreatureSkills(defFile, creatureDef);
            continue;
        }

        if (nextParam == "[RoomAffinity]")
        {
            loadRoomAffinity(defFile, creatureDef);
            continue;
        }

        if (nextParam != "[Stats]")
            continue;

        while (defFile.good())
        {
            if (exit)
                break;

            if(!(defFile >> nextParam))
                break;

            if (nextParam == "[/Stats]")
                break;

            // Handle ill-formed files.
            if (nextParam == "[/Creature]" || nextParam == "[/CreatureDefinitions]")
            {
                exit = true;
                break;
            }

            if (nextParam == "CreatureJob")
            {
                defFile >> nextParam;
                creatureDef->mCreatureJob = CreatureDefinition::creatureJobFromString(nextParam);
                continue;
            }
            else if (nextParam == "MeshName")
            {
                defFile >> nextParam;
                creatureDef->mMeshName = nextParam;
                continue;
            }
            else if (nextParam == "MeshScaleX")
            {
                defFile >> nextParam;
                creatureDef->mScale.x = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MeshScaleY")
            {
                defFile >> nextParam;
                creatureDef->mScale.y = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MeshScaleZ")
            {
                defFile >> nextParam;
                creatureDef->mScale.z = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "BedMeshName")
            {
                defFile >> nextParam;
                creatureDef->mBedMeshName = nextParam;
                continue;
            }
            else if (nextParam == "BedDim")
            {
                defFile >> nextParam;
                creatureDef->mBedDim1 = Helper::toInt(nextParam);
                defFile >> nextParam;
                creatureDef->mBedDim2 = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "BedSleepPos")
            {
                defFile >> nextParam;
                creatureDef->mBedPosX = Helper::toInt(nextParam);
                defFile >> nextParam;
                creatureDef->mBedPosY = Helper::toInt(nextParam);
                defFile >> nextParam;
                creatureDef->mBedOrientX = Helper::toDouble(nextParam);
                defFile >> nextParam;
                creatureDef->mBedOrientY = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MinHP")
            {
                defFile >> nextParam;
                creatureDef->mMinHP = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "HP/Level")
            {
                defFile >> nextParam;
                creatureDef->mHpPerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "Heal/Turn")
            {
                defFile >> nextParam;
                creatureDef->mHpHealPerTurn = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "AwakenessLost/Turn")
            {
                defFile >> nextParam;
                creatureDef->mAwakenessLostPerTurn = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "HungerGrowth/Turn")
            {
                defFile >> nextParam;
                creatureDef->mHungerGrowthPerTurn = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "TileSightRadius")
            {
                defFile >> nextParam;
                creatureDef->mSightRadius = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "MaxGoldCarryable")
            {
                defFile >> nextParam;
                creatureDef->mMaxGoldCarryable = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "DigRate")
            {
                defFile >> nextParam;
                creatureDef->mDigRate = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "DigRate/Level")
            {
                defFile >> nextParam;
                creatureDef->mDigRatePerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "ClaimRate")
            {
                defFile >> nextParam;
                creatureDef->mClaimRate = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "ClaimRate/Level")
            {
                defFile >> nextParam;
                creatureDef->mClaimRatePerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "GroundMoveSpeed")
            {
                defFile >> nextParam;
                creatureDef->mMoveSpeedGround = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "WaterMoveSpeed")
            {
                defFile >> nextParam;
                creatureDef->mMoveSpeedWater = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "LavaMoveSpeed")
            {
                defFile >> nextParam;
                creatureDef->mMoveSpeedLava = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "GroundSpeed/Level")
            {
                defFile >> nextParam;
                creatureDef->mGroundSpeedPerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "WaterSpeed/Level")
            {
                defFile >> nextParam;
                creatureDef->mWaterSpeedPerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "LavaSpeed/Level")
            {
                defFile >> nextParam;
                creatureDef->mLavaSpeedPerLevel = Helper::toDouble(nextParam);
                continue;
            }

            else if (nextParam == "PhyAtkMel")
            {
                defFile >> nextParam;
                creatureDef->mPhyAtkMel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "PhyAtkMel/Level")
            {
                defFile >> nextParam;
                creatureDef->mPhyAtkMelPerLvl = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MagAtkMel")
            {
                defFile >> nextParam;
                creatureDef->mMagAtkMel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MagAtkMel/Level")
            {
                defFile >> nextParam;
                creatureDef->mMagAtkMelPerLvl = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "PhyAtkRan")
            {
                defFile >> nextParam;
                creatureDef->mPhyAtkRan = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "PhyAtkRan/Level")
            {
                defFile >> nextParam;
                creatureDef->mPhyAtkRanPerLvl = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MagAtkRan")
            {
                defFile >> nextParam;
                creatureDef->mMagAtkRan = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MagAtkRan/Level")
            {
                defFile >> nextParam;
                creatureDef->mMagAtkRanPerLvl = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "RangeMesh")
            {
                defFile >> nextParam;
                if(nextParam != "none")
                    creatureDef->mRanAtkMesh = nextParam;
                defFile >> nextParam;
                if(nextParam != "none")
                    creatureDef->mRanAtkPartScript = nextParam;
                continue;
            }
            else if (nextParam == "PhysicalDefense")
            {
                defFile >> nextParam;
                creatureDef->mPhysicalDefense = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "PhysicalDef/Level")
            {
                defFile >> nextParam;
                creatureDef->mPhysicalDefPerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MagicalDefense")
            {
                defFile >> nextParam;
                creatureDef->mMagicalDefense = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MagicalDef/Level")
            {
                defFile >> nextParam;
                creatureDef->mMagicalDefPerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "AttackRange")
            {
                defFile >> nextParam;
                creatureDef->mAttackRange = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "AtkRange/Level")
            {
                defFile >> nextParam;
                creatureDef->mAtkRangePerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "AttackWarmupTime")
            {
                defFile >> nextParam;
                creatureDef->mAttackWarmupTime = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "WeakCoef")
            {
                defFile >> nextParam;
                creatureDef->mWeakCoef = Helper::toDouble(nextParam);
                OD_ASSERT_TRUE_MSG(creatureDef->mWeakCoef >= 0.0 && creatureDef->mWeakCoef <= 1.0, "mWeakCoef=" + Helper::toString(creatureDef->mWeakCoef));
                continue;
            }
            else if (nextParam == "FeeBase")
            {
                defFile >> nextParam;
                creatureDef->mFeeBase = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "FeePerLevel")
            {
                defFile >> nextParam;
                creatureDef->mFeePerLevel = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "SleepHeal")
            {
                defFile >> nextParam;
                creatureDef->mSleepHeal = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "TurnsStunDropped")
            {
                defFile >> nextParam;
                creatureDef->mTurnsStunDropped = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "CreatureMoodName")
            {
                defFile >> nextParam;
                creatureDef->mMoodModifierName = nextParam;
                continue;
            }
            else if (nextParam == "WeaponSpawnL")
            {
                defFile >> creatureDef->mWeaponSpawnL;
                continue;
            }
            else if (nextParam == "WeaponSpawnR")
            {
                defFile >> creatureDef->mWeaponSpawnR;
                continue;
            }
            else if (nextParam == "SoundFamilyPickup")
            {
                defFile >> creatureDef->mSoundFamilyPickup;
                continue;
            }
            else if (nextParam == "moundFamilyDrop")
            {
                defFile >> creatureDef->mSoundFamilyDrop;
                continue;
            }
            else if (nextParam == "SoundFamilyAttack")
            {
                defFile >> creatureDef->mSoundFamilyAttack;
                continue;
            }
            else if (nextParam == "SoundFamilyDie")
            {
                defFile >> creatureDef->mSoundFamilyDie;
                continue;
            }
            else if (nextParam == "SoundFamilySlap")
            {
                defFile >> creatureDef->mSoundFamilySlap;
                continue;
            }
        }
    }

    if (name.empty())
    {
        OD_LOG_ERR("Cannot have empty creature def name");
        return false;
    }
    creatureDef->mClassName = name;
    creatureDef->mBaseDefinition = baseDefinition;

    return true;
}

void CreatureDefinition::writeCreatureDefinitionDiff(
    const CreatureDefinition* def1, const CreatureDefinition* def2,
    std::ostream& file, const std::map<std::string, CreatureDefinition*>& defMap)
{
    file << "[Creature]" << std::endl;
    file << "    Name\t" << def2->mClassName << std::endl;
    if(!def2->mBaseDefinition.empty())
    {
        // If there is a base definition, we take it as the reference no matter what def1 is because
        // we want to write only the differences between the reference and def2
        auto it = defMap.find(def2->mBaseDefinition);
        if(it != defMap.end())
        {
            def1 = it->second;
        }
        OD_ASSERT_TRUE_MSG(def1 != nullptr, "BaseDefinition=" + def2->mBaseDefinition);
        file << "    BaseDefinition\t" << def2->mBaseDefinition << std::endl;
    }
    file << "    [Stats]" << std::endl;

    if(def1 == nullptr || (def1->mCreatureJob != def2->mCreatureJob))
        file << "    CreatureJob\t" << creatureJobToString(def2->mCreatureJob) << std::endl;

    if(def1 == nullptr || (def1->mMeshName.compare(def2->mMeshName) != 0))
        file << "    MeshName\t" << def2->mMeshName << std::endl;

    if(def1 == nullptr || (def1->mScale.x != def2->mScale.x))
        file << "    MeshScaleX\t" << def2->mScale.x << std::endl;

    if(def1 == nullptr || (def1->mScale.y != def2->mScale.y))
        file << "    MeshScaleY\t" << def2->mScale.y << std::endl;

    if(def1 == nullptr || (def1->mScale.z != def2->mScale.z))
        file << "    MeshScaleZ\t" << def2->mScale.z << std::endl;

    if(def1 == nullptr || (def1->mBedMeshName.compare(def2->mBedMeshName) != 0))
        file << "    BedMeshName\t" << def2->mBedMeshName << std::endl;

    if(def1 == nullptr || (def1->mBedDim1 != def2->mBedDim1) || (def1->mBedDim2 != def2->mBedDim2))
        file << "    BedDim\t" << def2->mBedDim1 << "\t" << def2->mBedDim2 << std::endl;

    if(def1 == nullptr || (def1->mBedPosX != def2->mBedPosX) || (def1->mBedPosY != def2->mBedPosY) || (def1->mBedOrientX != def2->mBedOrientX) || (def1->mBedOrientY != def2->mBedOrientY))
        file << "    BedPos\t" << def2->mBedPosX << "\t" << def2->mBedPosY << "\t" << def2->mBedOrientX << "\t" << def2->mBedOrientY << std::endl;

    if(def1 == nullptr || (def1->mMinHP != def2->mMinHP))
        file << "    MinHP\t" << def2->mMinHP << std::endl;

    if(def1 == nullptr || (def1->mHpPerLevel != def2->mHpPerLevel))
        file << "    HP/Level\t" << def2->mHpPerLevel << std::endl;

    if(def1 == nullptr || (def1->mHpHealPerTurn != def2->mHpHealPerTurn))
        file << "    Heal/Turn\t" << def2->mHpHealPerTurn << std::endl;

    if(def1 == nullptr || (def1->mAwakenessLostPerTurn != def2->mAwakenessLostPerTurn))
        file << "    AwakenessLost/Turn\t" << def2->mAwakenessLostPerTurn << std::endl;

    if(def1 == nullptr || (def1->mHungerGrowthPerTurn != def2->mHungerGrowthPerTurn))
        file << "    HungerGrowth/Turn\t" << def2->mHungerGrowthPerTurn << std::endl;

    if(def1 == nullptr || (def1->mSightRadius != def2->mSightRadius))
        file << "    TileSightRadius\t" << def2->mSightRadius << std::endl;

    if(def1 == nullptr || (def1->mMaxGoldCarryable != def2->mMaxGoldCarryable))
        file << "    MaxGoldCarryable\t" << def2->mMaxGoldCarryable << std::endl;

    if(def1 == nullptr || (def1->mDigRate != def2->mDigRate))
        file << "    DigRate\t" << def2->mDigRate << std::endl;

    if(def1 == nullptr || (def1->mDigRatePerLevel != def2->mDigRatePerLevel))
        file << "    DigRate/Level\t" << def2->mDigRatePerLevel << std::endl;

    if(def1 == nullptr || (def1->mClaimRate != def2->mClaimRate))
        file << "    ClaimRate\t" << def2->mClaimRate << std::endl;

    if(def1 == nullptr || (def1->mClaimRatePerLevel != def2->mClaimRatePerLevel))
        file << "    ClaimRate/Level\t" << def2->mClaimRatePerLevel << std::endl;

    if(def1 == nullptr || (def1->mMoveSpeedGround != def2->mMoveSpeedGround))
        file << "    GroundMoveSpeed\t" << def2->mMoveSpeedGround << std::endl;

    if(def1 == nullptr || (def1->mMoveSpeedWater != def2->mMoveSpeedWater))
        file << "    WaterMoveSpeed\t" << def2->mMoveSpeedWater << std::endl;

    if(def1 == nullptr || (def1->mMoveSpeedLava != def2->mMoveSpeedLava))
        file << "    LavaMoveSpeed\t" << def2->mMoveSpeedLava << std::endl;

    if(def1 == nullptr || (def1->mGroundSpeedPerLevel != def2->mGroundSpeedPerLevel))
        file << "    GroundSpeed/Level\t" << def2->mGroundSpeedPerLevel << std::endl;

    if(def1 == nullptr || (def1->mWaterSpeedPerLevel != def2->mWaterSpeedPerLevel))
        file << "    WaterSpeed/Level\t" << def2->mWaterSpeedPerLevel << std::endl;

    if(def1 == nullptr || (def1->mLavaSpeedPerLevel != def2->mLavaSpeedPerLevel))
        file << "    LavaSpeed/Level\t" << def2->mLavaSpeedPerLevel << std::endl;

    if(def1 == nullptr || (def1->mPhyAtkMel != def2->mPhyAtkMel))
        file << "    PhyAtkMel\t" << def2->mPhyAtkMel << std::endl;

    if(def1 == nullptr || (def1->mPhyAtkMelPerLvl != def2->mPhyAtkMelPerLvl))
        file << "    PhyAtkMel/Level\t" << def2->mPhyAtkMelPerLvl << std::endl;

    if(def1 == nullptr || (def1->mMagAtkMel != def2->mMagAtkMel))
        file << "    MagAtkMel\t" << def2->mMagAtkMel << std::endl;

    if(def1 == nullptr || (def1->mMagAtkMelPerLvl != def2->mMagAtkMelPerLvl))
        file << "    MagAtkMel/Level\t" << def2->mMagAtkMelPerLvl << std::endl;

    if(def1 == nullptr || (def1->mPhysicalDefense != def2->mPhysicalDefense))
        file << "    PhysicalDefense\t" << def2->mPhysicalDefense << std::endl;

    if(def1 == nullptr || (def1->mPhysicalDefPerLevel != def2->mPhysicalDefPerLevel))
        file << "    PhysicalDef/Level\t" << def2->mPhysicalDefPerLevel << std::endl;

    if(def1 == nullptr || (def1->mMagicalDefense != def2->mMagicalDefense))
        file << "    MagicalDefense\t" << def2->mMagicalDefense << std::endl;

    if(def1 == nullptr || (def1->mMagicalDefPerLevel != def2->mMagicalDefPerLevel))
        file << "    MagicalDef/Level\t" << def2->mMagicalDefPerLevel << std::endl;

    if(def1 == nullptr || (def1->mAttackRange != def2->mAttackRange))
        file << "    AttackRange\t" << def2->mAttackRange << std::endl;

    if(def1 == nullptr || (def1->mAtkRangePerLevel != def2->mAtkRangePerLevel))
        file << "    AtkRange/Level\t" << def2->mAtkRangePerLevel << std::endl;

    if(def1 == nullptr || (def1->mPhyAtkRan != def2->mPhyAtkRan))
        file << "    PhyAtkRan\t" << def2->mPhyAtkRan << std::endl;

    if(def1 == nullptr || (def1->mPhyAtkRanPerLvl != def2->mPhyAtkRanPerLvl))
        file << "    PhyAtkRan/Level\t" << def2->mPhyAtkRanPerLvl << std::endl;

    if(def1 == nullptr || (def1->mMagAtkRan != def2->mMagAtkRan))
        file << "    MagAtkRan\t" << def2->mMagAtkRan << std::endl;

    if(def1 == nullptr || (def1->mMagAtkRanPerLvl != def2->mMagAtkRanPerLvl))
        file << "    MagAtkRan/Level\t" << def2->mMagAtkRanPerLvl << std::endl;

    if(def1 == nullptr || (def1->mRanAtkMesh != def2->mRanAtkMesh) || (def1->mRanAtkPartScript != def2->mRanAtkPartScript))
    {
        std::string mesh;
        std::string script;
        if(def2->mRanAtkMesh.empty())
            mesh = "none";
        else
            mesh = def2->mRanAtkMesh;

        if(def2->mRanAtkPartScript.empty())
            script = "none";
        else
            script = def2->mRanAtkPartScript;

        file << "    RangeMesh\t" << mesh << "\t" << script << std::endl;
    }

    if(def1 == nullptr || (def1->mAttackWarmupTime != def2->mAttackWarmupTime))
        file << "    AttackWarmupTime\t" << def2->mAttackWarmupTime << std::endl;

    if(def1 == nullptr || (def1->mWeakCoef != def2->mWeakCoef))
        file << "    WeakCoef\t" << def2->mWeakCoef << std::endl;

    if(def1 == nullptr || (def1->mFeeBase != def2->mFeeBase))
        file << "    FeeBase\t" << def2->mFeeBase << std::endl;

    if(def1 == nullptr || (def1->mFeePerLevel != def2->mFeePerLevel))
        file << "    FeePerLevel\t" << def2->mFeePerLevel << std::endl;

    if(def1 == nullptr || (def1->mSleepHeal != def2->mSleepHeal))
        file << "    SleepHeal\t" << def2->mSleepHeal << std::endl;

    if(def1 == nullptr || (def1->mTurnsStunDropped != def2->mTurnsStunDropped))
        file << "    TurnsStunDropped\t" << def2->mTurnsStunDropped << std::endl;

    if(!def2->mMoodModifierName.empty() && (def1 == nullptr || (def1->mMoodModifierName != def2->mMoodModifierName)))
        file << "    CreatureMoodName\t" << def2->mMoodModifierName << std::endl;

    if(def1 == nullptr || (def1->mWeaponSpawnL.compare(def2->mWeaponSpawnL) != 0))
        file << "    WeaponSpawnL\t" << def2->mWeaponSpawnL << std::endl;

    if(def1 == nullptr || (def1->mWeaponSpawnR.compare(def2->mWeaponSpawnR) != 0))
        file << "    WeaponSpawnL\t" << def2->mWeaponSpawnR << std::endl;

    if(def1 == nullptr || (def1->mSoundFamilyPickup.compare(def2->mSoundFamilyPickup) != 0))
        file << "    SoundFamilyPickup\t" << def2->mSoundFamilyPickup << std::endl;

    if(def1 == nullptr || (def1->mSoundFamilyDrop.compare(def2->mSoundFamilyDrop) != 0))
        file << "    SoundFamilyDrop\t" << def2->mSoundFamilyDrop << std::endl;

    if(def1 == nullptr || (def1->mSoundFamilyAttack.compare(def2->mSoundFamilyAttack) != 0))
        file << "    SoundFamilyAttack\t" << def2->mSoundFamilyAttack << std::endl;

    if(def1 == nullptr || (def1->mSoundFamilyDie.compare(def2->mSoundFamilyDie) != 0))
        file << "    SoundFamilyDie\t" << def2->mSoundFamilyDie << std::endl;

    if(def1 == nullptr || (def1->mSoundFamilySlap.compare(def2->mSoundFamilySlap) != 0))
        file << "    SoundFamilySlap\t" << def2->mSoundFamilySlap << std::endl;

    file << "    [/Stats]" << std::endl;

    bool isSame;

    isSame = (def1 != nullptr && def1->mRoomAffinity.size() == def2->mRoomAffinity.size());
    uint32_t index = 0;
    while(isSame &&
          index < def2->mRoomAffinity.size())
    {
        isSame = def1->mRoomAffinity[index] == def2->mRoomAffinity[index];
        ++index;
    }
    if(!isSame)
    {
        file << "    [RoomAffinity]" << std::endl;
        for(const CreatureRoomAffinity& roomAffinity : def2->mRoomAffinity)
        {
            file << "    " << RoomManager::getRoomNameFromRoomType(roomAffinity.getRoomType());
            file << "\t" << roomAffinity.getLikeness();
            file << "\t" << roomAffinity.getEfficiency();
            file << std::endl;
        }
        file << "    [/RoomAffinity]" << std::endl;
    }

    isSame = true;
    for(uint32_t i = 0; i < (MAX_LEVEL - 1); ++i)
    {
        if(def1 == nullptr || (def1->mXPTable[i] != def2->mXPTable[i]))
        {
            isSame = false;
            break;
        }
    }
    if(!isSame)
    {
        file << "    [XP]" << std::endl;
        uint32_t i = 2;
        uint32_t levelMax;
        while(i <= MAX_LEVEL)
        {
            levelMax = std::min(i + 10U - (i % 10), MAX_LEVEL);
            file << "    # " << i <<"-" << levelMax << std::endl;
            file << "    ";

            while(i < levelMax)
            {
                file << def2->mXPTable[i - 2] << "\t";
                ++i;
            }
            file << def2->mXPTable[i - 2] << std::endl;
            ++i;
        }
        file << "    [/XP]" << std::endl;
    }

    isSame = (def1 != nullptr && (def1->mCreatureSkills.size() == def2->mCreatureSkills.size()));
    if(isSame)
    {
        for(uint32_t i = 0; i < def1->mCreatureSkills.size(); ++i)
        {
            if(!def1->mCreatureSkills[i]->isEqual(*(def2->mCreatureSkills[i])))
            {
                isSame = false;
                break;
            }
        }
    }
    if(!isSame && !def2->mCreatureSkills.empty())
    {
        file << "    [CreatureSkills]" << std::endl;
        for(const CreatureSkill* skill : def2->mCreatureSkills)
        {
            std::string format;
            skill->getFormatString(format);
            file << "    " << format << std::endl;
            file << "    ";
            CreatureSkill::write(skill, file);
        }
        file << "    [/CreatureSkills]" << std::endl;
    }
    file << "[/Creature]" << std::endl;
}

void CreatureDefinition::loadXPTable(std::stringstream& defFile, CreatureDefinition* creatureDef)
{
    if (creatureDef == nullptr)
    {
        OD_LOG_ERR("Cannot load null creature def");
        return;
    }

    std::string nextParam;
    bool exit = false;

    // The XP index
    unsigned int i = 0;

    while (defFile.good())
    {
        if (exit)
            break;

        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/XP]" || nextParam == "[/Stats]" ||
            nextParam == "[/Creature]" || nextParam == "[/Creatures]")
        {
            exit = true;
            break;
        }

        // Ignore values after the max level
        if (i >= MAX_LEVEL - 1)
        {
            OD_LOG_ERR("creatureDef=" + creatureDef->getClassName() + ", i=" + Helper::toString(i < MAX_LEVEL - 1));
            continue;
        }

        creatureDef->mXPTable[i++] = Helper::toDouble(nextParam);
    }
}

void CreatureDefinition::loadCreatureSkills(std::stringstream& defFile, CreatureDefinition* creatureDef)
{
    if (creatureDef == nullptr)
    {
        OD_LOG_ERR("Cannot load null creature def");
        return;
    }

    if(!defFile.good())
    {
        OD_LOG_ERR("input file invalid");
        return;
    }

    std::string nextParam;
    // We want to start on the next line
    std::getline(defFile, nextParam);
    while (defFile.good())
    {
        std::getline(defFile, nextParam);
        if(!defFile.good())
            break;

        Helper::trim(nextParam);
        if (nextParam == "[/CreatureSkills]" || nextParam == "[/Stats]" ||
            nextParam == "[/Creature]" || nextParam == "[/Creatures]")
        {
            break;
        }

        // Ignore values after the max level
        std::stringstream ss(nextParam);
        CreatureSkill* skill = CreatureSkill::load(ss);
        if (skill == nullptr)
        {
            OD_LOG_ERR("line=" + nextParam);
            continue;
        }

        creatureDef->mCreatureSkills.push_back(skill);
    }
}

void CreatureDefinition::loadRoomAffinity(std::stringstream& defFile, CreatureDefinition* creatureDef)
{
    OD_ASSERT_TRUE(creatureDef != nullptr);
    if (creatureDef == nullptr)
    {
        OD_LOG_ERR("Cannot load room afinity for null creatureDef");
        return;
    }

    std::string nextParam;
    bool exit = false;

    creatureDef->mRoomAffinity.clear();
    while (defFile.good())
    {
        if (exit)
            break;

        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/RoomAffinity]" ||
            nextParam == "[/Creature]" || nextParam == "[/Creatures]")
        {
            exit = true;
            break;
        }

        std::string roomName = nextParam;

        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/RoomAffinity]" ||
            nextParam == "[/Creature]" || nextParam == "[/Creatures]")
        {
            exit = true;
            break;
        }
        int32_t likeness = Helper::toInt(nextParam);

        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/RoomAffinity]" ||
            nextParam == "[/Creature]" || nextParam == "[/Creatures]")
        {
            exit = true;
            break;
        }
        double efficiency = Helper::toDouble(nextParam);

        RoomType roomType = RoomManager::getRoomTypeFromRoomName(roomName);
        if(roomType == RoomType::nullRoomType)
        {
            OD_LOG_ERR("Unknown room name=" + roomName);
            continue;
        }

        // We sort the CreatureRoomAffinity from the most liked to the less
        std::vector<CreatureRoomAffinity>::iterator it = creatureDef->mRoomAffinity.begin();
        while(it != creatureDef->mRoomAffinity.end())
        {
            CreatureRoomAffinity& roomAffinity = *it;
            if(roomAffinity.getLikeness() <= likeness)
                break;

            ++it;
        }
        creatureDef->mRoomAffinity.insert(it, CreatureRoomAffinity(roomType, likeness, efficiency));
    }
}

const CreatureRoomAffinity& CreatureDefinition::getRoomAffinity(RoomType roomType) const
{
    for(const CreatureRoomAffinity& roomAffinity : mRoomAffinity)
    {
        if(roomAffinity.getRoomType() != roomType)
            continue;

        return roomAffinity;
    }

    return EMPTY_AFFINITY;
}
