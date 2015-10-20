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

#include "spells/SpellManager.h"

#include "spells/Spell.h"
#include "ODApplication.h"
#include "game/Player.h"
#include "game/Research.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "network/ClientNotification.h"
#include "network/ODPacket.h"
#include "spells/SpellType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string EMPTY_STRING;

namespace
{
    static std::vector<const SpellFactory*>& getFactories()
    {
        static std::vector<const SpellFactory*> factory(static_cast<uint32_t>(SpellType::nbSpells), nullptr);
        return factory;
    }
}

void SpellManager::registerFactory(const SpellFactory* factory)
{
    std::vector<const SpellFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(factory->getSpellType());
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return;
    }

    factories[index] = factory;
}

void SpellManager::unregisterFactory(const SpellFactory* factory)
{
    std::vector<const SpellFactory*>& factories = getFactories();
    auto it = std::find(factories.begin(), factories.end(), factory);
    if(it == factories.end())
    {
        OD_LOG_ERR("Trying to unregister unknown factory=" + factory->getName());
        return;
    }
    factories.erase(it);
}

Spell* SpellManager::load(GameMap* gameMap, std::istream& is)
{
    if(!is.good())
        return nullptr;

    std::vector<const SpellFactory*>& factories = getFactories();
    std::string nextParam;
    OD_ASSERT_TRUE(is >> nextParam);
    const SpellFactory* factoryToUse = nullptr;
    for(const SpellFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getName().compare(nextParam) != 0)
            continue;

        factoryToUse = factory;
        break;
    }

    if(factoryToUse == nullptr)
    {
        OD_LOG_ERR("Unknown Spell type=" + nextParam);
        return nullptr;
    }

    Spell* spell = factoryToUse->getSpellFromStream(gameMap, is);
    if(!spell->importFromStream(is))
    {
        OD_LOG_ERR("Couldn't load creature Spell type=" + nextParam);
        delete spell;
        return nullptr;
    }

    return spell;
}

void SpellManager::dispose(const Spell* spell)
{
    delete spell;
}

void SpellManager::write(const Spell& spell, std::ostream& os)
{
    os << spell.getName();
    spell.exportToStream(os);
}

void SpellManager::checkSpellCast(GameMap* gameMap, SpellType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    uint32_t cooldown = player->getSpellCooldownTurns(type);
    if(cooldown > 0)
    {
        double remainingTime = static_cast<double>(cooldown) / ODApplication::turnsPerSecond;
        std::string errorStr = getSpellNameFromSpellType(type)
            + " (" + Helper::toString(remainingTime, 2)+ " s)";

        inputCommand.displayText(Ogre::ColourValue::Red, errorStr);
        return;
    }

    std::vector<const SpellFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return;
    }

    const SpellFactory& factory = *factories[index];
    factory.checkSpellCast(gameMap, inputManager, inputCommand);
}

bool SpellManager::castSpell(GameMap* gameMap, SpellType type, Player* player, ODPacket& packet)
{
    std::vector<const SpellFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return false;
    }

    const SpellFactory& factory = *factories[index];
    return factory.castSpell(gameMap, player, packet);
}

Spell* SpellManager::getSpellFromStream(GameMap* gameMap, std::istream& is)
{
    SpellType type;
    if(!(is >> type))
        return nullptr;

    std::vector<const SpellFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return nullptr;
    }

    const SpellFactory& factory = *factories[index];
    return factory.getSpellFromStream(gameMap, is);
}

Spell* SpellManager::getSpellFromPacket(GameMap* gameMap, ODPacket& is)
{
    SpellType type;
    if(!(is >> type))
        return nullptr;

    std::vector<const SpellFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return nullptr;
    }

    const SpellFactory& factory = *factories[index];
    return factory.getSpellFromPacket(gameMap, is);
}

const std::string& SpellManager::getSpellNameFromSpellType(SpellType type)
{
    std::vector<const SpellFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return EMPTY_STRING;
    }

    const SpellFactory& factory = *factories[index];
    return factory.getName();
}

const std::string& SpellManager::getSpellReadableName(SpellType type)
{
    std::vector<const SpellFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return EMPTY_STRING;
    }

    const SpellFactory& factory = *factories[index];
    return factory.getNameReadable();
}

SpellType SpellManager::getSpellTypeFromSpellName(const std::string& name)
{
    std::vector<const SpellFactory*>& factories = getFactories();
    for(const SpellFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getName().compare(name) != 0)
            continue;

        return factory->getSpellType();
    }

    OD_LOG_ERR("Cannot find spell name=" + name);
    return SpellType::nullSpellType;
}

ClientNotification* SpellManager::createSpellClientNotification(SpellType type)
{
    ClientNotification *clientNotification = new ClientNotification(ClientNotificationType::askCastSpell);
    clientNotification->mPacket << type;
    return clientNotification;
}

uint32_t SpellManager::getSpellCooldown(SpellType type)
{
    std::vector<const SpellFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return 0;
    }

    const SpellFactory& factory = *factories[index];
    return ConfigManager::getSingleton().getSpellConfigUInt32(factory.getCooldownKey());
}
