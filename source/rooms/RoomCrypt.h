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

#ifndef ROOMCRYPT_H
#define ROOMCRYPT_H

#include "rooms/Room.h"

class RoomCrypt: public Room
{
public:
    RoomCrypt(GameMap* gameMap);

    virtual RoomType getType() const override
    { return RoomType::crypt; }

    void absorbRoom(Room *r) override;

    void doUpkeep() override;

    bool hasCarryEntitySpot(GameEntity* carriedEntity) override;
    Tile* askSpotForCarriedEntity(GameEntity* carriedEntity) override;
    void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity) override;

    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;
protected:
    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override;
private:
    std::map<Tile*,std::pair<Creature*, int32_t> > mRottingCreatures;
    int32_t mRottenPoints;
};

#endif // ROOMCRYPT_H
