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

#include "entities/Tile.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "network/ODPacket.h"

#include "spell/SpellSummonWorker.h"
#include "spell/SpellCallToWar.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

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
    setIsOnMap(true);
    getGameMap()->addAnimatedObject(this);

    if(!getGameMap()->isServerGameMap())
        return;

    getGameMap()->addActiveObject(this);
}

void Spell::removeFromGameMap()
{
    getGameMap()->removeSpell(this);
    setIsOnMap(false);
    getGameMap()->removeAnimatedObject(this);

    if(!getGameMap()->isServerGameMap())
        return;

    fireRemoveEntityToSeatsWithVision();
    Tile* posTile = getPositionTile();
    if(posTile != nullptr)
        posTile->removeEntity(this);

    getGameMap()->removeActiveObject(this);
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

void Spell::notifySeatsWithVision(const std::vector<Seat*>& seats)
{
    // For spells, we want the caster and his allies to always have vision even if they
    // don't see the tile the spell is on. Of course, vision on the tile is not given by the spell
    // We notify seats that lost vision
    for(std::vector<Seat*>::iterator it = mSeatsWithVisionNotified.begin(); it != mSeatsWithVisionNotified.end();)
    {
        Seat* seat = *it;
        // If the seat is still in the list, nothing to do
        if(std::find(seats.begin(), seats.end(), seat) != seats.end())
        {
            ++it;
            continue;
        }

        // If the seat is the spell caster we don't remove vision
        if(getSeat() == seat)
        {
            ++it;
            continue;
        }

        // If the seat is a spell caster ally, we don't remove vision
        if(getSeat()->isAlliedSeat(seat))
        {
            ++it;
            continue;
        }

        // we remove vision
        it = mSeatsWithVisionNotified.erase(it);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireRemoveEntity(seat);
    }

    // We notify seats that gain vision
    for(Seat* seat : seats)
    {
        // If the seat was already in the list, nothing to do
        if(std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat) != mSeatsWithVisionNotified.end())
            continue;

        mSeatsWithVisionNotified.push_back(seat);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireAddEntity(seat, false);
    }

    // We give vision to the spell caster and his allies
    std::vector<Seat*> alliedSeats = getSeat()->getAlliedSeats();
    alliedSeats.push_back(getSeat());
    for(Seat* seat : alliedSeats)
    {
        // If the seat was already in the list, nothing to do
        if(std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat) != mSeatsWithVisionNotified.end())
            continue;

        mSeatsWithVisionNotified.push_back(seat);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireAddEntity(seat, false);
    }
}

std::string Spell::getSpellStreamFormat()
{
    return "typeSpell\t" + RenderedMovableEntity::getRenderedMovableEntityStreamFormat()
        + "optionalData\t";
}

void Spell::exportHeadersToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportHeadersToStream(os);
    os << getSpellType() << "\t";
}

void Spell::exportHeadersToPacket(ODPacket& os) const
{
    RenderedMovableEntity::exportHeadersToPacket(os);
    os << getSpellType();
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
