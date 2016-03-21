/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef RESEARCHENTITY_H
#define RESEARCHENTITY_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <iosfwd>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

class SkillEntity: public RenderedMovableEntity
{
public:
    SkillEntity(GameMap* gameMap, const std::string& libraryName, int32_t skillPoints);
    SkillEntity(GameMap* gameMap);

    virtual GameEntityType getObjectType() const override;

    virtual const Ogre::Vector3& getScale() const override;

    inline int32_t getSkillPoints() const
    { return mSkillPoints; }

    virtual EntityCarryType getEntityCarryType(Creature* carrier) override
    { return EntityCarryType::skillEntity; }

    virtual void notifyEntityCarryOn(Creature* carrier) override;
    virtual void notifyEntityCarryOff(const Ogre::Vector3& position) override;

    static SkillEntity* getSkillEntityFromStream(GameMap* gameMap, std::istream& is);
    static SkillEntity* getSkillEntityFromPacket(GameMap* gameMap, ODPacket& is);
    static std::string getSkillEntityStreamFormat();
protected:
    void exportToStream(std::ostream& os) const override;
    bool importFromStream(std::istream& is) override;

private:
    int32_t mSkillPoints;
};

#endif // RESEARCHENTITY_H
