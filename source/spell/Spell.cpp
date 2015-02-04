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

#include "gamemap/GameMap.h"

#include "network/ODPacket.h"

#include "spell/SpellSummonWorker.h"
#include "spell/SpellCallToWar.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string Spell::SPELL_OGRE_PREFIX = "Spell_";

Spell::Spell(GameMap* gameMap, const std::string& baseName, const std::string& meshName, Ogre::Real rotationAngle,
        int32_t nbTurns, const std::string& initialAnimationState, bool initialAnimationLoop) :
    RenderedMovableEntity(gameMap, baseName, meshName, rotationAngle, false, 1.0f,
        initialAnimationState, initialAnimationLoop),
        mNbTurns(nbTurns)
{
}

void Spell::doUpkeep()
{
    if(mNbTurns < 0)
        return;

    if(mNbTurns > 0)
    {
        --mNbTurns;
        return;
    }

    removeFromGameMap();
    deleteYourself();
}

void Spell::addToGameMap()
{
    getGameMap()->addSpell(this);
}

void Spell::removeFromGameMap()
{
    getGameMap()->removeSpell(this);

}

Spell* Spell::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    SpellType type;
    is >> type;

    switch (type)
    {
        case SpellType::nullSpellType:
            return nullptr;
        case SpellType::summonWorker:
            OD_ASSERT_TRUE_MSG(false, "summonWorker should not be in packet");
            return nullptr;
        case SpellType::callToWar:
            return new SpellCallToWar(gameMap);
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(type)));
            return nullptr;
    }
}

Spell* Spell::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    SpellType type;
    is >> type;

    switch (type)
    {
        case SpellType::nullSpellType:
            return nullptr;
        case SpellType::summonWorker:
            OD_ASSERT_TRUE_MSG(false, "summonWorker should not be in packet");
            return nullptr;
        case SpellType::callToWar:
            return new SpellCallToWar(gameMap);
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(type)));
            return nullptr;
    }
}

std::string Spell::getSpellNameFromSpellType(SpellType type)
{
    switch (type)
    {
        case SpellType::nullSpellType:
            return "NullSpellType";

        case SpellType::summonWorker:
            return "SummonWorker";

        case SpellType::callToWar:
            return "callToWar";

        default:
            return "UnknownSpellType=" + Helper::toString(static_cast<int32_t>(type));
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

        case SpellType::callToWar:
            return SpellCallToWar::getSpellCallToWarCost(gameMap, tiles, player);

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

        case SpellType::callToWar:
        {
            SpellCallToWar::castSpellCallToWar(gameMap, tiles, player);
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
    RenderedMovableEntity::exportHeadersToStream(os);
    os << getSpellType() << "\t";
}

void Spell::exportHeadersToPacket(ODPacket& os)
{
    RenderedMovableEntity::exportHeadersToPacket(os);
    os << getSpellType();
}

void Spell::exportToPacket(ODPacket& os) const
{
    int seatId = -1;
    Seat* seat = getSeat();
    if(seat != nullptr)
        seatId = seat->getId();

    os << seatId;
    RenderedMovableEntity::exportToPacket(os);
}

void Spell::importFromPacket(ODPacket& is)
{
    int seatId;
    OD_ASSERT_TRUE_MSG(is >> seatId, "name=" + getName());
    if(seatId != -1)
    {
        Seat* seat = getGameMap()->getSeatById(seatId);
        OD_ASSERT_TRUE_MSG(seat != nullptr, "name=" + getName() + ", seatId=" + Helper::toString(seatId));
        if(seat != nullptr)
            setSeat(seat);
    }
    RenderedMovableEntity::importFromPacket(is);
}

void Spell::exportToStream(std::ostream& os) const
{
    int seatId = -1;
    Seat* seat = getSeat();
    if(seat != nullptr)
        seatId = seat->getId();

    os << seatId;
    RenderedMovableEntity::exportToStream(os);
}

void Spell::importFromStream(std::istream& is)
{
    int seatId;
    OD_ASSERT_TRUE_MSG(is >> seatId, "name=" + getName());
    if(seatId != -1)
    {
        Seat* seat = getGameMap()->getSeatById(seatId);
        OD_ASSERT_TRUE_MSG(seat != nullptr, "name=" + getName() + ", seatId=" + Helper::toString(seatId));
        if(seat != nullptr)
            setSeat(seat);
    }
    RenderedMovableEntity::importFromStream(is);
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
