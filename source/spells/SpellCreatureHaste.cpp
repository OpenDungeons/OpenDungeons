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

#include "spells/SpellCreatureHaste.h"

#include "creatureeffect/CreatureEffectSpeedChange.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "spells/SpellType.h"
#include "spells/SpellManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

static SpellManagerRegister<SpellCreatureHaste> reg(SpellType::creatureHaste, "creatureHaste");

void SpellCreatureHaste::checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    int32_t pricePerTarget = ConfigManager::getSingleton().getSpellConfigInt32("CreatureHastePrice");
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerMana < pricePerTarget)
        {
            std::string txt = formatSpellPrice(SpellType::creatureHaste, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatSpellPrice(SpellType::creatureHaste, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    Tile* tileSelected = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
    if(tileSelected == nullptr)
        return;

    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
    }

    std::vector<EntityBase*> entities;
    // We search the closest creature alive
    tileSelected->fillWithEntities(entities, SelectionEntityWanted::creatureAliveAllied, player);
    Creature* closestCreature = nullptr;
    double closestDist = 0;
    for(EntityBase* entity : entities)
    {
        if(entity->getObjectType() != GameEntityType::creature)
        {
            OD_LOG_ERR("entityName=" + entity->getName() + ", entityType=" + Helper::toString(static_cast<uint32_t>(entity->getObjectType())));
            continue;
        }

        const Ogre::Vector3& entityPos = entity->getPosition();
        double dist = Pathfinding::squaredDistance(entityPos.x, inputManager.mKeeperHandPos.x, entityPos.y, inputManager.mKeeperHandPos.y);
        if(closestCreature == nullptr)
        {
            closestDist = dist;
            closestCreature = static_cast<Creature*>(entity);
            continue;
        }

        if(dist >= closestDist)
            continue;

        closestDist = dist;
        closestCreature = static_cast<Creature*>(entity);
    }

    if(closestCreature == nullptr)
    {
        std::string txt = formatSpellPrice(SpellType::creatureHaste, 0);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        return;
    }

    std::string txt = formatSpellPrice(SpellType::creatureHaste, pricePerTarget);
    inputCommand.displayText(Ogre::ColourValue::White, txt);

    if(inputManager.mCommandState != InputCommandState::validated)
        return;

    inputCommand.unselectAllTiles();

    ClientNotification *clientNotification = SpellManager::createSpellClientNotification(SpellType::creatureHaste);
    clientNotification->mPacket << closestCreature->getName();
    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool SpellCreatureHaste::castSpell(GameMap* gameMap, Player* player, ODPacket& packet)
{
    std::string creatureName;
    OD_ASSERT_TRUE(packet >> creatureName);

    // We check that the creature is a valid target
    Creature* creature = gameMap->getCreature(creatureName);
    if(creature == nullptr)
    {
        OD_LOG_ERR("creatureName=" + creatureName);
        return false;
    }

    if(!creature->getSeat()->isAlliedSeat(player->getSeat()))
    {
        OD_LOG_ERR("creatureName=" + creatureName);
        return false;
    }

    Tile* pos = creature->getPositionTile();
    if(pos == nullptr)
    {
        OD_LOG_ERR("creatureName=" + creatureName);
        return false;
    }

    if(!creature->isAlive())
    {
        // This can happen if the creature was alive on client side but is not since we received the message
        OD_LOG_WRN("creatureName=" + creatureName);
        return false;
    }

    // That can happen if the creature is not in perfect synchronization and is not on a claimed tile on the server gamemap
    if(!pos->isClaimedForSeat(player->getSeat()))
    {
        OD_LOG_WRN("Creature=" + creatureName + ", tile=" + Tile::displayAsString(pos));
        return false;
    }

    int32_t pricePerTarget = ConfigManager::getSingleton().getSpellConfigInt32("CreatureHastePrice");

    if(!player->getSeat()->takeMana(pricePerTarget))
        return false;

    uint32_t duration = ConfigManager::getSingleton().getSpellConfigUInt32("CreatureHasteDuration");
    double value = ConfigManager::getSingleton().getSpellConfigDouble("CreatureHasteValue");
    CreatureEffectSpeedChange* effect = new CreatureEffectSpeedChange(duration, value, "SpellCreatureHaste");
    creature->addCreatureEffect(effect);

    return true;
}

Spell* SpellCreatureHaste::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    OD_LOG_ERR("SpellCreatureHaste cannot be read from stream");
    return nullptr;
}

Spell* SpellCreatureHaste::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    OD_LOG_ERR("SpellCreatureHaste cannot be read from packet");
    return nullptr;
}
