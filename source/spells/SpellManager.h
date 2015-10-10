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
class Seat;
class Spell;

enum class SpellType;

//! \brief Factory class to register a new spell
class SpellFactory
{
public:
    virtual ~SpellFactory()
    {}

    virtual SpellType getSpellType() const = 0;
    virtual const std::string& getName() const = 0;
    virtual const std::string& getNameReadable() const = 0;
    virtual const std::string& getCooldownKey() const = 0;

    virtual void checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const = 0;
    virtual bool castSpell(GameMap* gameMap, Player* player, ODPacket& packet) const = 0;
    virtual Spell* getSpellFromStream(GameMap* gameMap, std::istream& is) const = 0;
    virtual Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is) const = 0;
};

class SpellManager
{
friend class SpellRegister;

public:
    static Spell* load(GameMap* gameMap, std::istream& is);
    //! \brief Handles the Spell deletion
    static void dispose(const Spell* spell);
    static void write(const Spell& spell, std::ostream& os);

    //! \brief Called on client side. It checks if the spell can be cast according to the given inputManager
    //! for the given player. It should update the InputCommand to make sure it displays the correct
    //! information (price, selection icon, ...).
    //! An ODPacket should be sent to the server if the spell is validated with relevant data. On server side, castSpell
    //! will be called with the data from the client and it should cast the spell if it is validated.
    static void checkSpellCast(GameMap* gameMap, SpellType type, const InputManager& inputManager, InputCommand& inputCommand);

    //! \brief Called on server side. Casts the spell according to the information in the packet
    //! returns true if the spell was correctly cast and false otherwise
    //! Note that this function does not set the cooldown
    static bool castSpell(GameMap* gameMap, SpellType type, Player* player, ODPacket& packet);

    //! \brief Gets the spell cooldown according to the given type
    static uint32_t getSpellCooldown(SpellType type);

    /*! \brief Exports the headers needed to recreate the Spell. It allows to extend Spells as much as wanted.
     * The content of the Spell will be exported by exportToPacket.
     * Note that spells that do not use these functions can return nullptr
     */
    static Spell* getSpellFromStream(GameMap* gameMap, std::istream &is);
    static Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is);

    //! \brief Gets the spell identification name
    static const std::string& getSpellNameFromSpellType(SpellType type);

    //! \brief Gets the spell readable name
    static const std::string& getSpellReadableName(SpellType type);

    static SpellType getSpellTypeFromSpellName(const std::string& name);

    /*! \brief Creates a ClientNotification to ask for creating a spell. It fills the packet with the needed data
     * for the SpellManager to retrieve the spell (mainly the SpellType) so that the spells only have to handle their
     * specific data.
     */
    static ClientNotification* createSpellClientNotification(SpellType type);

private:
    static void registerFactory(const SpellFactory* factory);
    static void unregisterFactory(const SpellFactory* factory);
};

class SpellRegister
{
public:
    SpellRegister(const SpellFactory* factoryToRegister) :
        mSpellFactory(factoryToRegister)
    {
        SpellManager::registerFactory(mSpellFactory);
    }
    ~SpellRegister()
    {
        SpellManager::unregisterFactory(mSpellFactory);
        delete mSpellFactory;
    }

private:
    const SpellFactory* mSpellFactory;
};

#endif // SPELLMANAGER_H
