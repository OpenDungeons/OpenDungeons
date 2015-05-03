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

#ifndef SPELLMANAGER_H
#define SPELLMANAGER_H

#include <map>
#include <vector>

class EntityBase;
class GameMap;
class Player;
class Spell;

enum class SpellType;

//! Class to gather functions used for spells
class SpellFunctions
{
public:
    typedef int (*GetSpellCostFunc)(std::vector<EntityBase*>& targets, GameMap* gameMap,
        SpellType type, int tileX1, int tileY1, int tileX2, int tileY2, Player* player);
    typedef void (*CastSpellFunc)(GameMap*, const std::vector<EntityBase*>&, Player*);

    SpellFunctions() :
        mGetSpellCostFunc(nullptr),
        mCastSpellFunc(nullptr)
    {}

    GetSpellCostFunc mGetSpellCostFunc;
    CastSpellFunc mCastSpellFunc;
};

class SpellManager
{
public:
    SpellManager()
    {}
    virtual ~SpellManager()
    {}

    static int getSpellCost(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);

    static void castSpell(GameMap* gameMap, SpellType type, const std::vector<EntityBase*>& targets,
        Player* player);

private:
    static std::map<SpellType, SpellFunctions>& getMap();

    template <typename D>
    static int getSpellCostReg(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
    {
        return D::getSpellSummonWorkerCost(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
    }

    template <typename D>
    static void castSpellReg(GameMap* gameMap, const std::vector<EntityBase*>& targets, Player* player)
    {
        D::castSpellSummonWorker(gameMap, targets, player);
    }

    template <typename T> friend class SpellManagerRegister;
};

template <typename T>
class SpellManagerRegister
{
public:
    SpellManagerRegister(SpellType spellType)
    {
        SpellFunctions funcs;
        funcs.mGetSpellCostFunc = &SpellManager::getSpellCostReg<T>;
        funcs.mCastSpellFunc = &SpellManager::castSpellReg<T>;
        SpellManager::getMap()[spellType] = funcs;
    }

private:
    SpellManagerRegister(const SpellManagerRegister&);
};


#endif // SPELLMANAGER_H
