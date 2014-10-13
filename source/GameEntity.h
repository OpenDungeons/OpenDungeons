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

#include "RenderManager.h"
#include "RenderRequest.h"

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

/* TODO list:
 * - complete the constructor
 * - add semaphores if/where needed
 * - static removeDeadObjects should not be in here (maybe in GameMap?)
 */

  public:
    enum ObjectType
    {
        unknown, creature, room, trap, weapon, roomobject, missileobject, tile
    };

    //! \brief Default constructor with default values
    GameEntity(
          GameMap*        gameMap,
          std::string     nName       = std::string(),
          std::string     nMeshName   = std::string(),
          Seat*           seat        = NULL
          ) :
    pSN         (NULL),
    position    (Ogre::Vector3(0, 0, 0)),
    name        (nName),
    meshName    (nMeshName),
    meshExists  (false),
    mSeat       (seat),
    mIsDeleteRequested (false),
    objectType  (unknown),
    gameMap     (NULL)
    {
        assert(gameMap !=  NULL);
        this->gameMap = gameMap;
    }

    virtual ~GameEntity(){}

    // ===== GETTERS =====
    virtual std::string getOgreNamePrefix() const = 0;

    //! \brief Get the name of the object
    inline const std::string&   getName         () const    { return name; }

    //! \brief Get the mesh name of the object
    inline const std::string&   getMeshName     () const    { return meshName; }

    //! \brief Get the seat that the object belongs to
    inline Seat*                getSeat         () const    { return mSeat; }

    //! \brief Get if the mesh is already existing
    inline bool                 isMeshExisting  () const    { return meshExists; }

    //! \brief Get if the object can be attacked or not
    virtual bool                isAttackable    () const    { return false; }

    //! \brief Get the type of this object
    inline ObjectType           getObjectType   () const    { return objectType; }

    //! \brief Pointer to the GameMap
    inline GameMap*             getGameMap      () const    { return gameMap; }

    virtual Ogre::Vector3       getPosition     () const
    {
        Ogre::Vector3 tempVector = position;
        return tempVector;
    }

    // ===== SETTERS =====
    //! \brief Set the name of the entity
    inline void setName         (const std::string& nName)      { name = nName; }

    //! \brief Set the name of the mesh file
    inline void setMeshName     (const std::string& nMeshName)  { meshName = nMeshName; }

    //! \brief Sets the seat this object belongs to
    inline void setSeat         (Seat* seat)                    { mSeat = seat; }

    //! \brief Set if the mesh exists
    inline void setMeshExisting (bool isExisting)               { meshExists = isExisting; }

    //! \brief Set the type of the object. Should be done in all final derived constructors
    inline void setObjectType   (ObjectType nType)              { objectType = nType; }

    virtual void                setPosition                     (const Ogre::Vector3& v)
    {
        position = v;
    }

    // ===== METHODS =====
    //! \brief Function that calls the mesh creation. If the mesh is already created, does nothing
    void    createMesh      ();
    //! \brief Function that calls the mesh destruction. If the mesh is not created, does nothing
    void    destroyMesh     ();
    //! \brief Function that schedules the object destruction. This function should not be called twice
    void    deleteYourself  ();

    inline void show()
    {
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::attachTile;
        request->p = static_cast<void*>(this);

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    };

    inline void hide()
    {
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::detachTile;
        request->p = static_cast<void*>(this);

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    };

    //! \brief defines what happens on each turn with this object
    virtual void    doUpkeep        () = 0;

    //! \brief Returns a list of the tiles that this object is in/covering.  For creatures and other small objects
    //! this will be a single tile, for larger objects like rooms this will be 1 or more tiles.
    virtual std::vector<Tile*> getCoveredTiles() = 0;

    //! \brief Returns the HP associated with the given tile of the object, it is up to the object how they want to treat the tile/HP relationship.
    virtual double getHP(Tile *tile) const = 0;

    //! \brief Returns defense rating for the object, i.e. how much less than inflicted damage should it recieve.
    virtual double getDefense() const = 0;

    //! \brief Subtracts the given number of hitpoints from the object, the tile specifies where
    //! the enemy inflicted the damage and the object can use this accordingly. attacker is
    //! the entity damaging
    virtual void takeDamage(GameEntity* attacker, double damage, Tile *tileTakingDamage) = 0;

    Ogre::SceneNode* pSN;

    static std::vector<GameEntity*> removeDeadObjects(const std::vector<GameEntity*> &objects)
    {
        std::vector<GameEntity*> ret;
        for(unsigned int i = 0, size = objects.size(); i < size; ++i)
        {
            if (objects[i]->getHP(NULL) > 0.0)
            ret.push_back(objects[i]);
        }

        return ret;
    }

    friend ODPacket& operator<<(ODPacket& os, const GameEntity::ObjectType& ot);
    friend ODPacket& operator>>(ODPacket& is, GameEntity::ObjectType& ot);

  protected:
    //! \brief Function that implements the mesh creation
    virtual void    createMeshLocal      () {};

    //! \brief Function that implements the mesh deletion
    virtual void    destroyMeshLocal     () {};

    //! \brief Function that implements the mesh deletion
    virtual void    deleteYourselfLocal  () {};

    //! \brief The position of this object
    Ogre::Vector3   position;

    //! \brief Convinience function to get ogreNamePrefix + name
    //! Used for nodes. The name does not include the _node and similar postfixes which are
    //! added in RenderManager.
    std::string getNodeNameWithoutPostfix();

  private:
    //! brief The name of the entity
    std::string     name;

    //! \brief The name of the mesh
    std::string     meshName;

    //! \brief Stores the existence state of the mesh
    bool            meshExists;

    //! \brief The seat that the object belongs to
    Seat*           mSeat;

    //! \brief A flag saying whether the object has been requested to delete
    bool            mIsDeleteRequested;

    //! \brief What kind of object is it
    ObjectType      objectType;

    //! \brief Pointer to the GameMap object.
    GameMap*        gameMap;
};

#endif /* GAMEENTITY_H_ */

