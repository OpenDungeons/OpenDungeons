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

#ifndef ROOMTREASURY_H
#define ROOMTREASURY_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

class RoomTreasuryTileData : public TileData
{
public:
    RoomTreasuryTileData() :
        TileData(),
        mGoldInTile(0)
    {}

    RoomTreasuryTileData(const RoomTreasuryTileData* roomTreasuryTileData) :
        TileData(roomTreasuryTileData),
        mGoldInTile(roomTreasuryTileData->mGoldInTile),
        mMeshOfTile(roomTreasuryTileData->mMeshOfTile)
    {}

    virtual ~RoomTreasuryTileData()
    {}

    virtual RoomTreasuryTileData* cloneTileData() const override
    { return new RoomTreasuryTileData(this); }

    int mGoldInTile;
    std::string mMeshOfTile;
};

class RoomTreasury: public Room
{
    friend class ODClient;
public:
    RoomTreasury(GameMap* gameMap);

    virtual RoomType getType() const
    { return RoomType::treasury; }

    // Functions overriding virtual functions in the Room base class.
    bool removeCoveredTile(Tile* t);

    // Functions specific to this class.
    int getTotalGold();
    int emptyStorageSpace();
    int depositGold(int gold, Tile *tile);
    int withdrawGold(int gold);
    virtual void doUpkeep();

    bool hasCarryEntitySpot(GameEntity* carriedEntity);
    Tile* askSpotForCarriedEntity(GameEntity* carriedEntity);
    void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity);

    static int getRoomCost(std::vector<Tile*>& tiles, GameMap* gameMap, RoomType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);
    static void buildRoom(GameMap* gameMap, const std::vector<Tile*>& tiles, Seat* seat);
    static Room* getRoomFromStream(GameMap* gameMap, std::istream& is);

protected:
    RoomTreasuryTileData* createTileData(Tile* tile) override;
    // Because treasury do not use active spots, we don't want the default
    // behaviour (removing the active spot tile) as it could result in removing an
    // unwanted treasury
    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
    {}

private:
    void updateMeshesForTile(Tile* tile, RoomTreasuryTileData* roomTreasuryTileData);
    bool mGoldChanged;
};

#endif // ROOMTREASURY_H
