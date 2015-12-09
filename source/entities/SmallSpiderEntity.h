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

#ifndef SMALLSPIDERENTITY_H
#define SMALLSPIDERENTITY_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <iosfwd>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

class SmallSpiderEntity: public RenderedMovableEntity
{
public:
    SmallSpiderEntity(GameMap* gameMap, bool isOnServerMap, const std::string& cryptName, int32_t nbTurnLife);
    SmallSpiderEntity(GameMap* gameMap, bool isOnServerMap);

    virtual void doUpkeep() override;

    virtual GameEntityType getObjectType() const override;

    bool canSlap(Seat* seat) override;

    virtual void correctEntityMovePosition(Ogre::Vector3& position) override;

    void slap()
    { mIsSlapped = true; }

    static SmallSpiderEntity* getSmallSpiderEntityFromPacket(GameMap* gameMap, ODPacket& is);

private:
    void addTileToListIfPossible(int x, int y, Room* currentCrypt, std::vector<Tile*>& possibleTileMove);
    int32_t mNbTurnLife;
    bool mIsSlapped;
};

#endif // SMALLSPIDERENTITY_H
