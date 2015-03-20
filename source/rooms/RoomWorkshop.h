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

#ifndef ROOMWORKSHOP_H
#define ROOMWORKSHOP_H

#include "rooms/Room.h"

enum class TrapType;

class RoomWorkshop: public Room
{
public:
    RoomWorkshop(GameMap* gameMap);

    virtual RoomType getType() const override
    { return RoomType::workshop; }

    virtual void doUpkeep() override;
    virtual bool hasOpenCreatureSpot(Creature* c) override;
    virtual bool addCreatureUsingRoom(Creature* c) override;
    virtual void removeCreatureUsingRoom(Creature* c) override;
    virtual void absorbRoom(Room *r) override;
    virtual void addCoveredTile(Tile* t, double nHP) override;
    virtual bool removeCoveredTile(Tile* t) override;

    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;

protected:
    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override;
private:
    //!\brief checks if a tile is available in the workshop to place a new crafted trap
    uint32_t countCraftedItemsOnRoom();
    Tile* checkIfAvailableSpot(const std::vector<Tile*>& activeSpots);
    int32_t mPoints;
    TrapType mTrapType;
    void getCreatureWantedPos(Creature* creature, Tile* tileSpot,
        Ogre::Real& wantedX, Ogre::Real& wantedY);
    std::vector<Tile*> mUnusedSpots;
    std::vector<Tile*> mAllowedSpotsForCraftedItems;
    std::map<Creature*,Tile*> mCreaturesSpots;
};

#endif // ROOMWORKSHOP_H
