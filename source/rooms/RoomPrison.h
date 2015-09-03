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
    { return RoomType::prison; }

    void absorbRoom(Room *r) override;

    void doUpkeep() override;

    uint32_t countPrisoners();

    //! \brief Function called each turn for each prisoner in the jail
    void actionPrisoner(Creature* creature);

    bool hasCarryEntitySpot(GameEntity* carriedEntity) override;
    Tile* askSpotForCarriedEntity(GameEntity* carriedEntity) override;
    void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity) override;

    static void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet);
    static void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoomEditor(GameMap* gameMap, ODPacket& packet);
    static bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles);
    static Room* getRoomFromStream(GameMap* gameMap, std::istream& is);

protected:
    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
private:
    std::vector<Creature*> mPendingPrisoners;
};

#endif // ROOMPRISON_H
