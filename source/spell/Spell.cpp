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

#include "spell/Spell.h"

#include "game/Player.h"

#include "network/ODPacket.h"

#include "spell/SpellSummonWorker.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

Spell::Spell(GameMap* gameMap, const std::string& meshName, Ogre::Real rotationAngle) :
    RenderedMovableEntity(gameMap, "Spell", meshName,
        rotationAngle, false, 1.0f)
{
}

Spell* Spell::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    Spell* tempSpell = nullptr;
    SpellType type;
    is >> type;

    switch (type)
    {
        case SpellType::nullSpellType:
            tempSpell = nullptr;
            break;
        case SpellType::summonWorker:
            OD_ASSERT_TRUE_MSG(false, "summonWorker should not be in stream");
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Helper::toString(
                static_cast<int>(type)));
    }

    if(tempSpell == nullptr)
        return nullptr;

    tempSpell->importFromStream(is);

    return tempSpell;
}

Spell* Spell::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    Spell* tempSpell = nullptr;
    SpellType type;
    is >> type;

    switch (type)
    {
        case SpellType::nullSpellType:
            tempSpell = nullptr;
            break;
        case SpellType::summonWorker:
            OD_ASSERT_TRUE_MSG(false, "summonWorker should not be in packet");
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(type)));
    }

    if(tempSpell == nullptr)
        return nullptr;

    tempSpell->importFromPacket(is);

    return tempSpell;
}

const char* Spell::getSpellNameFromSpellType(SpellType type)
{
    switch (type)
    {
        case SpellType::nullSpellType:
            return "NullSpellType";

        case SpellType::summonWorker:
            return "SummonWorker";

        default:
            return "UnknownSpellType";
    }
}

int Spell::getSpellCost(GameMap* gameMap, SpellType type, const std::vector<Tile*>& tiles, Player* player)
{
    switch (type)
    {
        case SpellType::nullSpellType:
            return 0;

        case SpellType::summonWorker:
            return SpellSummonWorker::getSpellSummonWorkerCost(gameMap, tiles, player);

        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(type)));
            return 0;
    }
}

void Spell::castSpell(GameMap* gameMap, SpellType type, const std::vector<Tile*>& tiles, Player* player)
{
    switch (type)
    {
        case SpellType::nullSpellType:
        {
            OD_ASSERT_TRUE_MSG(false, "Wrong spell casted by " + player->getNick());
            break;
        }

        case SpellType::summonWorker:
        {
            SpellSummonWorker::castSpellSummonWorker(gameMap, tiles, player);
            break;
        }

        default:
        {
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(type)) + " for player " + player->getNick());
            break;
        }
    }
}

std::string Spell::getFormat()
{
    return "typeSpell\tseatId\tnumTiles\t\tSubsequent Lines: tileX\ttileY\tisActivated(0/1)\t\tSubsequent Lines: optional specific data";
}

void Spell::exportHeadersToStream(std::ostream& os)
{
    os << getSpellType() << "\t";
}

void Spell::exportHeadersToPacket(ODPacket& os)
{
    os << getSpellType();
}

void Spell::exportToPacket(ODPacket& os) const
{
}

void Spell::importFromPacket(ODPacket& is)
{
}

void Spell::exportToStream(std::ostream& os) const
{
}

void Spell::importFromStream(std::istream& is)
{
}

std::istream& operator>>(std::istream& is, SpellType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<SpellType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const SpellType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}

ODPacket& operator>>(ODPacket& is, SpellType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<SpellType>(tmp);
    return is;
}

ODPacket& operator<<(ODPacket& os, const SpellType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}
