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
#include "rooms/RoomType.h"

enum class TrapType;

class RoomWorkshopTileData : public TileData
{
public:
    RoomWorkshopTileData() :
        TileData(),
        mCanHaveCraftedTrap(true)
    {}

    RoomWorkshopTileData(const RoomWorkshopTileData* roomWorkshopTileData) :
        TileData(roomWorkshopTileData),
        mCanHaveCraftedTrap(roomWorkshopTileData->mCanHaveCraftedTrap)
    {}

    virtual ~RoomWorkshopTileData()
    {}

    virtual RoomWorkshopTileData* cloneTileData() const override
    { return new RoomWorkshopTileData(this); }

    bool mCanHaveCraftedTrap;
};

class RoomWorkshop: public Room{
public:
    RoomWorkshop(GameMap* gameMap);

    virtual RoomType getType() const override
    { return mRoomType; }

    virtual void doUpkeep() override;
    virtual bool hasOpenCreatureSpot(Creature* c) override;
    virtual bool addCreatureUsingRoom(Creature* c) override;
    virtual void removeCreatureUsingRoom(Creature* c) override;
    virtual void absorbRoom(Room *r) override;

    static void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet);
    static bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles);
    static void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoomEditor(GameMap* gameMap, ODPacket& packet);
    static Room* getRoomFromStream(GameMap* gameMap, std::istream& is);

    static const RoomType mRoomType;

protected:
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

    RoomWorkshopTileData* createTileData(Tile* tile) override;
    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override;
private:
    //!\brief checks if a tile is available in the workshop to place a new crafted trap
    uint32_t countCraftedItemsOnRoom();
    Tile* checkIfAvailableSpot();
    int32_t mPoints;
    TrapType mTrapType;
    void getCreatureWantedPos(Creature* creature, Tile* tileSpot,
        Ogre::Real& wantedX, Ogre::Real& wantedY);
    std::vector<Tile*> mUnusedSpots;
    std::map<Creature*,Tile*> mCreaturesSpots;
};

#endif // ROOMWORKSHOP_H
