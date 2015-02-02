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

#ifndef ROOMFORGE_H
#define ROOMFORGE_H

#include "rooms/Room.h"

#include "traps/Trap.h"

class RoomForge: public Room
{
public:
    RoomForge(GameMap* gameMap);

    virtual RoomType getType() const
    { return RoomType::forge; }

    virtual void doUpkeep();
    virtual bool hasOpenCreatureSpot(Creature* c);
    virtual bool addCreatureUsingRoom(Creature* c);
    virtual void removeCreatureUsingRoom(Creature* c);
    virtual void absorbRoom(Room *r);
    virtual void addCoveredTile(Tile* t, double nHP);
    virtual bool removeCoveredTile(Tile* t);

protected:
    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile);
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile);
private:
    //!\brief checks if a tile is available in the forge to place a new crafted trap
    uint32_t countCraftedItemsOnRoom();
    Tile* checkIfAvailableSpot(const std::vector<Tile*>& activeSpots);
    int32_t getNbCraftedTrapsForType(Trap::TrapType type);
    int32_t mNbTurnsNoChangeSpots;
    int32_t mPoints;
    Trap::TrapType mTrapType;
    void getCreatureWantedPos(Creature* creature, Tile* tileSpot,
        Ogre::Real& wantedX, Ogre::Real& wantedY);
    std::vector<Tile*> mUnusedSpots;
    std::vector<Tile*> mAllowedSpotsForCraftedItems;
    std::map<Creature*,Tile*> mCreaturesSpots;
};

#endif // ROOMFORGE_H
