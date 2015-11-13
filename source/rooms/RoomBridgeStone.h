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

#ifndef ROOMBRIDGESTONE_H
#define ROOMBRIDGESTONE_H

#include "rooms/RoomBridge.h"
#include "rooms/RoomType.h"

class Tile;

class RoomBridgeStone: public RoomBridge
{
public:
    RoomBridgeStone(GameMap* gameMap);

    virtual RoomType getType() const override
    { return mRoomType; }

    static bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles);

    static const RoomType mRoomType;

protected:
    void updateFloodFillPathCreated(Seat* seat, const std::vector<Tile*>& tiles) override;
    void updateFloodFillTileRemoved(Seat* seat, Tile* tile) override;
};

#endif // ROOMBRIDGESTONE_H
