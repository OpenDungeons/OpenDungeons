/*
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

#ifndef RENDERREQUEST_H
#define RENDERREQUEST_H

#include "RenderManager.h"

#include <string>
#include <OgreVector3.h>
#include <OgreQuaternion.h>

class Tile;
class GameEntity;
class Building;
class Creature;
class RenderedMovableEntity;
class Weapon;
class MapLight;
class MovableGameEntity;

/*! \brief A data structure to be used for requesting that the OGRE rendering thread perform certain tasks.
 *
 *  This data structure is used filled out with a request and then placed in
 *  the global renderQueue.  The requests are taken out of the queue and
 *  processed by the frameStarted event in the ExampleFrameListener class.
 */
class RenderRequest
{
public:
    RenderRequest() {}
    virtual ~RenderRequest() {}

    virtual void executeRequest(RenderManager* manager) = 0;
protected:
    //Render request functions
    void rrRefreshTile(RenderManager* manager, Tile* curTile)
    { manager->rrRefreshTile(curTile); }
    void rrCreateTile(RenderManager* manager, Tile* curTile)
    { manager->rrCreateTile(curTile); }
    void rrDestroyTile(RenderManager* manager, Tile* curTile)
    { manager->rrDestroyTile(curTile); }
    void rrDetachEntity(RenderManager* manager, GameEntity* curEntity)
    { manager->rrDetachEntity(curEntity); }
    void rrAttachEntity(RenderManager* manager, GameEntity* curEntity)
    { manager->rrAttachEntity(curEntity); }
    void rrTemporalMarkTile(RenderManager* manager, Tile* curTile)
    { manager->rrTemporalMarkTile(curTile); }
    void rrShowSquareSelector(RenderManager* manager, const Ogre::Real& xPos, const Ogre::Real& yPos)
    { manager->rrShowSquareSelector(xPos, yPos); }
    void rrCreateBuilding(RenderManager* manager, Building* curBuilding, Tile* curTile)
    { manager->rrCreateBuilding(curBuilding, curTile); }
    void rrDestroyBuilding(RenderManager* manager, Building* curBuilding, Tile* curTile)
    { manager->rrDestroyBuilding(curBuilding, curTile); }
    void rrCreateRenderedMovableEntity(RenderManager* manager, RenderedMovableEntity* curRenderedMovableEntity)
    { manager->rrCreateRenderedMovableEntity(curRenderedMovableEntity); }
    void rrDestroyRenderedMovableEntity(RenderManager* manager, RenderedMovableEntity* curRenderedMovableEntity)
    { manager->rrDestroyRenderedMovableEntity(curRenderedMovableEntity); }
    void rrCreateCreature(RenderManager* manager, Creature* curCreature)
    { manager->rrCreateCreature(curCreature); }
    void rrDestroyCreature(RenderManager* manager, Creature* curCreature)
    { manager->rrDestroyCreature(curCreature); }
    void rrOrientSceneNodeToward(RenderManager* manager, MovableGameEntity* gameEntity, const Ogre::Vector3& direction)
    { manager->rrOrientSceneNodeToward(gameEntity, direction); }
    void rrScaleSceneNode(RenderManager* manager, Ogre::SceneNode* node, const Ogre::Vector3& scale)
    { manager->rrScaleSceneNode(node, scale); }
    void rrCreateWeapon(RenderManager* manager, Creature* curCreature, const Weapon* curWeapon, const std::string& hand)
    { manager->rrCreateWeapon(curCreature, curWeapon, hand); }
    void rrDestroyWeapon(RenderManager* manager, Creature* curCreature, const Weapon* curWeapon, const std::string& hand)
    { manager->rrDestroyWeapon(curCreature, curWeapon, hand); }
    void rrCreateMapLight(RenderManager* manager, MapLight* curMapLight, bool displayVisual)
    { manager->rrCreateMapLight(curMapLight, displayVisual); }
    void rrDestroyMapLight(RenderManager* manager, MapLight* curMapLight)
    { manager->rrDestroyMapLight(curMapLight); }
    void rrDestroyMapLightVisualIndicator(RenderManager* manager, MapLight* curMapLight)
    { manager->rrDestroyMapLightVisualIndicator(curMapLight); }
    void rrPickUpEntity(RenderManager* manager, GameEntity* curEntity)
    { manager->rrPickUpEntity(curEntity); }
    void rrDropHand(RenderManager* manager, GameEntity* curEntity)
    { manager->rrDropHand(curEntity); }
    void rrRotateHand(RenderManager* manager)
    { manager->rrRotateHand(); }
    void rrCreateCreatureVisualDebug(RenderManager* manager, Creature* curCreature, Tile* curTile)
    { manager->rrCreateCreatureVisualDebug(curCreature, curTile); }
    void rrDestroyCreatureVisualDebug(RenderManager* manager, Creature* curCreature, Tile* curTile)
    { manager->rrDestroyCreatureVisualDebug(curCreature, curTile); }
    void rrSetObjectAnimationState(RenderManager* manager, MovableGameEntity* curAnimatedObject, const std::string& animation, bool loop)
    { manager->rrSetObjectAnimationState(curAnimatedObject, animation, loop); }
    void rrMoveSceneNode(RenderManager* manager, const std::string& sceneNodeName, const Ogre::Vector3& position)
    { manager->rrMoveSceneNode(sceneNodeName, position); }
    void rrCarryEntity(RenderManager* manager, Creature* carrier, GameEntity* carried)
    { manager->rrCarryEntity(carrier, carried); }
    void rrReleaseCarriedEntity(RenderManager* manager, Creature* carrier, GameEntity* carried)
    { manager->rrReleaseCarriedEntity(carrier, carried); }
};

