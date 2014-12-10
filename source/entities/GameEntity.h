/*!
 * \file   GameEntity.h
 * \date:  16 September 2011
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 *
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#ifndef GAMEENTITY_H_
#define GAMEENTITY_H_

#include "render/RenderManager.h"
#include "render/RenderRequest.h"

#include <cassert>
#include <string>
#include <vector>
#include <OgreVector3.h>
#include <OgreSceneNode.h>

class GameMap;
class Tile;
class Quadtree;
class Seat;
class ODPacket;

/*! \class GameEntity GameEntity.h
 *  \brief This class holds elements that are common to every object placed in the game
 *
 * Functions and properties that are common to every object should be placed into this class
 * and initialised with a good default value in the default constructor.
 * Member variables are private and only accessed through getters and setters.
 */
class GameEntity
{
  public:
    enum ObjectType
    {
        unknown, creature, room, trap, weapon, renderedMovableEntity, tile
    };

    //! \brief Default constructor with default values
    GameEntity(
          GameMap*        gameMap,
          std::string     name       = std::string(),
          std::string     meshName   = std::string(),
          float           opacity     = 1.0f,
          Seat*           seat        = nullptr
          ) :
    mPosition          (Ogre::Vector3(0, 0, 0)),
    mOpacity           (opacity),
    mName              (name),
    mMeshName          (meshName),
    mMeshExists        (false),
    mSeat              (seat),
    mIsDeleteRequested (false),
    mObjectType        (unknown),
    mGameMap           (gameMap),
    mIsOnMap           (true),
    mRendererSceneNode (nullptr)
    {
        assert(mGameMap != nullptr);
    }

    virtual ~GameEntity() {}

    // ===== GETTERS =====
    virtual std::string getOgreNamePrefix() const = 0;

    //! \brief Get the name of the object
    inline const std::string& getName() const
    { return mName; }

    //! \brief Get the mesh name of the object
    inline const std::string& getMeshName() const
    { return mMeshName; }

    //! \brief Get the seat that the object belongs to
    inline Seat* getSeat() const
    { return mSeat; }

    //! \brief Get if the mesh is already existing
    inline bool isMeshExisting() const
    { return mMeshExists; }

    //! \brief Get if the mesh is already existing
    inline float getOpacity() const
    { return mOpacity; }

    //! \brief Get if the object can be attacked or not
    virtual bool isAttackable() const
    { return false; }

    //! \brief Get the type of this object
    inline ObjectType getObjectType() const
    { return mObjectType; }

    //! \brief Pointer to the GameMap
    inline GameMap* getGameMap() const
    { return mGameMap; }

    virtual Ogre::Vector3 getPosition() const
    { return mPosition; }

    inline Ogre::SceneNode* getSceneNode() const
    { return mRendererSceneNode; }

    // ===== SETTERS =====
    //! \brief Set the name of the entity
    inline void setName(const std::string& name)
    { mName = name; }

    //! \brief Set the name of the mesh file
    inline void setMeshName(const std::string& meshName)
    { mMeshName = meshName; }

    //! \brief Sets the seat this object belongs to
    inline void setSeat(Seat* seat)
    { mSeat = seat; }

    //! \brief Set if the mesh exists
    inline void setMeshExisting(bool isExisting)
    { mMeshExists = isExisting; }

    //! \brief Set the entity opacity
    void setMeshOpacity(float opacity);

    //! \brief Set the entity opacity value whithout refreshing it. Useful at construction.
    void setOpacity(float opacity)
    { mOpacity = opacity; }

    //! \brief Set the type of the object. Should be done in all final derived constructors
    inline void setObjectType (ObjectType type)
    { mObjectType = type; }

    virtual void setPosition(const Ogre::Vector3& v)
    { mPosition = v; }

    inline void setSceneNode(Ogre::SceneNode* sceneNode)
    { mRendererSceneNode = sceneNode; }

