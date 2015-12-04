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

#include "creaturemood/CreatureMoodCreature.h"

#include "creaturemood/CreatureMoodManager.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/GameEntityType.h"
#include "gamemap/GameMap.h"
#include "utils/LogManager.h"

static const std::string CreatureMoodCreatureName = "Creature";

namespace
{
class CreatureMoodCreatureFactory : public CreatureMoodFactory
{
    CreatureMood* createCreatureMood() const override
    { return new CreatureMoodCreature; }

    const std::string& getCreatureMoodName() const override
    {
        return CreatureMoodCreatureName;
    }
};

// Register the factory
static CreatureMoodRegister reg(new CreatureMoodCreatureFactory);
}

const std::string& CreatureMoodCreature::getModifierName() const
{
    return CreatureMoodCreatureName;
}

int32_t CreatureMoodCreature::computeMood(const Creature& creature) const
{
    std::vector<GameEntity*> alliedCreatures = creature.getGameMap()->getVisibleCreatures(creature.getVisibleTiles(),
        creature.getSeat(), false);
    int nbCreatures = 0;
    for(GameEntity* entity : alliedCreatures)
    {
        if(entity->getObjectType() != GameEntityType::creature)
            continue;

        if(&creature == entity)
            continue;

        Creature* alliedCreature = static_cast<Creature*>(entity);
        if(alliedCreature->getDefinition()->getClassName() != mCreatureClass)
            continue;

        ++nbCreatures;
    }
    return nbCreatures * mMoodModifier;
}

CreatureMoodCreature* CreatureMoodCreature::clone() const
{
    return new CreatureMoodCreature(*this);
}

bool CreatureMoodCreature::importFromStream(std::istream& is)
{
    if(!CreatureMood::importFromStream(is))
        return false;

    if(!(is >> mCreatureClass))
        return false;
    if(!(is >> mMoodModifier))
        return false;

    return true;
}

void CreatureMoodCreature::exportToStream(std::ostream& os) const
{
    CreatureMood::exportToStream(os);
    os << "\t" << mCreatureClass;
    os << "\t" << mMoodModifier;
}

void CreatureMoodCreature::getFormatString(std::string& format) const
{
    CreatureMood::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "CreatureClass\tMoodModifier";
}
