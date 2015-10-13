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
class ParticleSystem;
} //End namespace Ogre

class Creature;
class GameEntity;
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
    giftBox,
    gold,
    koCreature
};

enum class EntityParticleEffectType
{
    creature,
    missile,
    nbEffects
};

class EntityParticleEffect
{
public:
    EntityParticleEffect(const std::string& name, const std::string& script, uint32_t nbTurnsEffect) :
        mName(name),
        mScript(script),
        mParticleSystem(nullptr),
        mNbTurnsEffect(nbTurnsEffect)
    {}

    virtual ~EntityParticleEffect()
    {}

    virtual EntityParticleEffectType getEntityParticleEffectType() = 0;

    std::string mName;
    std::string mScript;
    Ogre::ParticleSystem* mParticleSystem;
    uint32_t mNbTurnsEffect;
};

/*! \brief Allows to subscribe to events happening to a GameEntity.
 *  That will allow to save a pointer on a GameEntity and subscribe to events. If the
 *  entity is removed from the gamemap, pickedup, ... we are sure to be notified and that
 *  the given pointer will be valid
 */
class GameEntityListener
{
public:
    GameEntityListener()
    {}

    virtual ~GameEntityListener()
    {}

    virtual std::string getListenerName() const = 0;

    //! \brief Called when a GameEntity we asked to be notified of is dead (note that some entities
    //! might never send this event like rooms or traps)
    //! If false is returned, the listener will be removed from the listener list
    virtual bool notifyDead(GameEntity* entity) = 0;

    //! \brief Called when a GameEntity we asked to be notified of is removed from gamemap
    //! If false is returned, the listener will be removed from the listener list
    virtual bool notifyRemovedFromGameMap(GameEntity* entity) = 0;

    //! \brief Called when a GameEntity we asked to be notified of is pickedup
    //! If false is returned, the listener will be removed from the listener list
    virtual bool notifyPickedUp(GameEntity* entity) = 0;

    //! \brief Called when a GameEntity we asked to be notified of is dropped
    //! If false is returned, the listener will be removed from the listener list
    virtual bool notifyDropped(GameEntity* entity) = 0;
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
          bool            isOnServerMap,
          std::string     name       = std::string(),
          std::string     meshName   = std::string(),
          Seat*           seat        = nullptr
          );

    virtual ~GameEntity() {}

    // ===== GETTERS =====

    //! \brief Get if the object can be attacked or not
    virtual bool isAttackable(Tile* tile, Seat* seat) const
    { return false; }

    //! \brief Pointer to the GameMap
    inline GameMap* getGameMap() const
    { return mGameMap; }

    inline bool getIsOnServerMap() const
    { return mIsOnServerMap; }

    // ===== METHODS =====
    //! \brief Function that schedules the object destruction. This function should not be called twice
    void deleteYourself();

    //! \brief Retrieves the position tile from the game map
    Tile* getPositionTile() const;

    //! \brief defines what happens on each turn with this object on server side
    virtual void doUpkeep() = 0;

    //! \brief defines what happens on each turn with this object on client side. Note
    //! that they need to register to GameMap::addClientUpkeepEntity
    virtual void clientUpkeep();

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
    virtual double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage,
        bool ignorePhysicalDefense, bool ignoreMagicalDefense) = 0;

    //! \brief Adds the entity to the correct spaces of the gamemap (animated objects, creature, ...)
    virtual void addToGameMap() = 0;
    virtual void removeFromGameMap() = 0;

    inline bool getIsOnMap() const
    { return mIsOnMap; }

    inline void setIsOnMap(bool isOnMap)
    { mIsOnMap = isOnMap; }

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
    virtual EntityCarryType getEntityCarryType(Creature* carrier)
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
    virtual void restoreEntityState();

    /*! This function should be called on server side on entities loaded from level files and only them. It will
     * be called after all entities are created and added to the gamemap on all of them to allow to restore
     * links between entities (like beds and creatures). It will be called on Creature, Room, Trap, RenderedMovableEntity
     * and Spell
     */
    virtual void restoreInitialEntityState()
    {}

    static std::string getGameEntityStreamFormat();

    virtual void destroyMeshLocal();

    //! Used on client side to correct drop position to avoid all dropped entities
    //! to be on the exact same position (center of the tile)
    virtual void correctDropPosition(Ogre::Vector3& position)
    {}

    //! \brief Called while moving the entity to add it to the tile it gets on
    virtual void addEntityToPositionTile();
    //! \brief Called while moving the entity to remove it from the tile it gets off
    virtual void removeEntityFromPositionTile();

    void addGameEntityListener(GameEntityListener* listener);
    void removeGameEntityListener(GameEntityListener* listener);

    static void exportToStream(GameEntity* entity, std::ostream& os);

  protected:
    virtual void exportHeadersToStream(std::ostream& os) const override;
    virtual void exportHeadersToPacket(ODPacket& os) const override;
    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;
    virtual void exportToPacket(ODPacket& os, const Seat* seat) const override;
    virtual void importFromPacket(ODPacket& is) override;

    //! \brief Fires a add entity message to the player of the given seat
    virtual void fireAddEntity(Seat* seat, bool async) = 0;
    //! \brief Fires a remove creature message to the player of the given seat (if not null). If null, it fires to
    //! all players with vision
    virtual void fireRemoveEntity(Seat* seat) = 0;
    std::vector<Seat*> mSeatsWithVisionNotified;

    //! List of particle effects affecting this entity. Note that the particle effects are not saved on the entity automatically
    //! when exporting to stream or packet because some might build them alone and saving them would break level and saved
    //! games compatibility. If it becomes useful later, it can be done.
    std::vector<EntityParticleEffect*> mEntityParticleEffects;

    //! Constructs a particle system based on this entity name
    std::string nextParticleSystemsName();

    void fireEntityDead();

    void fireEntityRemoveFromGameMap();

  private:

    //! \brief Pointer to the GameMap object.
    GameMap* mGameMap;

    //! \brief Whether the entity is on map or not (for example, when it is
    //! picked up, it is not on map)
    bool mIsOnMap;

    //! Unique number allowing to have unique names for particle systems attached to this creature
    uint32_t mParticleSystemsNumber;

    const bool mIsOnServerMap;

    //! \brief List of the entity listening for events (removed from gamemap, picked up, ...) on this game entity
    std::vector<GameEntityListener*> mGameEntityListeners;
};

#endif // GAMEENTITY_H
