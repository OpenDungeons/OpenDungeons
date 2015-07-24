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

#ifndef GIFTBOXENTITY_H
#define GIFTBOXENTITY_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <iosfwd>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

enum class GiftBoxType
{
    research,
    nbTypes
};
std::ostream& operator<<(std::ostream& os, const GiftBoxType& type);
std::istream& operator>>(std::istream& is, GiftBoxType& type);

class GiftBoxEntity: public RenderedMovableEntity
{
public:
    GiftBoxEntity(GameMap* gameMap, bool isOnServerMap, const std::string& baseName, const std::string& meshName, GiftBoxType type);
    GiftBoxEntity(GameMap* gameMap, bool isOnServerMap);

    virtual GameEntityType getObjectType() const override
    { return GameEntityType::giftBoxEntity; }

    virtual const Ogre::Vector3& getScale() const override;

    virtual EntityCarryType getEntityCarryType() override
    { return EntityCarryType::giftBox; }

    virtual void notifyEntityCarryOn(Creature* carrier) override;
    virtual void notifyEntityCarryOff(const Ogre::Vector3& position) override;

    //! \brief Server side function that will be called when the gift box is carried to the dungeon temple. It should be
    //! called by the overriding classes to do what they need. However, this function is not pure virtual because on client side,
    //! we don't want to make a difference
    virtual void applyEffect()
    {}

    static GiftBoxEntity* getGiftBoxEntityFromStream(GameMap* gameMap, std::istream& is);
    static GiftBoxEntity* getGiftBoxEntityFromPacket(GameMap* gameMap, ODPacket& is);
    static std::string getGiftBoxEntityStreamFormat();
protected:
    virtual void exportHeadersToStream(std::ostream& os) const override;

private:
    GiftBoxType mGiftBoxType;
};

#endif // GIFTBOXENTITY_H
