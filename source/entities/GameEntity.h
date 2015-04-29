/*!
 * \file   GameEntity.h
 * \date:  16 September 2011
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 *
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

#ifndef GAMEENTITY_H
#define GAMEENTITY_H

#include "entities/EntityBase.h"

#include <OgreVector3.h>
#include <cassert>
#include <string>
#include <vector>
#include <cstdint>

namespace Ogre
{
class SceneNode;
} //End namespace Ogre

class Creature;
class GameMap;
class ODPacket;
class Player;
class Seat;
class Tile;

//! This enum is used to know how carryable entities should be prioritized from lowest
//! to highest
enum class EntityCarryType
{
    notCarryable,
    corpse,
    researchEntity,
    craftedTrap,
    gold
};

/*! \class GameEntity GameEntity.h
 *  \brief This class holds elements that are common to every object placed in the game
 *
 * Functions and properties that are common to every object should be placed into this class
 * and initialised with a good default value in the default constructor.
 * Member variables are private and only accessed through getters and setters.
 */
class GameEntity : public EntityBase
{
  public:
    //! \brief Default constructor with default values
    GameEntity(
          GameMap*        gameMap,
          std::string     name       = std::string(),
          std::string     meshName   = std::string(),
          Seat*           seat        = nullptr
          ) :
    EntityBase(name, meshName, seat),
    mGameMap           (gameMap),
    mIsOnMap           (true)
    {
        assert(mGameMap != nullptr);
    }

    virtual ~GameEntity() {}

    // ===== GETTERS =====

    //! \brief Get if the object can be attacked or not
    virtual bool isAttackable(Tile* tile, Seat* seat) const
    { return false; }

    //! \brief Pointer to the GameMap
    inline GameMap* getGameMap() const
    { return mGameMap; }

    // ===== METHODS =====
    //! \brief Function that schedules the object destruction. This function should not be called twice
    void deleteYourself();

    //! \brief Retrieves the position tile from the game map
    Tile* getPositionTile() const;

    //! \brief defines what happens on each turn with this object on server side
    virtual void doUpkeep() = 0;

    //! \brief defines what happens on each turn with this object on client side. Note
    //! that they need to register to GameMap::addClientUpkeepEntity
    virtual void clientUpkeep()
    {}

    //! \brief Returns a list of the tiles that this object is in/covering.  For creatures and other small objects
    //! this will be a single tile, for larger objects like rooms this will be 1 or more tiles.
    virtual std::vector<Tile*> getCoveredTiles() = 0;

    //! \brief Returns the tile at the given index (nullptr if no tile).
    virtual Tile* getCoveredTile(int index) = 0;

    //! \brief Returns the number of covered tiles.
    virtual uint32_t numCoveredTiles() = 0;

    //! \brief Returns the HP associated with the given tile of the object, it is up to the object how they want to treat the tile/HP relationship.
    virtual double getHP(Tile *tile) const = 0;

    //! \brief Subtracts the given number of hitpoints from the object, the tile specifies where
    //! the enemy inflicted the damage and the object can use this accordingly. attacker is
    //! the entity damaging
    virtual double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage) = 0;

    //! \brief Adds the entity to the correct spaces of the gamemap (animated objects, creature, ...)
    virtual void addToGameMap() = 0;
    virtual void removeFromGameMap() = 0;

    virtual bool getIsOnMap()
    { return mIsOnMap; }

    virtual void setIsOnMap(bool isOnMap)
    { mIsOnMap = isOnMap; }

    virtual bool tryDrop(Seat* seat, Tile* tile)
    { return false; }
    virtual void drop(const Ogre::Vector3& v)
    {}

    void firePickupEntity(Player* playerPicking);
    void fireDropEntity(Player* playerPicking, Tile* tile);

    //! \brief Called each turn with the list of seats that have vision on the tile where the entity is. It should handle
    //! messages to notify players that gain/lose vision
    virtual void notifySeatsWithVision(const std::vector<Seat*>& seats);
    //! \brief Functions to add/remove a seat with vision
    virtual void addSeatWithVision(Seat* seat, bool async);
    virtual void removeSeatWithVision(Seat* seat);

    //! \brief Fires remove event to every seat with vision
    virtual void fireRemoveEntityToSeatsWithVision();

    //! \brief Returns true if the entity can be carried by a worker. False otherwise.
    virtual EntityCarryType getEntityCarryType()
    { return EntityCarryType::notCarryable; }

    //! \brief Called when the entity is being carried
    virtual void notifyEntityCarryOn(Creature* carrier)
    {}

    //! \brief Called when the entity is being carried and is dropped
    virtual void notifyEntityCarryOff(const Ogre::Vector3& position)
    {}

    //! \brief Called when the entity is being carriedand moves
    virtual void notifyCarryMove(const Ogre::Vector3& position)
    { mPosition = position; }

    //! This function should be called on client side just after the entity is added to the gamemap.
    //! It should restore the entity state (if it was dead before the client got vision, it should
    //! be dead on the ground for example).
    //! Note that this function is to be called on client side only
    virtual void restoreEntityState()
    {}

    /*! This function should be called on server side on entities loaded from level files and only them. It will
     * be called after all entities are created and added to the gamemap on all of them to allow to restore
     * links between entities (like beds and creatures). It will be called on Creature, Room, Trap, RenderedMovableEntity
     * and Spell
     */
    virtual void restoreInitialEntityState()
    {}

    static std::string getGameEntityStreamFormat();

    /*! \brief Exports the headers needed to recreate the GameEntity. For example, for missile objects
     * type cannon, it exports GameEntityType::missileObject and MissileType::oneHit. The content of the
     * GameEntityType will be exported by exportToPacket. exportHeadersTo* should export the needed information
     * to know which class should be used. Then, importFromPacket can be called to import the data. The rule of
     * thumb is that importFrom* should be the exact opposite to exportTo*
     * exportToStream and importFromStream are used to write data in level files (editor or, later, save game).
     * exportToPacket and importFromPacket are used to send data from the server to the clients.
     * Note that the functions using stream and packet might not export the same data. Functions using packet will
     * export/import only the needed information for the clients while functions using the stream will export/import
     * every needed information to save/restore the entity from scratch.
     */
    virtual void exportHeadersToStream(std::ostream& os) const;
    virtual void exportHeadersToPacket(ODPacket& os) const;
    //! \brief Exports the data of the GameEntity
    virtual void exportToStream(std::ostream& os) const;
    virtual void importFromStream(std::istream& is);
    virtual void exportToPacket(ODPacket& os) const;
    virtual void importFromPacket(ODPacket& is);
  protected:

    //! \brief Called while moving the entity to add it to the tile it gets on
    virtual bool addEntityToTile(Tile* tile);
    //! \brief Called while moving the entity to remove it from the tile it gets off
    virtual bool removeEntityFromTile(Tile* tile);

    //! \brief Fires a add entity message to the player of the given seat
    virtual void fireAddEntity(Seat* seat, bool async) = 0;
    //! \brief Fires a remove creature message to the player of the given seat (if not null). If null, it fires to
    //! all players with vision
    virtual void fireRemoveEntity(Seat* seat) = 0;
    std::vector<Seat*> mSeatsWithVisionNotified;

  private:
    //! \brief Pointer to the GameMap object.
    GameMap* mGameMap;

    //! \brief Whether the entity is on map or not (for example, when it is
    //! picked up, it is not on map)
    bool mIsOnMap;
};

#endif // GAMEENTITY_H
