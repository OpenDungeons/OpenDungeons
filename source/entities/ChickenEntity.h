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

#ifndef CHICKENENTITY_H
#define CHICKENENTITY_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <istream>
#include <ostream>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

class ChickenEntity: public RenderedMovableEntity
{
public:
    ChickenEntity(GameMap* gameMap, const std::string& hatcheryName);
    ChickenEntity(GameMap* gameMap);

    virtual void doUpkeep();

    virtual RenderedMovableEntityType getRenderedMovableEntityType()
    { return RenderedMovableEntityType::chickenEntity; }

    virtual bool tryPickup(Seat* seat, bool isEditorMode);
    virtual void pickup();
    virtual bool tryDrop(Seat* seat, Tile* tile, bool isEditorMode);

    bool eatChicken(Creature* creature);

    bool canSlap(Seat* seat, bool isEditorMode);

    void slap(bool isEditorMode)
    { mIsSlapped = true; }

    static ChickenEntity* getChickenEntityFromStream(GameMap* gameMap, std::istream& is);
    static ChickenEntity* getChickenEntityFromPacket(GameMap* gameMap, ODPacket& is);
    static const char* getFormat();
private:
    enum ChickenState
    {
        free,
        eaten,
        dying,
        dead
    };
    ChickenState mChickenState;
    int32_t mNbTurnOutsideHatchery;
    int32_t mNbTurnDie;
    bool mIsSlapped;

    void addTileToListIfPossible(int x, int y, Room* currentHatchery, std::vector<Tile*>& possibleTileMove);
};

#endif // CHICKENENTITY_H
