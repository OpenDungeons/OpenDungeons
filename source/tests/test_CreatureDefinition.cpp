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

#define BOOST_TEST_MODULE CreatureDefinition
#include "BoostTestTargetConfig.h"

#include "tests/mocks/TestLogManager.h"
#include "entities/CreatureDefinition.h"

#include <fstream>
#include <memory>

BOOST_AUTO_TEST_CASE(test_CreatureDefinition)
{
    //TODO: implement tests

    //Create log manager instance as the functions will try to access it
    //through the singleton interface.
    TestLogManager t;
    std::stringstream stream;
    std::map<std::string, CreatureDefinition*> defMap;
    CreatureDefinition c;
    CreatureDefinition::writeCreatureDefinitionDiff(nullptr, &c, stream, defMap);

    std::unique_ptr<CreatureDefinition> newDef(CreatureDefinition::load(stream, defMap));
    BOOST_CHECK(newDef->getAtkRangePerLevel() == c.getAtkRangePerLevel());
    BOOST_CHECK(newDef->getAttackRange() == c.getAttackRange());
}
