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

#include "network/ODPacket.h"
#include "rooms/Room.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

static CreatureRoomAffinity EMPTY_AFFINITY(RoomType::nullRoomType, 0, 0);

double CreatureDefinition::getXPNeededWhenLevel(unsigned int level) const
{
    // This should never happen
    OD_ASSERT_TRUE(level < MAX_LEVEL);
    OD_ASSERT_TRUE(level < mXPTable.size());
    OD_ASSERT_TRUE(level > 0);
    // Return 0.0, meaning there is an error.
    if (level >= MAX_LEVEL || level < 1 || level >= mXPTable.size())
        return 0.0;

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
    os << c->mBedMeshName << c->mBedDim1 << c->mBedDim2;
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
    os << c->mPhysicalAttack << c->mPhysicalAtkPerLevel;
    os << c->mMagicalAttack << c->mMagicalAtkPerLevel;
    os << c->mPhysicalDefense << c->mPhysicalDefPerLevel;
    os << c->mMagicalDefense << c->mMagicalDefPerLevel;
    os << c->mAttackRange << c->mAtkRangePerLevel;
    os << c->mAttackWarmupTime;
    os << c->mWeakCoef;
    os << c->mFeeBase;
    os << c->mFeePerLevel;
    os << c->mSleepHeal;
    os << c->mMoodModifierName;
    os << c->mWeaponSpawnL;
    os << c->mWeaponSpawnR;

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
    is >> c->mBedMeshName >> c->mBedDim1 >> c->mBedDim2;
    is >> c->mScale.x >> c->mScale.y >> c->mScale.z;
    is >> c->mMinHP >> c->mHpPerLevel >> c->mHpHealPerTurn;
    is >> c->mAwakenessLostPerTurn >> c->mHungerGrowthPerTurn;
    is >> c->mSightRadius;
    is >> c->mMaxGoldCarryable;
    is >> c->mDigRate >> c->mDigRatePerLevel;
    is >> c->mClaimRate >> c->mClaimRatePerLevel;
    is >> c->mMoveSpeedGround >> c->mMoveSpeedWater >> c->mMoveSpeedLava;
    is >> c->mGroundSpeedPerLevel >> c->mWaterSpeedPerLevel >> c->mLavaSpeedPerLevel;
    is >> c->mPhysicalAttack >> c->mPhysicalAtkPerLevel;
    is >> c->mMagicalAttack >> c->mMagicalAtkPerLevel;
    is >> c->mPhysicalDefense >> c->mPhysicalDefPerLevel;
    is >> c->mMagicalDefense >> c->mMagicalDefPerLevel;
    is >> c->mAttackRange >> c->mAtkRangePerLevel;
    is >> c->mAttackWarmupTime;
    is >> c->mWeakCoef;
    is >> c->mFeeBase;
    is >> c->mFeePerLevel;
    is >> c->mSleepHeal;
    is >> c->mMoodModifierName;
    is >> c->mWeaponSpawnL;
    is >> c->mWeaponSpawnR;

    for (unsigned int i = 0; i < c->mXPTable.size(); ++i)
    {
        double xpValue;
        is >> xpValue;
        c->mXPTable[i] = xpValue;
    }

    return is;
}

CreatureDefinition* CreatureDefinition::load(std::stringstream& defFile)
{
    if (!defFile.good())
        return nullptr;

    CreatureDefinition* creatureDef = new CreatureDefinition();
    if(!update(creatureDef, defFile))
    {
        delete creatureDef;
        creatureDef = nullptr;
    }

    return creatureDef;

}

