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

#ifndef ROOMCASINO_H
#define ROOMCASINO_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

class Creature;
class Tile;

class RoomCasinoGameCreatureInfo
{
public:
    RoomCasinoGameCreatureInfo(Creature* creature, bool isReadyCreature) :
        mCreature(nullptr),
        mIsReady(false)
    {}

    Creature* mCreature;
    bool mIsReady;
};

class RoomCasinoGame
{
public:
    RoomCasinoGame() :
        mCreature1(nullptr, false),
        mCreature2(nullptr, false),
        mCooldown(0)
    {}

    RoomCasinoGameCreatureInfo mCreature1;
    RoomCasinoGameCreatureInfo mCreature2;
    uint32_t mCooldown;
};

class RoomCasino: public Room
{
public:
    RoomCasino(GameMap* gameMap);

    RoomType getType() const override
    { return mRoomType; }

    void doUpkeep() override;
    bool hasOpenCreatureSpot(Creature* creature) override;
    bool addCreatureUsingRoom(Creature* creature) override;
    void removeCreatureUsingRoom(Creature* creature) override;
    void absorbRoom(Room* room) override;
    bool useRoom(Creature& creature, bool forced) override;

    static const RoomType mRoomType;

protected:
    RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override;

private:
    void setCreatureWinning(Creature& creature, const Ogre::Vector3& gamePosition);
    void setCreatureLoosing(Creature& creature, const Ogre::Vector3& gamePosition);
    std::map<Tile*,RoomCasinoGame> mCreaturesSpots;
};

#endif // ROOMCASINO_H
