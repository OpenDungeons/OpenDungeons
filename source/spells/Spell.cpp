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

#include "spells/Spell.h"

#include "entities/Tile.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "network/ODPacket.h"

#include "spells/SpellSummonWorker.h"
#include "spells/SpellCallToWar.h"
#include "spells/SpellCreatureHeal.h"
#include "spells/SpellCreatureExplosion.h"
#include "spells/SpellManager.h"
#include "spells/SpellType.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

Spell::Spell(GameMap* gameMap, bool isOnServerMap, const std::string& baseName, const std::string& meshName, Ogre::Real rotationAngle,
        int32_t nbTurns) :
    RenderedMovableEntity(gameMap, isOnServerMap, baseName, meshName, rotationAngle, false, 1.0f),
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

    if(!getIsOnServerMap())
        return;

    getGameMap()->addActiveObject(this);
}

void Spell::removeFromGameMap()
{
    getGameMap()->removeSpell(this);
    setIsOnMap(false);
    getGameMap()->removeAnimatedObject(this);

    Tile* posTile = getPositionTile();
    if(posTile != nullptr)
        posTile->removeEntity(this);

    if(!getIsOnServerMap())
        return;

    fireRemoveEntityToSeatsWithVision();

    getGameMap()->removeActiveObject(this);
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
    std::string format = RenderedMovableEntity::getRenderedMovableEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "optionalData";

    return "typeSpell\t" + format;
}

std::string Spell::formatSpellPrice(SpellType type, uint32_t price)
{
    return SpellManager::getSpellNameFromSpellType(type) + " [" + Helper::toString(price)+ " Mana]";
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
