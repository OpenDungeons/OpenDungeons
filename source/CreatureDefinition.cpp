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

#include "CreatureDefinition.h"

#include "Helper.h"

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

ODPacket& operator<<(ODPacket& os, CreatureDefinition* c)
{
    std::string creatureJob = CreatureDefinition::creatureJobToString(c->mCreatureJob);
    os << c->mClassName
       << creatureJob
       << c->mMeshName;
    os << c->mBedMeshName << c->mBedDim1 << c->mBedDim2;
    os << c->mScale.x << c->mScale.y << c->mScale.z;
    os << c->mHpPerLevel;
    os << c->mMaxHP;
    os << c->mSightRadius << c->mDigRate << c->mDanceRate
       << c->mMoveSpeedGround << c->mMoveSpeedWater << c->mMoveSpeedLava;
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
    is >> c->mHpPerLevel >> c->mMaxHP;
    is >> c->mSightRadius >> c->mDigRate >> c->mDanceRate;
    is >> c->mMoveSpeedGround >> c->mMoveSpeedWater >> c->mMoveSpeedLava;

    return is;
}

bool CreatureDefinition::load(std::stringstream& defFile, CreatureDefinition* c)
{
    if (!defFile.good())
        return false;

    bool enoughInfo = false;
    std::string nextParam;

    while (defFile.good())
    {
        defFile >> nextParam;
        if (nextParam == "[/Creature]")
            return enoughInfo;
        if (nextParam == "[/Creatures]")
            return enoughInfo;

        if (nextParam == "Name")
        {
            defFile >> nextParam;
            c->mClassName = nextParam;
            enoughInfo = true;
            continue;
        }

        if (nextParam != "[Stats]")
            continue;

        while (defFile.good())
        {
            defFile >> nextParam;
            if (nextParam == "[/Stats]")
                break;

            // Handle ill-formed files.
            if (nextParam == "[/Creature]")
                return enoughInfo;
            if (nextParam == "[/Creatures]")
                return enoughInfo;

            if (nextParam == "CreatureJob")
            {
                defFile >> nextParam;
                c->mCreatureJob = CreatureDefinition::creatureJobFromString(nextParam);
                continue;
            }
            else if (nextParam == "MeshName")
            {
                defFile >> nextParam;
                c->mMeshName = nextParam;
                continue;
            }
            else if (nextParam == "MeshScaleX")
            {
                defFile >> nextParam;
                c->mScale.x = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MeshScaleY")
            {
                defFile >> nextParam;
                c->mScale.y = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MeshScaleZ")
            {
                defFile >> nextParam;
                c->mScale.z = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "BedMeshName")
            {
                defFile >> nextParam;
                c->mBedMeshName = nextParam;
                continue;
            }
            else if (nextParam == "BedDimX")
            {
                defFile >> nextParam;
                c->mBedDim1 = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "BedDimY")
            {
                defFile >> nextParam;
                c->mBedDim2 = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "HP/Level")
            {
                defFile >> nextParam;
                c->mHpPerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MaxHP")
            {
                defFile >> nextParam;
                c->mMaxHP = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "TileSightRadius")
            {
                defFile >> nextParam;
                c->mSightRadius = Helper::toInt(nextParam); // Turn to int?
                continue;
            }
            else if (nextParam == "DigRate")
            {
                defFile >> nextParam;
                c->mDigRate = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "ClaimRate")
            {
                defFile >> nextParam;
                c->mDanceRate = Helper::toDouble(nextParam); // Rename member to mClaimRate
                continue;
            }
            else if (nextParam == "GroundMoveSpeed")
            {
                defFile >> nextParam;
                c->mMoveSpeedGround = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "WaterMoveSpeed")
            {
                defFile >> nextParam;
                c->mMoveSpeedWater = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "LavaMoveSpeed")
            {
                defFile >> nextParam;
                c->mMoveSpeedLava = Helper::toDouble(nextParam);
                continue;
            }
        }
    }

    return enoughInfo;
}
