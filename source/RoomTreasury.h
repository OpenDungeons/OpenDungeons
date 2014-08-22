/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "Room.h"
#include "ODPacket.h"

class RoomTreasury: public Room
{
    friend class ODClient;
public:
    RoomTreasury(GameMap* gameMap);

    // Functions overriding virtual functions in the Room base class.
    void absorbRoom(Room *r);
    bool doUpkeep();
    void addCoveredTile(Tile* t, double nHP = defaultRoomTileHP);
    void removeCoveredTile(Tile* t);
    void clearCoveredTiles();

    // Functions specific to this class.
    int getTotalGold();
    int emptyStorageSpace();
    int depositGold(int gold, Tile *tile);
    int withdrawGold(int gold);
protected:
    virtual void destroyMeshLocal();
    virtual void deleteYourselfLocal();
private:
    //! \brief Tells which room object is used to show how much the tile is full of gold.
    enum TreasuryTileFullness
    {
        noGold = 0, quarter, half, threeQuarters, fullOfGold
    };

    static TreasuryTileFullness getTreasuryTileFullness(int gold);
    const char* getMeshNameForTreasuryTileFullness(TreasuryTileFullness fullness);

    void updateMeshesForTile(Tile *t);
    void createMeshesForTile(Tile *t, TreasuryTileFullness fullness);
    void destroyMeshesForTile(Tile *t, TreasuryTileFullness fullness);
    void createGoldMeshes();
    void destroyGoldMeshes(bool resetValues);

    friend ODPacket& operator<<(ODPacket& os, const TreasuryTileFullness& tf);
    friend ODPacket& operator>>(ODPacket& is, TreasuryTileFullness& tf);

    std::map<Tile*, int> mGoldInTile;
    std::map<Tile*, TreasuryTileFullness> mFullnessOfTile;
};

#endif // ROOMTREASURY_H
