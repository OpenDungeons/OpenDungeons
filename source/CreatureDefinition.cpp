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
#include "ODPacket.h"

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
    os << c->mSightRadius << c->mDigRate << c->mClaimRate
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
    is >> c->mSightRadius >> c->mDigRate >> c->mClaimRate;
    is >> c->mMoveSpeedGround >> c->mMoveSpeedWater >> c->mMoveSpeedLava;

    return is;
}

CreatureDefinition* CreatureDefinition::load(std::stringstream& defFile)
{
    if (!defFile.good())
        return nullptr;

    CreatureDefinition* creatureDef = new CreatureDefinition();
    std::string nextParam;
    bool exit = false;
    bool enoughInfo = false;

    while (defFile.good())
    {
        if (exit)
            break;

        defFile >> nextParam;
        if (nextParam == "[/Creature]" || nextParam == "[/Creatures]")
        {
            exit = true;
            break;
        }

        if (nextParam == "Name")
        {
            defFile >> nextParam;
            creatureDef->mClassName = nextParam;
            enoughInfo = true;
            continue;
        }

        if (nextParam != "[Stats]")
            continue;

        while (defFile.good())
        {
            if (exit)
                break;

            defFile >> nextParam;
            if (nextParam == "[/Stats]")
                break;

            // Handle ill-formed files.
            if (nextParam == "[/Creature]" || nextParam == "[/Creatures]")
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
            else if (nextParam == "HP/Level")
            {
                defFile >> nextParam;
                creatureDef->mHpPerLevel = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "MaxHP")
            {
                defFile >> nextParam;
                creatureDef->mMaxHP = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "TileSightRadius")
            {
                defFile >> nextParam;
                creatureDef->mSightRadius = Helper::toInt(nextParam);
                continue;
            }
            else if (nextParam == "DigRate")
            {
                defFile >> nextParam;
                creatureDef->mDigRate = Helper::toDouble(nextParam);
                continue;
            }
            else if (nextParam == "ClaimRate")
            {
                defFile >> nextParam;
                creatureDef->mClaimRate = Helper::toDouble(nextParam);
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
        }
    }

    if (!enoughInfo)
    {
        delete creatureDef;
        creatureDef = nullptr;
    }

    return creatureDef;
}