class RenderRequestRefreshTile : public RenderRequest
{
public:
    RenderRequestRefreshTile(Tile* tile) :
        mTile(tile)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrRefreshTile(manager, mTile); }
private:
    Tile* mTile;
};

class RenderRequestCreateTile : public RenderRequest
{
public:
    RenderRequestCreateTile(Tile* tile) :
        mTile(tile)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrCreateTile(manager, mTile); }
private:
    Tile* mTile;
};

class RenderRequestDestroyTile : public RenderRequest
{
public:
    RenderRequestDestroyTile(Tile* tile) :
        mTile(tile)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDestroyTile(manager, mTile); }
private:
    Tile* mTile;
};

class RenderRequestTemporalMarkTile : public RenderRequest
{
public:
    RenderRequestTemporalMarkTile(Tile* tile) :
        mTile(tile)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrTemporalMarkTile(manager, mTile); }
private:
    Tile* mTile;
};

class RenderRequestDetachEntity : public RenderRequest
{
public:
    RenderRequestDetachEntity(GameEntity* entity) :
        mEntity(entity)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDetachEntity(manager, mEntity); }
private:
    GameEntity* mEntity;
};

class RenderRequestAttachEntity : public RenderRequest
{
public:
    RenderRequestAttachEntity(GameEntity* entity) :
        mEntity(entity)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrAttachEntity(manager, mEntity); }
private:
    GameEntity* mEntity;
};

class RenderRequestShowSquareSelector : public RenderRequest
{
public:
    RenderRequestShowSquareSelector(Ogre::Real x, Ogre::Real y) :
        mX(x),
        mY(y)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrShowSquareSelector(manager, mX, mY); }
private:
    Ogre::Real mX;
    Ogre::Real mY;
};

class RenderRequestCreateBuilding : public RenderRequest
{
public:
    RenderRequestCreateBuilding(Building* building, Tile* tile) :
        mBuilding(building),
        mTile(tile)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrCreateBuilding(manager, mBuilding, mTile); }
private:
    Building* mBuilding;
    Tile* mTile;
};

class RenderRequestDestroyBuilding : public RenderRequest
{
public:
    RenderRequestDestroyBuilding(Building* building, Tile* tile) :
        mBuilding(building),
        mTile(tile)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDestroyBuilding(manager, mBuilding, mTile); }
private:
    Building* mBuilding;
    Tile* mTile;
};

class RenderRequestCreateRenderedMovableEntity : public RenderRequest
{
public:
    RenderRequestCreateRenderedMovableEntity(RenderedMovableEntity* renderedMovableEntity) :
        mRenderedMovableEntity(renderedMovableEntity)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrCreateRenderedMovableEntity(manager, mRenderedMovableEntity); }
private:
    RenderedMovableEntity* mRenderedMovableEntity;
};

class RenderRequestDestroyRenderedMovableEntity : public RenderRequest
{
public:
    RenderRequestDestroyRenderedMovableEntity(RenderedMovableEntity* renderedMovableEntity) :
        mRenderedMovableEntity(renderedMovableEntity)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDestroyRenderedMovableEntity(manager, mRenderedMovableEntity); }
private:
    RenderedMovableEntity* mRenderedMovableEntity;
};

class RenderRequestCreateCreature : public RenderRequest
{
public:
    RenderRequestCreateCreature(Creature* creature) :
        mCreature(creature)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrCreateCreature(manager, mCreature); }
private:
    Creature* mCreature;
};

class RenderRequestDestroyCreature : public RenderRequest
{
public:
    RenderRequestDestroyCreature(Creature* creature) :
        mCreature(creature)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDestroyCreature(manager, mCreature); }
private:
    Creature* mCreature;
};

class RenderRequestOrientSceneNodeToward : public RenderRequest
{
public:
    RenderRequestOrientSceneNodeToward(MovableGameEntity* gameEntity, const Ogre::Vector3& direction) :
        mGameEntity(gameEntity),
        mDirection(direction)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrOrientSceneNodeToward(manager, mGameEntity, mDirection); }
private:
    MovableGameEntity* mGameEntity;
    Ogre::Vector3 mDirection;
};

class RenderRequestScaleSceneNode : public RenderRequest
{
public:
    RenderRequestScaleSceneNode(Ogre::SceneNode* node, const Ogre::Vector3& scale) :
        mNode(node),
        mScale(scale)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrScaleSceneNode(manager, mNode, mScale); }