    // ===== METHODS =====
    //! \brief Function that calls the mesh creation. If the mesh is already created, does nothing
    void createMesh();
    //! \brief Function that calls the mesh destruction. If the mesh is not created, does nothing
    void destroyMesh();
    //! \brief Function that schedules the object destruction. This function should not be called twice
    void deleteYourself();

    inline void show()
    {
        RenderRequestAttachEntity request(this);
        RenderManager::executeRenderRequest(request);
    }

    inline void hide()
    {
        RenderRequestDetachEntity request(this);
        RenderManager::executeRenderRequest(request);
    }

    //! \brief Retrieves the position tile from the game map
    Tile* getPositionTile() const;

    //! \brief defines what happens on each turn with this object
    virtual void doUpkeep() = 0;

    //! \brief Returns a list of the tiles that this object is in/covering.  For creatures and other small objects
    //! this will be a single tile, for larger objects like rooms this will be 1 or more tiles.
    virtual std::vector<Tile*> getCoveredTiles() = 0;

    //! \brief Returns the HP associated with the given tile of the object, it is up to the object how they want to treat the tile/HP relationship.
    virtual double getHP(Tile *tile) const = 0;

    //! \brief Subtracts the given number of hitpoints from the object, the tile specifies where
    //! the enemy inflicted the damage and the object can use this accordingly. attacker is
    //! the entity damaging
    virtual double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage) = 0;

    //! \brief Called when the entity is being carried
    virtual void notifyEntityCarried(bool isCarried)
    {}

    virtual bool getIsOnMap()
    { return mIsOnMap; }

    virtual void setIsOnMap(bool isOnMap)
    { mIsOnMap = isOnMap; }

    //! \brief Called when the stat windows is displayed
    virtual bool canDisplayStatsWindow(Seat* seat)
    { return false; }
    virtual void createStatsWindow()
    {}

    //! \brief Called when the entity is being slapped
    virtual bool canSlap(Seat* seat, bool isEditorMode)
    { return false; }
    virtual void slap(bool isEditorMode)
    {}

    //! \brief Called when the entity is being picked up /dropped
    virtual bool tryPickup(Seat* seat, bool isEditorMode)
    { return false; }
    virtual void pickup()
    {}
    virtual bool tryDrop(Seat* seat, Tile* tile, bool isEditorMode)
    { return false; }
    virtual void drop(const Ogre::Vector3& v)
    {}

    friend ODPacket& operator<<(ODPacket& os, const GameEntity::ObjectType& ot);
    friend ODPacket& operator>>(ODPacket& is, GameEntity::ObjectType& ot);

  protected:
    //! \brief Function that implements the mesh creation
    virtual void createMeshLocal() {};

    //! \brief Function that implements the mesh deletion
    virtual void destroyMeshLocal() {};

    //! \brief The position of this object
    Ogre::Vector3 mPosition;

    //! \brief The model current opacity
    float mOpacity;

    //! \brief Convinience function to get ogreNamePrefix + name
    //! Used for nodes. The name does not include the _node and similar postfixes which are
    //! added in RenderManager.
    std::string getNodeNameWithoutPostfix();

  private:
    //! brief The name of the entity
    std::string mName;

    //! \brief The name of the mesh
    std::string mMeshName;

    //! \brief Stores the existence state of the mesh
    bool mMeshExists;

    //! \brief The seat that the object belongs to
    Seat* mSeat;

    //! \brief A flag saying whether the object has been requested to delete
    bool mIsDeleteRequested;

    //! \brief What kind of object is it
    ObjectType mObjectType;

    //! \brief Pointer to the GameMap object.
    GameMap* mGameMap;

    //! \brief Whether the entity is on map or not (for example, when it is
    //! picked up, it is not on map)
    bool mIsOnMap;

    //! Used by the renderer to save the scene node this entity belongs to. This is useful
    //! when the entity is removed from the scene (during pickup for example)
    Ogre::SceneNode* mRendererSceneNode;
};

#endif // GAMEENTITY_H_
