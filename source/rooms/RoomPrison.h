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

#ifndef ROOMPRISON_H
#define ROOMPRISON_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

class InputCommand;
class InputManager;

class RoomPrison: public Room
{
public:
    RoomPrison(GameMap* gameMap);

    virtual RoomType getType() const override
    { return mRoomType; }

    void absorbRoom(Room *r) override;

    void doUpkeep() override;

    bool hasOpenCreatureSpot(Creature* creature) override;
    bool addCreatureUsingRoom(Creature* creature) override;
    void removeCreatureUsingRoom(Creature* creature) override;

    uint32_t countPrisoners();

    bool hasCarryEntitySpot(GameEntity* carriedEntity) override;
    Tile* askSpotForCarriedEntity(GameEntity* carriedEntity) override;
    void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity) override;

    bool isInContainment(Creature& creature) override;

    bool useRoom(Creature& creature, bool forced) override;

    void creatureDropped(Creature& creature) override;

    void restoreInitialEntityState() override;

    static const RoomType mRoomType;

protected:
    virtual BuildingObject* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
    void exportToStream(std::ostream& os) const override;
    bool importFromStream(std::istream& is) override;

private:
    std::vector<Creature*> mPendingPrisoners;
    //! \brief Used at map loading to save creatures in prison
    std::vector<std::string> mPrisonersLoad;
};

#endif // ROOMPRISON_H