private:
    Ogre::SceneNode* mNode;
    Ogre::Vector3 mScale;
};

class RenderRequestCreateWeapon : public RenderRequest
{
public:
    RenderRequestCreateWeapon(Creature* creature, const Weapon* weapon, const std::string& hand) :
        mCreature(creature),
        mWeapon(weapon),
        mHand(hand)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrCreateWeapon(manager, mCreature, mWeapon, mHand); }
private:
    Creature* mCreature;
    const Weapon* mWeapon;
    std::string mHand;
};

class RenderRequestDestroyWeapon : public RenderRequest
{
public:
    RenderRequestDestroyWeapon(Creature* creature, const Weapon* weapon, const std::string& hand) :
        mCreature(creature),
        mWeapon(weapon),
        mHand(hand)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDestroyWeapon(manager, mCreature, mWeapon, mHand); }
private:
    Creature* mCreature;
    const Weapon* mWeapon;
    std::string mHand;
};

class RenderRequestCreateMapLight : public RenderRequest
{
public:
    RenderRequestCreateMapLight(MapLight* mapLight, bool displayVisual) :
        mMapLight(mapLight),
        mDisplayVisual(displayVisual)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrCreateMapLight(manager, mMapLight, mDisplayVisual); }
private:
    MapLight* mMapLight;
    bool mDisplayVisual;
};

class RenderRequestDestroyMapLight : public RenderRequest
{
public:
    RenderRequestDestroyMapLight(MapLight* mapLight) :
        mMapLight(mapLight)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDestroyMapLight(manager, mMapLight); }
private:
    MapLight* mMapLight;
};

class RenderRequestDestroyMapLightVisualIndicator : public RenderRequest
{
public:
    RenderRequestDestroyMapLightVisualIndicator(MapLight* mapLight) :
        mMapLight(mapLight)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDestroyMapLightVisualIndicator(manager, mMapLight); }
private:
    MapLight* mMapLight;
};

class RenderRequestPickUpEntity : public RenderRequest
{
public:
    RenderRequestPickUpEntity(GameEntity* entity) :
        mEntity(entity)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrPickUpEntity(manager, mEntity); }
private:
    GameEntity* mEntity;
};

class RenderRequestDropHand : public RenderRequest
{
public:
    RenderRequestDropHand(GameEntity* entity) :
        mEntity(entity)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDropHand(manager, mEntity); }
private:
    GameEntity* mEntity;
};

class RenderRequestRotateHand : public RenderRequest
{
public:
    RenderRequestRotateHand()
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrRotateHand(manager); }
};

class RenderRequestCreateCreatureVisualDebug : public RenderRequest
{
public:
    RenderRequestCreateCreatureVisualDebug(Creature* creature, Tile* tile) :
        mCreature(creature),
        mTile(tile)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrCreateCreatureVisualDebug(manager, mCreature, mTile); }
private:
    Creature* mCreature;
    Tile* mTile;
};

class RenderRequestDestroyCreatureVisualDebug : public RenderRequest
{
public:
    RenderRequestDestroyCreatureVisualDebug(Creature* creature, Tile* tile) :
        mCreature(creature),
        mTile(tile)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrDestroyCreatureVisualDebug(manager, mCreature, mTile); }
private:
    Creature* mCreature;
    Tile* mTile;
};

class RenderRequestSetObjectAnimationState : public RenderRequest
{
public:
    RenderRequestSetObjectAnimationState(MovableGameEntity* animatedObject, const std::string& animation, bool loop) :
        mAnimatedObject(animatedObject),
        mAnimation(animation),
        mLoop(loop)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrSetObjectAnimationState(manager, mAnimatedObject, mAnimation, mLoop); }
private:
    MovableGameEntity* mAnimatedObject;
    std::string mAnimation;
    bool mLoop;
};

class RenderRequestMoveSceneNode : public RenderRequest
{
public:
    RenderRequestMoveSceneNode(const std::string& sceneNodeName, const Ogre::Vector3& position) :
        mSceneNodeName(sceneNodeName),
        mPosition(position)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrMoveSceneNode(manager, mSceneNodeName, mPosition); }
private:
    std::string mSceneNodeName;
    Ogre::Vector3 mPosition;
};

class RenderRequestCarryEntity : public RenderRequest
{
public:
    RenderRequestCarryEntity(Creature* carrier, GameEntity* carried) :
        mCarrier(carrier),
        mCarried(carried)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrCarryEntity(manager, mCarrier, mCarried); }
private:
    Creature* mCarrier;
    GameEntity* mCarried;
};

class RenderRequestReleaseCarriedEntity : public RenderRequest
{
public:
    RenderRequestReleaseCarriedEntity(Creature* carrier, GameEntity* carried) :
        mCarrier(carrier),
        mCarried(carried)
    {}
    virtual void executeRequest(RenderManager* manager)
    { rrReleaseCarriedEntity(manager, mCarrier, mCarried); }
private:
    Creature* mCarrier;
    GameEntity* mCarried;
};

#endif // RENDERREQUEST_H
