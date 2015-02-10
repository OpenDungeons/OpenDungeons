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

#include "entities/CreatureDefinition.h"

#include "spawnconditions/SpawnCondition.h"
#include "spawnconditions/SpawnConditionCreature.h"
#include "spawnconditions/SpawnConditionGold.h"
#include "spawnconditions/SpawnConditionRoom.h"

#include "rooms/Room.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::vector<const SpawnCondition*> SpawnCondition::EMPTY_SPAWNCONDITIONS;

SpawnCondition* SpawnCondition::load(std::istream& defFile)
{
    std::string nextParam;
    SpawnCondition* condition = nullptr;
    while (defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/Condition]" || nextParam == "[/SpawnCondition]" || nextParam == "[/SpawnConditions]")
            return condition;

        if(condition != nullptr)
        {
            // The previous line was a valid condition so we should have had an ending tag
            OD_ASSERT_TRUE_MSG(false, "nextParam=" + nextParam);
            return condition;
        }
        if (nextParam == "Room")
        {
            if(!(defFile >> nextParam))
                break;
            RoomType roomType = Room::getRoomTypeFromRoomName(nextParam);
            if(roomType == RoomType::nullRoomType)
            {
                OD_ASSERT_TRUE_MSG(false, "nextParam=" + nextParam);
                break;
            }
            if(!(defFile >> nextParam))
                break;
            int32_t nbActiveSpotsMin = Helper::toInt(nextParam);
            if(!(defFile >> nextParam))
                break;
            int32_t pointsPerAdditionalActiveSpots = Helper::toInt(nextParam);

            condition = new SpawnConditionRoom(roomType, nbActiveSpotsMin, pointsPerAdditionalActiveSpots);
        }

        if (nextParam == "Creature")
        {
            if(!(defFile >> nextParam))
                break;
            const CreatureDefinition* creatureDefinition = ConfigManager::getSingleton().getCreatureDefinition(nextParam);
            if(creatureDefinition == nullptr)
            {
                OD_ASSERT_TRUE_MSG(false, "nextParam=" + nextParam);
                return nullptr;
            }
            if(!(defFile >> nextParam))
                break;
            int32_t nbCreatureMin = Helper::toInt(nextParam);
            if(!(defFile >> nextParam))
                break;
            int32_t pointsPerAdditionalCreature = Helper::toInt(nextParam);

            condition = new SpawnConditionCreature(creatureDefinition, nbCreatureMin, pointsPerAdditionalCreature);
        }

        if (nextParam == "Gold")
        {
            if(!(defFile >> nextParam))
                break;
            int32_t nbGoldMin = Helper::toInt(nextParam);
            if(!(defFile >> nextParam))
                break;
            int32_t pointsPerAdditional100Gold = Helper::toInt(nextParam);

            condition = new SpawnConditionGold(nbGoldMin, pointsPerAdditional100Gold);
        }
    }
    OD_ASSERT_TRUE(false);
    return nullptr;
}
