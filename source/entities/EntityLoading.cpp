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

#include "entities/GameEntity.h"
#include "entities/GameEntityType.h"
#include "entities/BuildingObject.h"
#include "entities/ChickenEntity.h"
#include "entities/CraftedTrap.h"
#include "entities/Creature.h"
#include "entities/GiftBoxEntity.h"
#include "entities/MapLight.h"
#include "entities/MissileObject.h"
#include "entities/PersistentObject.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/SkillEntity.h"
#include "entities/SmallSpiderEntity.h"
#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "entities/TreasuryObject.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "spells/Spell.h"
#include "spells/SpellManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>


namespace Entities
{
GameEntity* getGameEntityFromStream(GameMap* gameMap, GameEntityType type, std::istream& is)
{
    GameEntity* entity = nullptr;
    switch(type)
    {
        case GameEntityType::buildingObject:
        {
            // Building objects are not stored in level files, we should not be here
            OD_LOG_ERR("Cannot create entity from stream");
            break;
        }
        case GameEntityType::chickenEntity:
        {
            entity = ChickenEntity::getChickenEntityFromStream(gameMap, is);
            break;
        }
        case GameEntityType::craftedTrap:
        {
            entity = CraftedTrap::getCraftedTrapFromStream(gameMap, is);
            break;
        }
        case GameEntityType::creature:
        {
            entity = Creature::getCreatureFromStream(gameMap, is);
            break;
        }
        case GameEntityType::mapLight:
        {
            entity = MapLight::getMapLightFromStream(gameMap, is);
            break;
        }
        case GameEntityType::missileObject:
        {
            entity = MissileObject::getMissileObjectFromStream(gameMap, is);
            break;
        }
        case GameEntityType::persistentObject:
        {
            // Persistent objects are not stored in level files, we should not be here
            OD_LOG_ERR("Cannot create entity from stream");
            break;
        }
        case GameEntityType::smallSpiderEntity:
        {
            // Small spiders are not stored in level files, we should not be here
            OD_LOG_ERR("Cannot create entity from stream");
            break;
        }
        case GameEntityType::spell:
        {
            entity = SpellManager::getSpellFromStream(gameMap, is);
            break;
        }
        case GameEntityType::trapEntity:
        {
            // Trap entities are handled by the trap
            OD_LOG_ERR("Cannot create entity from stream");
            break;
        }
        case GameEntityType::treasuryObject:
        {
            entity = TreasuryObject::getTreasuryObjectFromStream(gameMap, is);
            break;
        }
        case GameEntityType::skillEntity:
        {
            entity = SkillEntity::getSkillEntityFromStream(gameMap, is);
            break;
        }
        case GameEntityType::giftBoxEntity:
        {
            entity = GiftBoxEntity::getGiftBoxEntityFromStream(gameMap, is);
            break;
        }
        default:
        {
            OD_LOG_ERR("Unknown enum value : " + Helper::toString(
                static_cast<int>(type)));
            break;
        }
    }

    if(entity == nullptr)
    {
        OD_LOG_ERR("null entity type=" + Helper::toString(static_cast<uint32_t>(type)));
        return nullptr;
    }

    return entity;
}

GameEntity* getGameEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    GameEntity* entity = nullptr;
    GameEntityType type;
    OD_ASSERT_TRUE(is >> type);
    switch(type)
    {
        case GameEntityType::buildingObject:
        {
            entity = BuildingObject::getBuildingObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::chickenEntity:
        {
            entity = ChickenEntity::getChickenEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::craftedTrap:
        {
            entity = CraftedTrap::getCraftedTrapFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::creature:
        {
            entity = Creature::getCreatureFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::mapLight:
        {
            entity = MapLight::getMapLightFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::missileObject:
        {
            entity = MissileObject::getMissileObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::persistentObject:
        {
            entity = PersistentObject::getPersistentObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::smallSpiderEntity:
        {
            entity = SmallSpiderEntity::getSmallSpiderEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::spell:
        {
            entity = SpellManager::getSpellFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::trapEntity:
        {
            entity = TrapEntity::getTrapEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::treasuryObject:
        {
            entity = TreasuryObject::getTreasuryObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::skillEntity:
        {
            entity = SkillEntity::getSkillEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::giftBoxEntity:
        {
            entity = GiftBoxEntity::getGiftBoxEntityFromPacket(gameMap, is);
            break;
        }
        default:
        {
            OD_LOG_ERR("Unknown enum value : " + Helper::toString(static_cast<int>(type)));
            break;
        }
    }

    OD_ASSERT_TRUE(entity != nullptr);
    if(entity == nullptr)
    {
        OD_LOG_ERR("null entity type=" + Helper::toString(static_cast<int>(type)));
        return nullptr;
    }

    return entity;
}
} //namespace Entities
