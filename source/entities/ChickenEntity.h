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
#include <iosfwd>

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

    virtual void doUpkeep() override;

    virtual double getMoveSpeed() const override
    { return 0.4; }

    virtual GameEntityType getObjectType() const override;

    virtual bool tryPickup(Seat* seat) override;
    virtual void pickup() override;
    virtual bool tryDrop(Seat* seat, Tile* tile) override;

    virtual void correctEntityMovePosition(Ogre::Vector3& position) override;

    bool eatChicken(Creature* creature);

    bool canSlap(Seat* seat) override;

    void slap() override
    { mIsSlapped = true; }

    inline bool getLockEat(const Creature& worker) const
    { return mLockedEat; }

    inline void setLockEat(const Creature& worker, bool lock)
    { mLockedEat = lock; }

    static ChickenEntity* getChickenEntityFromStream(GameMap* gameMap, std::istream& is);
    static ChickenEntity* getChickenEntityFromPacket(GameMap* gameMap, ODPacket& is);
    static std::string getChickenEntityStreamFormat();
protected:
    void exportToStream(std::ostream& os) const override;
    bool importFromStream(std::istream& is) override;

private:
    enum ChickenState
    {
        free,
        eaten,
        dying
    };
    ChickenState mChickenState;
    int32_t mNbTurnOutsideHatchery;
    int32_t mNbTurnDie;
    bool mIsSlapped;
    bool mLockedEat;

    void addTileToListIfPossible(int x, int y, Room* currentHatchery, std::vector<Tile*>& possibleTileMove);
};

#endif // CHICKENENTITY_H
