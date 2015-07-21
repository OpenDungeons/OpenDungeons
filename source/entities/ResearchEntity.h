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

class ResearchEntity: public RenderedMovableEntity
{
public:
    ResearchEntity(GameMap* gameMap, bool isOnServerMap, const std::string& libraryName, int32_t researchPoints);
    ResearchEntity(GameMap* gameMap, bool isOnServerMap);

    virtual GameEntityType getObjectType() const override
    { return GameEntityType::researchEntity; }

    virtual const Ogre::Vector3& getScale() const override;

    inline int32_t getResearchPoints() const
    { return mResearchPoints; }

    virtual EntityCarryType getEntityCarryType() override
    { return EntityCarryType::researchEntity; }

    virtual void notifyEntityCarryOn(Creature* carrier) override;
    virtual void notifyEntityCarryOff(const Ogre::Vector3& position) override;

    static ResearchEntity* getResearchEntityFromStream(GameMap* gameMap, std::istream& is);
    static ResearchEntity* getResearchEntityFromPacket(GameMap* gameMap, ODPacket& is);
    static std::string getResearchEntityStreamFormat();
protected:
    void exportToStream(std::ostream& os) const override;
    void importFromStream(std::istream& is) override;

private:
    int32_t mResearchPoints;
};

#endif // RESEARCHENTITY_H
