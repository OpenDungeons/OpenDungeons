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

//TODO: ideally we shouldn't need this file. Reasons:
// - The stream operators should be replaced by a proper XML reader class that creates
// the Definition objects through its ctor (that's what ctors are for).
// - The name strings should ideally be read from a file, too (XML? Script? but not hardcoded,
// who knows what creature types will there be in the future - shouldn't need recompiling
// for plain content additions/adjustments or translations).
// - CreatureDefintion is ... a plain Defintion. That's what header files are for.

#include "CreatureDefinition.h"

#include "Helper.h"

CreatureDefinition::CreatureJob CreatureDefinition::creatureJobFromString(const std::string& s)
{
    if (s.compare("BasicWorker") == 0)
        return basicWorker;
    else if (s.compare("AdvancedWorker") == 0)
        return advancedWorker;
    else if (s.compare("Scout") == 0)
        return scout;
    else if (s.compare("WeakFighter") == 0)
        return weakFighter;
    else if (s.compare("WeakSpellcaster") == 0)
        return weakSpellcaster;
    else if (s.compare("WeakBuilder") == 0)
        return weakBuilder;
    else if (s.compare("StrongFighter") == 0)
        return strongFighter;
    else if (s.compare("StrongSpellcaster") == 0)
        return strongSpellcaster;
    else if (s.compare("StrongBuilder") == 0)
        return strongBuilder;
    else if (s.compare("Guard") == 0)
        return guard;
    else if (s.compare("SpecialCreature") == 0)
        return specialCreature;
    else if (s.compare("Summon") == 0)
        return summon;
    else if (s.compare("SuperCreature") == 0)
        return superCreature;
    else
        return nullCreatureJob;
}

std::string CreatureDefinition::creatureJobToString(CreatureJob c)
{
    switch (c)
    {
    case nullCreatureJob:
        return "NullCreatureJob";

    case basicWorker:
        return "BasicWorker";

    case advancedWorker:
        return "AdvancedWorker";

    case scout:
        return "Scout";

    case weakFighter:
        return "WeakFighter";

    case weakSpellcaster:
        return "WeakSpellcaster";

    case weakBuilder:
        return "WeakBuilder";

    case strongFighter:
        return "StrongFighter";

    case strongSpellcaster:
        return "StrongSpellcaster";

    case strongBuilder:
        return "StrongBuilder";

    case guard:
        return "Guard";

    case specialCreature:
        return "SpecialCreature";

    case summon:
        return "Summon";

    case superCreature:
        return "SuperCreature";

    default:
        return "NullCreatureJob";
    }
}

std::ostream& operator<<(std::ostream& os, CreatureDefinition* c)
{
    //TODO: Need to include maxHP/maxMana in the file format.
    os << c->mClassName << "\t"
       << CreatureDefinition::creatureJobToString(c->mCreatureJob)
       << "\t" << c->mMeshName << "\t";
    os << c->mBedMeshName << "\t" << c->mBedDim1 << "\t" << c->mBedDim2 << "\t";
    os << c->mScale.x << "\t" << c->mScale.y << "\t" << c->mScale.z << "\t";
    os << c->mHpPerLevel << "\t" << c->mManaPerLevel << "\t";
    os << c->mMaxHP << "\t" << c->mMaxMana << "\t";
    os << c->mSightRadius << "\t" << c->mDigRate << "\t" << c->mDanceRate << "\t"
       << c->mMoveSpeed << "\t";
    os << Tile::tilePassabilityToString(c->mTilePassability);
    return os;
}

std::istream& operator>>(std::istream& is, CreatureDefinition* c)
{
    std::string tempString;
    is >> c->mClassName >> tempString;
    c->mCreatureJob = CreatureDefinition::creatureJobFromString(tempString);
    is >> c->mMeshName;
    is >> c->mBedMeshName >> c->mBedDim1 >> c->mBedDim2;
    is >> c->mScale.x >> c->mScale.y >> c->mScale.z;
    is >> c->mHpPerLevel >> c->mManaPerLevel >> c->mMaxHP >> c->mMaxMana;
    is >> c->mSightRadius >> c->mDigRate >> c->mDanceRate >> c->mMoveSpeed;
    is >> tempString;
    c->mTilePassability = Tile::tilePassabilityFromString(tempString);

    return is;
}

ODPacket& operator<<(ODPacket& os, CreatureDefinition* c)
{
    //TODO: Need to include maxHP/maxMana in the file format.
    std::string creatureJob = CreatureDefinition::creatureJobToString(c->mCreatureJob);
    std::string tilePassability = Tile::tilePassabilityToString(c->mTilePassability);
    os << c->mClassName
       << creatureJob
       << c->mMeshName;
    os << c->mBedMeshName << c->mBedDim1 << c->mBedDim2;
    os << c->mScale.x << c->mScale.y << c->mScale.z;
    os << c->mHpPerLevel << c->mManaPerLevel;
    os << c->mMaxHP << c->mMaxMana;
    os << c->mSightRadius << c->mDigRate << c->mDanceRate
       << c->mMoveSpeed;
    os << tilePassability;
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
    is >> c->mHpPerLevel >> c->mManaPerLevel >> c->mMaxHP >> c->mMaxMana;
    is >> c->mSightRadius >> c->mDigRate >> c->mDanceRate >> c->mMoveSpeed;
    is >> tempString;
    c->mTilePassability = Tile::tilePassabilityFromString(tempString);

    return is;
}

void CreatureDefinition::loadFromLine(const std::string& line, CreatureDefinition* c)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    c->mClassName = elems[0];
    c->mCreatureJob = CreatureDefinition::creatureJobFromString(elems[1]);
    c->mMeshName = elems[2];

    c->mBedMeshName = elems[3];
    c->mBedDim1 = Helper::toInt(elems[4]);
    c->mBedDim2 = Helper::toInt(elems[5]);

    c->mScale.x = Helper::toDouble(elems[6]);
    c->mScale.y = Helper::toDouble(elems[7]);
    c->mScale.z = Helper::toDouble(elems[8]);

    c->mHpPerLevel = Helper::toDouble(elems[9]);
    c->mManaPerLevel = Helper::toDouble(elems[10]);
    c->mMaxHP = Helper::toDouble(elems[11]);
    c->mMaxMana = Helper::toDouble(elems[12]);

    c->mSightRadius = Helper::toDouble(elems[13]);
    c->mDigRate = Helper::toDouble(elems[14]);
    c->mDanceRate = Helper::toDouble(elems[15]);
    c->mMoveSpeed = Helper::toDouble(elems[16]);
    c->mTilePassability = Tile::tilePassabilityFromString(elems[17]);
}
