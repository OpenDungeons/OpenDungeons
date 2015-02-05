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

#ifndef TREASURYOBJECT_H
#define TREASURYOBJECT_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <istream>
#include <ostream>

class Room;
class GameMap;
class ODPacket;

class TreasuryObject: public RenderedMovableEntity
{
public:
    TreasuryObject(GameMap* gameMap, int goldValue);
    TreasuryObject(GameMap* gameMap);

    virtual void doUpkeep();

    virtual RenderedMovableEntityType getRenderedMovableEntityType() const
    { return RenderedMovableEntityType::treasuryObject; }

    virtual bool tryPickup(Seat* seat, bool isEditorMode);
    virtual bool tryDrop(Seat* seat, Tile* tile, bool isEditorMode);
    void mergeGold(TreasuryObject* obj);
    void addGold(int goldValue);

    virtual void exportToStream(std::ostream& os) const;
    virtual void importFromStream(std::istream& is);
    virtual void exportToPacket(ODPacket& os) const;
    virtual void importFromPacket(ODPacket& is);

    virtual void pickup();

    virtual bool tryEntityCarryOn();
    virtual void notifyEntityCarryOn();
    virtual void notifyEntityCarryOff(const Ogre::Vector3& position);

    static const char* getMeshNameForGold(int gold);

    static const char* getFormat();
    static TreasuryObject* getTreasuryObjectFromStream(GameMap* gameMap, std::istream& is);
    static TreasuryObject* getTreasuryObjectFromPacket(GameMap* gameMap, ODPacket& is);
protected:
    virtual bool addEntityToTile(Tile* tile);
    virtual bool removeEntityFromTile(Tile* tile);

private:
    int mGoldValue;
    bool mHasGoldValueChanged;
};

#endif // TREASURYOBJECT_H
