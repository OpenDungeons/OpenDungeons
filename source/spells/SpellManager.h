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

#include <vector>
#include <istream>
#include <cstdint>

class EntityBase;
class GameMap;
class ODPacket;
class Player;
class Spell;

enum class SpellType;

//! Class to gather functions used for spells
class SpellFunctions
{
    friend class SpellManager;
public:
    typedef int (*GetSpellCostFunc)(std::vector<EntityBase*>& targets, GameMap* gameMap,
        SpellType type, int tileX1, int tileY1, int tileX2, int tileY2, Player* player);
    typedef void (*CastSpellFunc)(GameMap*, const std::vector<EntityBase*>&, Player*);
    typedef Spell* (*GetSpellFromStreamFunc)(GameMap* gameMap, std::istream& is);
    typedef Spell* (*GetSpellFromPacketFunc)(GameMap* gameMap, ODPacket& is);

    SpellFunctions() :
        mGetSpellCostFunc(nullptr),
        mCastSpellFunc(nullptr),
        mGetSpellFromStreamFunc(nullptr),
        mGetSpellFromPacketFunc(nullptr)
    {}

    int getSpellCostFunc(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player) const;

    void castSpellFunc(GameMap* gameMap, SpellType type, const std::vector<EntityBase*>& targets,
        Player* player) const;

    Spell* getSpellFromStreamFunc(GameMap* gameMap, SpellType type, std::istream& is) const;

    Spell* getSpellFromPacketFunc(GameMap* gameMap, SpellType type, ODPacket& is) const;

private:
    std::string mName;
    GetSpellCostFunc mGetSpellCostFunc;
    CastSpellFunc mCastSpellFunc;
    GetSpellFromStreamFunc mGetSpellFromStreamFunc;
    GetSpellFromPacketFunc mGetSpellFromPacketFunc;

};

class SpellManager
{
public:
    //! Returns the spell cost required to cast the spell for the given player. targets will
    //! be filled with the suitable targets. Note that if there are more targets than available mana,
    //! most spells will fill targets until no more mana is left (chosen randomly between available
    //! targets)
    //! If no target is available, this function should return the mana needed for 1 target and
    //! targets vector should be empty
    //! Returns the mana the spell will cost. If < 0, it means the spell cannot be cast
    static int getSpellCost(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);

    //! Casts the spell. In most of the cases, targets should be the vector filled by getSpellCost
    static void castSpell(GameMap* gameMap, SpellType type, const std::vector<EntityBase*>& targets,
        Player* player);

    /*! \brief Exports the headers needed to recreate the Spell. It allows to extend Spells as much as wanted.
     * The content of the Spell will be exported by exportToPacket.
     * Note that spells that do not use these functions can return nullptr
     */
    static Spell* getSpellFromStream(GameMap* gameMap, std::istream &is);
    static Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is);

    static const std::string& getSpellNameFromSpellType(SpellType type);

    static SpellType getSpellTypeFromSpellName(const std::string& name);

private:
    static void registerSpell(SpellType type, const std::string& name,
        SpellFunctions::GetSpellCostFunc getSpellCostFunc,
        SpellFunctions::CastSpellFunc castSpellFunc,
        SpellFunctions::GetSpellFromStreamFunc getSpellFromStreamFunc,
        SpellFunctions::GetSpellFromPacketFunc getSpellFromPacketFunc);

    template <typename D>
    static int getSpellCostReg(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
    {
        return D::getSpellCost(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
    }

    template <typename D>
    static void castSpellReg(GameMap* gameMap, const std::vector<EntityBase*>& targets, Player* player)
    {
        D::castSpell(gameMap, targets, player);
    }

    template <typename D>
    static Spell* getSpellFromStreamReg(GameMap* gameMap, std::istream& is)
    {
        return D::getSpellFromStream(gameMap, is);
    }

    template <typename D>
    static Spell* getSpellFromPacketReg(GameMap* gameMap, ODPacket& is)
    {
        return D::getSpellFromPacket(gameMap, is);
    }

    template <typename T> friend class SpellManagerRegister;
};

template <typename T>
class SpellManagerRegister
{
public:
    SpellManagerRegister(SpellType spellType, const std::string& name)
    {
        SpellManager::registerSpell(spellType, name, &SpellManager::getSpellCostReg<T>,
            &SpellManager::castSpellReg<T>, &SpellManager::getSpellFromStreamReg<T>,
            &SpellManager::getSpellFromPacketReg<T>);
    }

private:
    SpellManagerRegister(const std::string& name, const SpellManagerRegister&);
};


#endif // SPELLMANAGER_H
