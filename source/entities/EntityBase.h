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

#ifndef ENTITYBASE_H
#define ENTITYBASE_H

#include "entities/GameEntityType.h"

#include <OgreVector3.h>

#include <string>

class Seat;
namespace Ogre
{
    class SceneNode;
}

class EntityBase
{
public:
    EntityBase(std::string name, std::string meshName, Seat* seat = nullptr);

    virtual ~EntityBase()
    {}

    std::string getOgreNamePrefix() const;

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

    const Ogre::Vector3& getPosition() const
    { return mPosition; }

    inline Ogre::SceneNode* getParentSceneNode() const
    { return mParentSceneNode; }

    inline Ogre::SceneNode* getEntityNode() const
    { return mEntityNode; }

    virtual const Ogre::Vector3& getScale() const
    { return Ogre::Vector3::UNIT_SCALE; }

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

    //! \brief Set the new entity position. If isMove is true, that means that the entity was
    //! already on map and is moving. If false, it means that the entity was not on map (for example
    //! if being dropped or created).
    virtual void setPosition(const Ogre::Vector3& v, bool isMove)
    { mPosition = v; }

    inline void setParentSceneNode(Ogre::SceneNode* sceneNode)
    { mParentSceneNode = sceneNode; }

    inline void setEntityNode(Ogre::SceneNode* sceneNode)
    { mEntityNode = sceneNode; }

    //! \brief Function that calls the mesh creation. If the mesh is already created, does nothing
    void createMesh();
    //! \brief Function that calls the mesh destruction. If the mesh is not created, does nothing
    void destroyMesh();

    //! \brief Get the type of this object
    virtual GameEntityType getObjectType() const = 0;

    //! \brief Called when the stat windows is displayed
    virtual bool canDisplayStatsWindow(Seat* seat)
    { return false; }

    virtual void createStatsWindow()
    {}

    //! \brief Called when the entity is being slapped
    virtual bool canSlap(Seat* seat)
    { return false; }
    virtual void slap()
    {}

    //! \brief Called when the entity is being picked up /dropped
    virtual bool tryPickup(Seat* seat)
    { return false; }
    virtual void pickup()
    {}
protected:
    //! \brief Function that implements the mesh creation
    virtual void createMeshLocal() {};

    //! \brief Function that implements the mesh deletion
    virtual void destroyMeshLocal() {};

    //! \brief The position of this object
    Ogre::Vector3 mPosition;

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

    //! Used by the renderer to save the scene node this entity belongs to. This is useful
    //! when the entity is removed from the scene (during pickup for example)
    Ogre::SceneNode* mParentSceneNode;

    //! Used by the renderer to save this entity's node
    Ogre::SceneNode* mEntityNode;
private:
    EntityBase(const EntityBase& other) = delete;
    EntityBase& operator= (const EntityBase& other) = delete;
};

#endif // ENTITYBASE_H
