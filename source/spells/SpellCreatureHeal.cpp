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

#include "spells/SpellCreatureHeal.h"

#include "creatureeffect/CreatureEffectHeal.h"
#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "sound/SoundEffectsManager.h"
#include "spells/SpellType.h"
#include "spells/SpellManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string SpellCreatureHealName = "creatureHeal";
const std::string SpellCreatureHealNameDisplay = "Creature heal";
const std::string SpellCreatureHealCooldownKey = "CreatureHealCooldown";
const SpellType SpellCreatureHeal::mSpellType = SpellType::creatureHeal;

namespace
{
class SpellCreatureHealFactory : public SpellFactory
{
    SpellType getSpellType() const override
    { return SpellCreatureHeal::mSpellType; }

    const std::string& getName() const override
    { return SpellCreatureHealName; }

    const std::string& getCooldownKey() const override
    { return SpellCreatureHealCooldownKey; }

    const std::string& getNameReadable() const override
    { return SpellCreatureHealNameDisplay; }

    virtual void checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    { SpellCreatureHeal::checkSpellCast(gameMap, inputManager, inputCommand); }

    virtual bool castSpell(GameMap* gameMap, Player* player, ODPacket& packet) const override
    { return SpellCreatureHeal::castSpell(gameMap, player, packet); }

    Spell* getSpellFromStream(GameMap* gameMap, std::istream &is) const override
    { return SpellCreatureHeal::getSpellFromStream(gameMap, is); }

    Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is) const override
    { return SpellCreatureHeal::getSpellFromPacket(gameMap, is); }
};

// Register the factory
static SpellRegister reg(new SpellCreatureHealFactory);
}

