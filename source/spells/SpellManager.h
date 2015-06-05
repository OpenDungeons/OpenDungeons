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

class ClientNotification;
class EntityBase;
class GameMap;
class InputCommand;
class InputManager;
class ODPacket;
class Player;
class Spell;

enum class SpellType;

//! Class to gather functions used for spells
class SpellFunctions
{
    friend class SpellManager;
public:
    typedef void (*CheckSpellCastFunc)(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    typedef bool (*CastSpellFunc)(GameMap* gameMap, Player* player, ODPacket& packet);
    typedef Spell* (*GetSpellFromStreamFunc)(GameMap* gameMap, std::istream& is);
    typedef Spell* (*GetSpellFromPacketFunc)(GameMap* gameMap, ODPacket& is);

    SpellFunctions() :
        mCheckSpellCastFunc(nullptr),
        mCastSpellFunc(nullptr),
        mGetSpellFromStreamFunc(nullptr),
        mGetSpellFromPacketFunc(nullptr)
    {}

    void checkSpellCastFunc(GameMap* gameMap, SpellType type, const InputManager& inputManager, InputCommand& inputCommand) const;

    bool castSpellFunc(GameMap* gameMap, SpellType type, Player* player, ODPacket& packet) const;

    Spell* getSpellFromStreamFunc(GameMap* gameMap, SpellType type, std::istream& is) const;

    Spell* getSpellFromPacketFunc(GameMap* gameMap, SpellType type, ODPacket& is) const;

private:
    std::string mName;
    CheckSpellCastFunc mCheckSpellCastFunc;
    CastSpellFunc mCastSpellFunc;
    GetSpellFromStreamFunc mGetSpellFromStreamFunc;
    GetSpellFromPacketFunc mGetSpellFromPacketFunc;

};

class SpellManager
{
public:
    //! \brief Called on client side. It checks if the spell can be cast according to the given inputManager
    //! for the given player. It should update the InputCommand to make sure it displays the correct
    //! information (price, selection icon, ...).
    //! An ODPacket should be sent to the server if the spell is validated with relevant data. On server side, castSpell
    //! will be called with the data from the client and it should cast the spell if it is validated.
    static void checkSpellCast(GameMap* gameMap, SpellType type, const InputManager& inputManager, InputCommand& inputCommand);

    //! \brief Called on server side. Casts the spell according to the information in the packet
    //! returns true if the spell was correctly cast and false otherwise
    static bool castSpell(GameMap* gameMap, SpellType type, Player* player, ODPacket& packet);

    /*! \brief Exports the headers needed to recreate the Spell. It allows to extend Spells as much as wanted.
     * The content of the Spell will be exported by exportToPacket.
     * Note that spells that do not use these functions can return nullptr
     */
    static Spell* getSpellFromStream(GameMap* gameMap, std::istream &is);
    static Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is);

    static const std::string& getSpellNameFromSpellType(SpellType type);

    static SpellType getSpellTypeFromSpellName(const std::string& name);

    /*! \brief Creates a ClientNotification to ask for creating a spell. It fills the packet with the needed data
     * for the SpellManager to retrieve the spell (mainly the SpellType) so that the spells only have to handle their
     * specific data.
     */
    static ClientNotification* createSpellClientNotification(SpellType type);

private:
    static void registerSpell(SpellType type, const std::string& name,
        SpellFunctions::CheckSpellCastFunc checkSpellCastFunc,
        SpellFunctions::CastSpellFunc castSpellFunc,
        SpellFunctions::GetSpellFromStreamFunc getSpellFromStreamFunc,
        SpellFunctions::GetSpellFromPacketFunc getSpellFromPacketFunc);

    template <typename D>
    static void checkSpellCastReg(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
    {
        return D::checkSpellCast(gameMap, inputManager, inputCommand);
    }

    template <typename D>
    static bool castSpellReg(GameMap* gameMap, Player* player, ODPacket& packet)
    {
        return D::castSpell(gameMap, player, packet);
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
        SpellManager::registerSpell(spellType, name, &SpellManager::checkSpellCastReg<T>,
            &SpellManager::castSpellReg<T>, &SpellManager::getSpellFromStreamReg<T>,
            &SpellManager::getSpellFromPacketReg<T>);
    }

private:
    SpellManagerRegister(const std::string& name, const SpellManagerRegister&);
};


#endif // SPELLMANAGER_H
