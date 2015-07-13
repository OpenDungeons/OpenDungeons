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

#include "ODApplication.h"
#include "game/Player.h"
#include "game/Research.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "network/ClientNotification.h"
#include "network/ODPacket.h"
#include "spells/SpellType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string EMPTY_STRING;

void SpellFunctions::checkSpellCastFunc(GameMap* gameMap, SpellType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    if(mCheckSpellCastFunc == nullptr)
    {
        OD_LOG_ERR("null mCheckSpellCastFunc function spell=" + Helper::toString(static_cast<uint32_t>(type)));
        return;
    }

    mCheckSpellCastFunc(gameMap, inputManager, inputCommand);
}

bool SpellFunctions::castSpellFunc(GameMap* gameMap, SpellType type, Player* player, ODPacket& packet) const
{
    if(mCastSpellFunc == nullptr)
    {
        OD_LOG_ERR("null mCastSpellFunc function spell=" + Helper::toString(static_cast<uint32_t>(type)));
        return false;
    }

    return mCastSpellFunc(gameMap, player, packet);
}

Spell* SpellFunctions::getSpellFromStreamFunc(GameMap* gameMap, SpellType type, std::istream& is) const
{
    if(mGetSpellFromStreamFunc == nullptr)
    {
        OD_LOG_ERR("null mGetSpellFromStreamFunc function spell=" + Helper::toString(static_cast<uint32_t>(type)));
        return nullptr;
    }

    return mGetSpellFromStreamFunc(gameMap, is);
}

Spell* SpellFunctions::getSpellFromPacketFunc(GameMap* gameMap, SpellType type, ODPacket& is) const
{
    if(mGetSpellFromPacketFunc == nullptr)
    {
        OD_LOG_ERR("null mGetSpellFromPacketFunc function spell=" + Helper::toString(static_cast<uint32_t>(type)));
        return nullptr;
    }

    return mGetSpellFromPacketFunc(gameMap, is);
}


std::vector<SpellFunctions>& getSpellFunctions()
{
    static std::vector<SpellFunctions> spellList(static_cast<uint32_t>(SpellType::nbSpells));
    return spellList;
}

void SpellManager::checkSpellCast(GameMap* gameMap, SpellType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index));
        return;
    }

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
    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    spellFuncs.checkSpellCastFunc(gameMap, type, inputManager, inputCommand);
}

bool SpellManager::castSpell(GameMap* gameMap, SpellType type, Player* player, ODPacket& packet)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index));
        return false;
    }

    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    return spellFuncs.castSpellFunc(gameMap, type, player, packet);
}

Spell* SpellManager::getSpellFromStream(GameMap* gameMap, std::istream& is)
{
    SpellType type;
    OD_ASSERT_TRUE(is >> type);
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index));
        return nullptr;
    }

    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    return spellFuncs.getSpellFromStreamFunc(gameMap, type, is);
}

Spell* SpellManager::getSpellFromPacket(GameMap* gameMap, ODPacket& is)
{
    SpellType type;
    OD_ASSERT_TRUE(is >> type);
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index));
        return nullptr;
    }

    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    return spellFuncs.getSpellFromPacketFunc(gameMap, type, is);
}

const std::string& SpellManager::getSpellNameFromSpellType(SpellType type)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index));
        return EMPTY_STRING;
    }
    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    return spellFuncs.mName;
}

SpellType SpellManager::getSpellTypeFromSpellName(const std::string& name)
{
    uint32_t nbSpells = static_cast<uint32_t>(SpellType::nbSpells);
    for(uint32_t i = 0; i < nbSpells; ++i)
    {
        SpellFunctions& spellFuncs = getSpellFunctions()[i];
        if(name.compare(spellFuncs.mName) == 0)
            return static_cast<SpellType>(i);
    }

    OD_LOG_ERR("Cannot find spell name=" + name);
    return SpellType::nullSpellType;
}

void SpellManager::registerSpell(SpellType type, const std::string& name,
    SpellFunctions::CheckSpellCastFunc checkSpellCastFunc,
    SpellFunctions::CastSpellFunc castSpellFunc,
    SpellFunctions::GetSpellFromStreamFunc getSpellFromStreamFunc,
    SpellFunctions::GetSpellFromPacketFunc getSpellFromPacketFunc)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index));
        return;
    }

    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    spellFuncs.mName = name;
    spellFuncs.mCheckSpellCastFunc = checkSpellCastFunc;
    spellFuncs.mCastSpellFunc = castSpellFunc;
    spellFuncs.mGetSpellFromStreamFunc = getSpellFromStreamFunc;
    spellFuncs.mGetSpellFromPacketFunc = getSpellFromPacketFunc;
}

ClientNotification* SpellManager::createSpellClientNotification(SpellType type)
{
    ClientNotification *clientNotification = new ClientNotification(ClientNotificationType::askCastSpell);
    clientNotification->mPacket << type;
    return clientNotification;
}