void SpellCreatureHeal::checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    int32_t priceTotal = 0;
    int32_t pricePerTarget = ConfigManager::getSingleton().getSpellConfigInt32("CreatureHealPrice");
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerMana < pricePerTarget)
        {
            std::string txt = formatCastSpell(SpellType::creatureHeal, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatCastSpell(SpellType::creatureHeal, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mLStartDragX,
            inputManager.mLStartDragY);
    }

    std::vector<GameEntity*> targets;
    gameMap->playerSelects(targets, inputManager.mXPos, inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY,
        SelectionTileAllowed::groundClaimedAllied, SelectionEntityWanted::creatureAliveOwnedHurt, player);

    if(targets.empty())
    {
        // If we have no owned creature that can be heal, we look for enemy creatures in jail
        gameMap->playerSelects(targets, inputManager.mXPos, inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY,
            SelectionTileAllowed::groundClaimedAllied, SelectionEntityWanted::creatureAliveInOwnedPrisonHurt, player);
    }
    if(targets.empty())
    {
        std::string txt = formatCastSpell(SpellType::creatureHeal, 0);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        return;
    }

    std::random_shuffle(targets.begin(), targets.end());
    std::vector<Creature*> creatures;
    for(GameEntity* target : targets)
    {
        if(playerMana < pricePerTarget)
            break;

        if(target->getObjectType() != GameEntityType::creature)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_LOG_ERR("Wrong target name=" + target->getName() + ", type=" + Helper::toString(static_cast<int32_t>(target->getObjectType())));
            }
            continue;
        }

        Creature* creature = static_cast<Creature*>(target);
        creatures.push_back(creature);

        priceTotal += pricePerTarget;
        playerMana -= pricePerTarget;
    }

    std::string txt = formatCastSpell(SpellType::creatureHeal, priceTotal);
    inputCommand.displayText(Ogre::ColourValue::White, txt);

    if(inputManager.mCommandState != InputCommandState::validated)
        return;

    inputCommand.unselectAllTiles();

    ClientNotification *clientNotification = SpellManager::createSpellClientNotification(SpellType::creatureHeal);
    uint32_t nbCreatures = creatures.size();
    clientNotification->mPacket << nbCreatures;
    for(Creature* creature : creatures)
        clientNotification->mPacket << creature->getName();

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool SpellCreatureHeal::castSpell(GameMap* gameMap, Player* player, ODPacket& packet)
{
    uint32_t nbCreatures;
    OD_ASSERT_TRUE(packet >> nbCreatures);
    std::vector<Creature*> creatures;
    // We can cast heal on allied creatures or on enemy creatures in prison. But not on both at the same time
    bool isEnemyTarget = false;
    while(nbCreatures > 0)
    {
        --nbCreatures;
        std::string creatureName;
        OD_ASSERT_TRUE(packet >> creatureName);

        // We check that the creatures are valid targets
        Creature* creature = gameMap->getCreature(creatureName);
        if(creature == nullptr)
        {
            OD_LOG_ERR("creatureName=" + creatureName);
            continue;
        }

        Tile* pos = creature->getPositionTile();
        if(pos == nullptr)
        {
            OD_LOG_ERR("creatureName=" + creatureName);
            continue;
        }

        if(!creature->isAlive())
        {
            // This can happen if the creature was alive on client side but is not since we received the message
            OD_LOG_WRN("creatureName=" + creatureName);
            continue;
        }

        // That can happen if the creature is not in perfect synchronization and is not on a claimed tile on the server gamemap
        if(!pos->isClaimedForSeat(player->getSeat()))
        {
            OD_LOG_INF("WARNING : " + creatureName + ", tile=" + Tile::displayAsString(pos));
            continue;
        }

        // That can happen if the creature is not in perfect synchronization and is full health on server side but not on client
        if(!creature->isHurt())
        {
            OD_LOG_INF("WARNING : " + creatureName + " is not hurt. Heal cannot be cast on it");
            continue;
        }

        if(!creature->getSeat()->isAlliedSeat(player->getSeat()))
        {
            if(creatures.empty() || isEnemyTarget)
            {
                isEnemyTarget = true;
                creatures.push_back(creature);
                continue;
            }
            OD_LOG_ERR("creatureName=" + creatureName);
            continue;
        }

        if(!creatures.empty() && isEnemyTarget)
        {
            OD_LOG_ERR("creatureName=" + creatureName);
            continue;
        }
        isEnemyTarget = false;
        creatures.push_back(creature);
    }

    if(creatures.empty())
        return false;

    int32_t pricePerTarget = ConfigManager::getSingleton().getSpellConfigInt32("CreatureHealPrice");
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    uint32_t nbTargets = std::min(static_cast<uint32_t>(playerMana / pricePerTarget), static_cast<uint32_t>(creatures.size()));
    int32_t priceTotal = nbTargets * pricePerTarget;

    if(creatures.size() > nbTargets)
        creatures.resize(nbTargets);

    if(!player->getSeat()->takeMana(priceTotal))
        return false;

    uint32_t duration = ConfigManager::getSingleton().getSpellConfigUInt32("CreatureHealDuration");
    double value = ConfigManager::getSingleton().getSpellConfigDouble("CreatureHealValue");
    std::vector<Tile*> affectedTiles;
    for(Creature* creature : creatures)
    {
        CreatureEffectHeal* effect = new CreatureEffectHeal(duration, value, "SpellCreatureHeal");
        creature->addCreatureEffect(effect);

        Tile* tile = creature->getPositionTile();
        if(tile == nullptr)
        {
            OD_LOG_ERR("creature=" + creature->getName() + " on nullptr tile");
            continue;
        }

        if(std::find(affectedTiles.begin(), affectedTiles.end(), tile) != affectedTiles.end())
            continue;

        affectedTiles.push_back(tile);
    }

    for(Tile* tile : affectedTiles)
    {
        fireSpellSound(*tile, "Heal");
    }

    return true;
}

Spell* SpellCreatureHeal::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    OD_LOG_ERR("SpellCreatureHeal cannot be read from stream");
    return nullptr;
}

Spell* SpellCreatureHeal::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    OD_LOG_ERR("SpellCreatureHeal cannot be read from packet");
    return nullptr;
}