bool CreatureDefinition::update(CreatureDefinition* creatureDef, std::stringstream& defFile)
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
            const CreatureDefinition* def = ConfigManager::getSingleton().getCreatureDefinition(baseDefinition);
            OD_ASSERT_TRUE_MSG(def != nullptr, "Couldn't find base class " + baseDefinition);
            if(def == nullptr)
                return false;

            *creatureDef = *def;
            continue;
        }

        if (nextParam == "[XP]")
        {
            loadXPTable(defFile, creatureDef);
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
            else if (nextParam == "BedDimX")
            {
                defFile >> nextParam;
                creatureDef->mBedDim1 = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "BedDimY")
            {
                defFile >> nextParam;
                creatureDef->mBedDim2 = Helper::toInt(nextParam);
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

            else if (nextParam == "PhysicalAttack")
            {
                defFile >> nextParam;
                creatureDef->mPhysicalAttack = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "PhysicalAtk/Level")
            {
                defFile >> nextParam;
                creatureDef->mPhysicalAtkPerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MagicalAttack")
            {
                defFile >> nextParam;
                creatureDef->mMagicalAttack = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MagicalAtk/Level")
            {
                defFile >> nextParam;
                creatureDef->mMagicalAtkPerLevel = Helper::toDouble(nextParam);
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
        }
    }

    if (name.empty())
    {
        OD_ASSERT_TRUE(false);
        return false;
    }
    creatureDef->mClassName = name;
    creatureDef->mBaseDefinition = baseDefinition;

    return true;
}

void CreatureDefinition::writeCreatureDefinitionDiff(const CreatureDefinition* def1, const CreatureDefinition* def2, std::ofstream& file)
{
    file << "[Creature]" << std::endl;
    file << "    Name\t" << def2->mClassName << std::endl;
    if(!def2->mBaseDefinition.empty())
    {
        // If there is a base definition, we take it as the reference no matter what def1 is because
        // we want to write only the differences between the reference and def2
        def1 = ConfigManager::getSingleton().getCreatureDefinition(def2->mBaseDefinition);
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

    if(def1 == nullptr || (def1->mBedDim1 != def2->mBedDim1))
        file << "    BedDimX\t" << def2->mBedDim1 << std::endl;

    if(def1 == nullptr || (def1->mBedDim2 != def2->mBedDim2))
        file << "    BedDimY\t" << def2->mBedDim2 << std::endl;

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

    if(def1 == nullptr || (def1->mPhysicalAttack != def2->mPhysicalAttack))
        file << "    PhysicalAttack\t" << def2->mPhysicalAttack << std::endl;

    if(def1 == nullptr || (def1->mPhysicalAtkPerLevel != def2->mPhysicalAtkPerLevel))
        file << "    PhysicalAtk/Level\t" << def2->mPhysicalAtkPerLevel << std::endl;

    if(def1 == nullptr || (def1->mMagicalAttack != def2->mMagicalAttack))
        file << "    MagicalAttack\t" << def2->mMagicalAttack << std::endl;

    if(def1 == nullptr || (def1->mMagicalAtkPerLevel != def2->mMagicalAtkPerLevel))
        file << "    MagicalAtk/Level\t" << def2->mMagicalAtkPerLevel << std::endl;

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

    if(def1 == nullptr || (def1->mWeaponSpawnL.compare(def2->mWeaponSpawnL) != 0))
        file << "    WeaponSpawnL\t" << def2->mWeaponSpawnL << std::endl;

    if(def1 == nullptr || (def1->mWeaponSpawnR.compare(def2->mWeaponSpawnR) != 0))
        file << "    WeaponSpawnL\t" << def2->mWeaponSpawnR << std::endl;

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
            file << "    " << Room::getRoomNameFromRoomType(roomAffinity.getRoomType());
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
    file << "[/Creature]" << std::endl;
}

void CreatureDefinition::loadXPTable(std::stringstream& defFile, CreatureDefinition* creatureDef)
{
    OD_ASSERT_TRUE(creatureDef != nullptr);
    if (creatureDef == nullptr)
        return;

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
        OD_ASSERT_TRUE(i < MAX_LEVEL - 1);
        if (i >= MAX_LEVEL - 1)
            continue;

        creatureDef->mXPTable[i++] = Helper::toDouble(nextParam);
    }
}

void CreatureDefinition::loadRoomAffinity(std::stringstream& defFile, CreatureDefinition* creatureDef)
{
    OD_ASSERT_TRUE(creatureDef != nullptr);
    if (creatureDef == nullptr)
        return;

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

        RoomType roomType = Room::getRoomTypeFromRoomName(roomName);
        OD_ASSERT_TRUE_MSG(roomType != RoomType::nullRoomType, "Unknown room name=" + roomName);
        if(roomType == RoomType::nullRoomType)
            continue;

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
