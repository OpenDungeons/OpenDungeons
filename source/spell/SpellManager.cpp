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

#include "spell/SpellManager.h"

#include "network/ODPacket.h"

#include "spell/SpellType.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string EMPTY_STRING;

int SpellFunctions::getSpellCostFunc(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player) const
{
    if(mGetSpellCostFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mGetSpellCostFunc function spell=" + Helper::toString(static_cast<uint32_t>(type)));
        return 0;
    }

    return mGetSpellCostFunc(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void SpellFunctions::castSpellFunc(GameMap* gameMap, SpellType type, const std::vector<EntityBase*>& targets,
    Player* player) const
{
    if(mCastSpellFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mCastSpellFunc function spell=" + Helper::toString(static_cast<uint32_t>(type)));
        return;
    }

    mCastSpellFunc(gameMap, targets, player);
}

Spell* SpellFunctions::getSpellFromStreamFunc(GameMap* gameMap, SpellType type, std::istream& is) const
{
    if(mGetSpellFromStreamFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mGetSpellFromStreamFunc function spell=" + Helper::toString(static_cast<uint32_t>(type)));
        return nullptr;
    }

    return mGetSpellFromStreamFunc(gameMap, is);
}

Spell* SpellFunctions::getSpellFromPacketFunc(GameMap* gameMap, SpellType type, ODPacket& is) const
{
    if(mGetSpellFromPacketFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mGetSpellFromPacketFunc function spell=" + Helper::toString(static_cast<uint32_t>(type)));
        return nullptr;
    }

    return mGetSpellFromPacketFunc(gameMap, is);
}


std::vector<SpellFunctions>& getSpellFunctions()
{
    static std::vector<SpellFunctions> spellList(static_cast<uint32_t>(SpellType::nbSpells));
    return spellList;
}

int SpellManager::getSpellCost(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return 0;
    }

    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    return spellFuncs.getSpellCostFunc(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void SpellManager::castSpell(GameMap* gameMap, SpellType type, const std::vector<EntityBase*>& targets,
    Player* player)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    spellFuncs.castSpellFunc(gameMap, type, targets, player);
}

Spell* SpellManager::getSpellFromStream(GameMap* gameMap, std::istream& is)
{
    SpellType type;
    OD_ASSERT_TRUE(is >> type);
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
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
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
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
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
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

    OD_ASSERT_TRUE_MSG(false, "Cannot find spell name=" + name);
    return SpellType::nullSpellType;
}

void SpellManager::registerSpell(SpellType type, const std::string& name,
    SpellFunctions::GetSpellCostFunc getSpellCostFunc,
    SpellFunctions::CastSpellFunc castSpellFunc,
    SpellFunctions::GetSpellFromStreamFunc getSpellFromStreamFunc,
    SpellFunctions::GetSpellFromPacketFunc getSpellFromPacketFunc)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getSpellFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    SpellFunctions& spellFuncs = getSpellFunctions()[index];
    spellFuncs.mName = name;
    spellFuncs.mGetSpellCostFunc = getSpellCostFunc;
    spellFuncs.mCastSpellFunc = castSpellFunc;
    spellFuncs.mGetSpellFromStreamFunc = getSpellFromStreamFunc;
    spellFuncs.mGetSpellFromPacketFunc = getSpellFromPacketFunc;
}
