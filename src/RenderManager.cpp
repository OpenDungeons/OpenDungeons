/*
 * RenderManager.cpp
 *
 *  Created on: 26. mars 2011
 *      Author: oln
 */

#include "RenderManager.h"

#include <semaphore.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include "GameMap.h"
#include "RenderRequest.h"
#include "Functions.h"
#include "Defines.h"
#include "Room.h"
#include "RoomObject.h"

#include "Globals.h"


template<> RenderManager * Ogre::Singleton<RenderManager>::ms_Singleton = 0;

/*! \brief Returns a reference to the singleton object of SoundEffectsHelper.
 *
 */
RenderManager& RenderManager::getSingleton()
{
    assert ( ms_Singleton );
    return ( *ms_Singleton );
}

/*! \brief Returns a pointer to the singleton object of SoundEffectsHelper.
 *
 */
RenderManager* RenderManager::getSingletonPtr()
{
    return ms_Singleton;
}

RenderManager::RenderManager() :
        initialized ( false ),
        roomSceneNode(NULL),
        creatureSceneNode(NULL),
        lightSceneNode(NULL)
{
    // TODO Auto-generated constructor stub

}

RenderManager::~RenderManager()
{
    // TODO Auto-generated destructor stub
}

bool RenderManager::initialize ( sem_t* renderQueueSemaphore, std::deque <
                                 RenderRequest* > * renderQueue, Ogre::SceneManager* sceneManager,
                                 Ogre::SceneNode roomSceneNode, Ogre::SceneNode creatureSceneNode,
                                 GameMap* gameMap )
{
    this->renderQueueSemaphore = renderQueueSemaphore;
    this->renderQueue = renderQueue;
    this->sceneManager = sceneManager;
    this->roomSceneNode = roomSceneNode;
    this->creatureSceneNode = creatureSceneNode;
    this->gameMap = gameMap;

    initialized = false;
    return false;
}

bool RenderManager::handleRenderRequest ( const RenderRequest& renderRequest )
{
    switch ( renderRequest.type )
    {

    }
}

bool RenderManager::processRenderRequests()
{
    while ( true )
    {
        // Remove the first item from the render queue
        sem_wait ( renderQueueSemaphore );

        // Verify that the renderQueue still contains items, this can happen because the check at the top
        // of the loop is not semaphore protected and is therefore subject to a race condition.
        RenderRequest *curReq = NULL;

        if ( renderQueue.size() == 0 )
        {
            // If the renderQueue now contains 0 objects we should process this object and then
            // release any of the other threads which were waiting on a renderQueue flush.
            //FIXME: Noting is actually being done based on this, this should be used to implement
            //a function making it easy to allow functions to wait on this.
            sem_post ( renderQueueSemaphore );
            break;
        }
        else
        {
            curReq = renderQueue.front();
            renderQueue.pop_front();
            sem_post ( renderQueueSemaphore );
        }

        // Handle the request
        handleRenderRequest ( *curReq );

        // Decrement the number of outstanding references to things from the turn number the event was queued on.
        gameMap.threadUnlockForTurn ( curReq->turnNumber );

        delete curReq;

        curReq = NULL;

        // If we have finished processing the last renderRequest that was in the queue we
        // can release all of the threads that were waiting for the queue to be flushed.
        unsigned int numThreadsWaiting =
            numThreadsWaitingOnRenderQueueEmpty.get();

        for ( unsigned int i = 0; i < numThreadsWaiting; ++i )
        {
            sem_post ( renderQueueEmptySemaphore );
        }
    }
}

void RenderManager::rrRefreshTile ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );

    if ( sceneManager->hasSceneNode ( curTile->name + "_node" ) )
    {

        // Unlink and delete the old mesh
        sceneManager->getSceneNode ( curTile->name + "_node" )->detachObject (
            curTile->name );
        sceneManager->destroyEntity ( curTile->name );

        Ogre::Entity* ent = sceneManager->createEntity ( curTile->name,
                            Tile::meshNameFromFullness(curTile->getType(),
                                                       curTile->getFullnessMeshNumber()) );
        colourizeEntity ( ent, curTile->getColor() );

        // Link the tile mesh back to the relevant scene node so OGRE will render it
        Ogre::SceneNode* node = sceneManager->getSceneNode ( curTile->name + "_node" );
        node->attachObject ( ent );
        node->resetOrientation();
        node->roll ( Ogre::Degree ( curTile->rotation ) );
    }
}

void RenderManager::rrCreateTile ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    
    Ogre::Entity* ent = sceneManager->createEntity ( curTile->name,
                            Tile::meshNameFromFullness(curTile->getType(),
                                                       curTile->getFullnessMeshNumber()) );
    colourizeEntity ( ent, curTile->getColor() );
    Ogre::SceneNode* node = sceneManager->getRootSceneNode()->createChildSceneNode (
                                curTile->name + "_node" );
    //node->setPosition(Ogre::Vector3(x/BLENDER_UNITS_PER_OGRE_UNIT, y/BLENDER_UNITS_PER_OGRE_UNIT, 0));
    node->attachObject ( ent );
    node->setPosition ( Ogre::Vector3 ( curTile->x, curTile->y, 0 ) );
    node->setScale ( Ogre::Vector3 ( BLENDER_UNITS_PER_OGRE_UNIT,
                                     BLENDER_UNITS_PER_OGRE_UNIT,
                                     BLENDER_UNITS_PER_OGRE_UNIT ) );
    node->resetOrientation();
    node->roll ( Ogre::Degree ( curTile->rotation ) );
}

void RenderManager::rrDestroyTile ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );

    if ( sceneManager->hasEntity ( curTile->name ) )
    {
        Ogre::Entity ent = sceneManager->getEntity ( curTile->name );
        Ogre::SceneNode* node = sceneManager->getSceneNode ( curTile->name + "_node" );
        node->detachAllObjects();
        sceneManager->destroySceneNode ( curTile->name + "_node" );
        sceneManager->destroyEntity ( ent );
    }
}

void RenderManager::rrCreateRoom ( const RenderRequest& renderRequest )
{
    Room* curRoom = static_cast<Room*> ( renderRequest.p );
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );

    std::stringstream tempSS;
    tempSS << curRoom->getName() << "_" << curTile->x << "_"
    << curTile->y;
    Ogre::Entity ent = sceneManager->createEntity ( tempSS.str(), curRoom->meshName
                       + ".mesh" );
    colourizeEntity ( ent, curRoom->color );
    Ogre::SceneNode* node = roomSceneNode->createChildSceneNode ( tempSS.str()
                            + "_node" );
    node->setPosition ( Ogre::Vector3 ( curTile->x, curTile->y, 0.0 ) );
    node->setScale ( Ogre::Vector3 ( BLENDER_UNITS_PER_OGRE_UNIT,
                                     BLENDER_UNITS_PER_OGRE_UNIT,
                                     BLENDER_UNITS_PER_OGRE_UNIT ) );
    node->attachObject ( ent );
}

void RenderManager::rrDestroyRoom ( const RenderRequest& renderRequest )
{
    Room* curRoom = static_cast<Room*> ( renderRequest.p );
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );
    std::stringstream tempSS;
    tempSS << curRoom->getName() << "_" << curTile->x << "_"
    << curTile->y;
    if (sceneManager->hasEntity(tempSS.str()))
    {
        Ogre::Entity ent = sceneManager->getEntity(tempSS.str());
        Ogre::SceneNode* node = sceneManager->getSceneNode(tempSS.str() + "_node");
        node->detachObject(ent);
        roomSceneNode->removeChild(node);
        sceneManager->destroyEntity(ent);
        sceneManager->destroySceneNode(tempSS.str() + "_node");
    }
}

void RenderManager::rrCreateRoomObject ( const RenderRequest& renderRequest )
{
    RoomObject* curRoomObject = static_cast<RoomObject*> (renderRequest.p);
    Room* curRoom = static_cast<Room*> ( renderRequest.p2 );

    std::string tempString = curRoomObject->getOgreNamePrefix()
                             + curRoomObject->getName();
    Ogre::Entity* ent = sceneManager->createEntity(tempString,
                        curRoomObject->getMeshName() + ".mesh");
    colourizeEntity(ent, curRoomObject->getParentRoom()->color);
    Ogre::SceneNode* node
    = roomSceneNode->createChildSceneNode(tempString
                                          + "_node");
    node->setPosition(Ogre::Vector3(curRoomObject->x,
                                    curRoomObject->y, 0.0));
    node->roll(Ogre::Degree(curRoomObject->rotationAngle));

    node->attachObject(ent);
}

void RenderManager::rrDestroyRoomObject ( const RenderRequest& renderRequest )
{
    RoomObject* curRoomObject = static_cast<RoomObject*> (renderRequest.p);
    Room* curRoom = static_cast<Room*> ( renderRequest.p2 );

    std::string tempString = curRoomObject->getOgreNamePrefix()
                                        + curRoomObject->getName();
    Ogre::Entity* ent = sceneManager->getEntity(tempString);
    Ogre::SceneNode* node = sceneManager->getSceneNode(tempString + "_node");
    node->detachObject(ent);
    sceneManager->destroySceneNode(node->getName());
    sceneManager->destroyEntity(ent);

}

void RenderManager::rrCreateTrap ( const RenderRequest& renderRequest )
{
    Trap* curTrap = static_cast<Trap*>( renderRequest.p);
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );

    std::stringstream tempSS;
    tempSS << "Trap_" << curTrap->getName() + "_tile_"
    << curTile->x << "_" << curTile->y;
    std::string tempString = tempSS.str();
    Ogre::Entity* ent = sceneManager->createEntity(tempString,
                        curTrap->getMeshName() + ".mesh");
    Ogre::SceneNode* node
    = roomSceneNode->createChildSceneNode(tempString
                                          + "_node");
    node->setPosition(Ogre::Vector3(curTile->x, curTile->y, 0.0));
    node->attachObject(ent);
}

void RenderManager::rrDestroyTrap ( const RenderRequest& renderRequest )
{
    Trap* curTrap = static_cast<Trap*>( renderRequest.p);
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );

    std::stringstream tempSS;
                tempSS << "Trap_" << curTrap->getName() + "_tile_"
                        << curTile->x << "_" << curTile->y;
                std::string tempString = tempSS.str();
                Ogre::Entity* ent = sceneManager->getEntity(tempString);
                Ogre::SceneNode* node = sceneManager->getSceneNode(tempString + "_node");
                node->detachObject(ent);
                sceneManager->destroySceneNode(node->getName());
                sceneManager->destroyEntity(ent);
}

void RenderManager::rrCreateTreasuryIndicator ( const RenderRequest& renderRequest )
{
        Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    Room* curRoom = static_cast<Room*> ( renderRequest.p2 );
    std::stringstream tempSS;

                tempSS << curRoom->getName() << "_" << curTile->x << "_"
                        << curTile->y;
                Ogre::Entity* ent = sceneManager->createEntity(tempSS.str()
                        + "_treasury_indicator", renderRequest.str + ".mesh");
                Ogre::SceneNode* node = sceneManager->getSceneNode(tempSS.str() + "_node");

                //FIXME: This second scene node is purely to cancel out the effects of BLENDER_UNITS_PER_OGRE_UNIT, it can be gotten rid of when that hack is fixed.
                node = node->createChildSceneNode(node->getName()
                        + "_hack_node");
                node->setScale(Ogre::Vector3(1.0 / BLENDER_UNITS_PER_OGRE_UNIT,
                        1.0 / BLENDER_UNITS_PER_OGRE_UNIT, 1.0
                                / BLENDER_UNITS_PER_OGRE_UNIT));

                node->attachObject(ent);
}

void RenderManager::rrDestroyTreasuryIndicator ( const RenderRequest& renderRequest )
{
        Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    Room* curRoom = static_cast<Room*> ( renderRequest.p2 );

    std::stringstream tempSS;
                tempSS << curRoom->getName() << "_" << curTile->x << "_"
                        << curTile->y;
                if (sceneManager->hasEntity(tempSS.str() + "_treasury_indicator"))
                {
                    Ogre::Entity* ent = sceneManager->getEntity(tempSS.str()
                            + "_treasury_indicator");

                    //FIXME: This second scene node is purely to cancel out the effects of BLENDER_UNITS_PER_OGRE_UNIT, it can be gotten rid of when that hack is fixed.
                    Ogre::SceneNode* node = sceneManager->getSceneNode(tempSS.str() + "_node"
                            + "_hack_node");

                    /*  The proper code once the above hack is fixed.
                     node = mSceneMgr->getSceneNode(tempSS.str() + "_node");
                     */
                    node->detachObject(ent);

                    //FIXME: This line is not needed once the above hack is fixed.
                    sceneManager->destroySceneNode(node->getName());

                    sceneManager->destroyEntity(ent);
                }
}

void RenderManager::rrCreateCreature ( const RenderRequest& renderRequest )
{
                 Creature* curCreature = static_cast<Creature*>(renderRequest.p);

                // Load the mesh for the creature
                Ogre::Entity* ent = sceneManager->createEntity("Creature_" + curCreature->name,
                        curCreature->meshName);
                colourizeEntity(ent, curCreature->color);
                Ogre::SceneNode* node = creatureSceneNode->createChildSceneNode(
                        curCreature->name + "_node");
                curCreature->sceneNode = node;
                node->setPosition(curCreature->getPosition());
                node->setScale(curCreature->scale);

                node->attachObject(ent);
}

void RenderManager::rrDestroyCreature ( const RenderRequest& renderRequest )
{
                Creature* curCreature = static_cast<Creature*>(renderRequest.p);
                if (sceneManager->hasEntity("Creature_" + curCreature->name))
                {
                    Ogre::Entity* ent = sceneManager->getEntity("Creature_" + curCreature->name);
                    Ogre::SceneNode node = sceneManager->getSceneNode(curCreature->name + "_node");
                    node->detachObject(ent);
                    creatureSceneNode->removeChild(node);
                    sceneManager->destroyEntity(ent);
                    sceneManager->destroySceneNode(curCreature->name + "_node");
                }
                curCreature->sceneNode = NULL;
}

void RenderManager::rrOrientSceneNodeToward ( const RenderRequest& renderRequest )
{
                Ogre::SceneNode node = sceneManager->getSceneNode(renderRequest.str);
                Ogre::Vector3 tempVector = node->getOrientation()
                        * Ogre::Vector3::NEGATIVE_UNIT_Y;

                // Work around 180 degree quaternion rotation quirk
                if ((1.0f + tempVector.dotProduct(renderRequest.vec)) < 0.0001f)
                {
                    node->roll(Degree(180));
                }
                else
                {
                    node->rotate(tempVector.getRotationTo(renderRequest.vec));
                }
}

void RenderManager::rrReorientSceneNode ( const RenderRequest& renderRequest )
{
    Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(renderRequest.p);

    if (node != NULL)
    {
        node->rotate(renderRequest.quaternion);
    }
}

void RenderManager::rrScaleSceneNode( const RenderRequest& renderRequest )
{
                Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(renderRequest.p);

                if (node != NULL)
                {
                    node->scale(renderRequest.p);
                }
}

void RenderManager::rrCreateWeapon ( const RenderRequest& renderRequest )
{
                Weapon* curWeapon = static_cast<Weapon*>( renderRequest.p);
                Creature* curCreature = static_cast<Creature*>(renderRequest.p2);

                Ogre::Entity* ent = sceneManager->getEntity("Creature_" + curCreature->name);
                colourizeEntity(ent, curCreature->color);
                Ogre::Entity* weaponEntity = sceneManager->createEntity("Weapon_"
                        + curWeapon->handString + "_" + curCreature->name,
                        curWeapon->meshName);
                Ogre::Bone weaponBone = ent->getSkeleton()->getBone(
                    "Weapon_" + curWeapon->handString);

                // Rotate by -90 degrees around the x-axis from the bone's rotation.
                Ogre::Quaternion rotationQuaternion;
                rotationQuaternion.FromAngleAxis(Degree(-90.0), Ogre::Vector3(1.0,
                        0.0, 0.0));

                ent->attachObjectToBone(weaponBone->getName(), weaponEntity,
                        rotationQuaternion);
}

void RenderManager::rrDestroyWeapon ( const RenderRequest& renderRequest )
{
                Weapon* curWeapon = static_cast<Weapon*>( renderRequest.p);
                Creature* curCreature = static_cast<Creature*>(renderRequest.p2);

                if (curWeapon->name.compare("none") != 0)
                {
                    Ogre::Entity* ent = sceneManager->getEntity("Weapon_"
                            + curWeapon->handString + "_" + curCreature->name);
                    sceneManager->destroyEntity(ent);
                }
}

void RenderManager::rrCreateMissileObject ( const RenderRequest& renderRequest )
{
                MissileObject* curMissileObject = static_cast<MissileObject*>(renderRequest.p);
                Ogre::Entity ent = sceneManager->createEntity(curMissileObject->name,
                        curMissileObject->meshName + ".mesh");
                //TODO:  Make a new subroot scene node for these so lookups are faster since only a few missile objects should be onscreen at once.
                Ogre::SceneNode node = creatureSceneNode->createChildSceneNode(
                        curMissileObject->name + "_node");
                node->setPosition(curMissileObject->getPosition());
                node->attachObject(ent);
}

void RenderManager::rrDestroyMissileObject ( const RenderRequest& renderRequest )
{
                    MissileObject* curMissileObject = static_cast<MissileObject*>(renderRequest.p);
                if (sceneManager->hasEntity(curMissileObject->name))
                {
                    Ogre::Entity ent = sceneManager->getEntity(curMissileObject->name);
                    Ogre::SceneNode node = sceneManager->getSceneNode(curMissileObject->name
                            + "_node");
                    node->detachObject(ent);
                    creatureSceneNode->removeChild(node);
                    sceneManager->destroyEntity(ent);
                    sceneManager->destroySceneNode(curMissileObject->name
                            + "_node");
                }
}

void RenderManager::rrCreateMapLight ( const RenderRequest& renderRequest )
{
                MapLight* curMapLight = static_cast<MapLight*> (renderRequest.p);

                // Create the light and attach it to the lightSceneNode.
                std::string mapLightName = "MapLight_" + curMapLight->getName();
                Ogre::Light* light = sceneManager->createLight(mapLightName);
                light->setDiffuseColour(curMapLight->getDiffuseColor());
                light->setSpecularColour(curMapLight->getSpecularColor());
                light->setAttenuation(curMapLight->getAttenuationRange(),
                        curMapLight->getAttenuationConstant(),
                        curMapLight->getAttenuationLinear(),
                        curMapLight->getAttenuationQuadratic());

                // Create the base node that the "flicker_node" and the mesh attach to.
                Ogre::SceneNode* mapLightNode = lightSceneNode->createChildSceneNode(mapLightName
                        + "_node");
                mapLightNode->setPosition(curMapLight->getPosition());

                //TODO - put this in request so we don't have to include the globals here.
                if (serverSocket == NULL && clientSocket == NULL)
                {
                    // Create the MapLightIndicator mesh so the light can be drug around in the map editor.
                    Ogre::Entity* lightEntity = sceneManager->createEntity("MapLightIndicator_"
                            + curMapLight->getName(), "Light.mesh");
                    mapLightNode->attachObject(lightEntity);
                }

                // Create the "flicker_node" which moves around randomly relative to
                // the base node.  This node carries the light itself.
                Ogre::SceneNode* flickerNode
                        = mapLightNode->createChildSceneNode(mapLightName
                                + "_flicker_node");
                flickerNode->attachObject(light);
}

void RenderManager::rrDestroyMapLight ( const RenderRequest& renderRequest )
{
                MapLight* curMapLight = static_cast<MapLight*> (renderRequest.p);
                std::string mapLightName = "MapLight_" + curMapLight->getName();
                if (mSceneMgr->hasLight(mapLightName))
                {
                    Ogre::Light* light = mSceneMgr->getLight(mapLightName);
                    Ogre::SceneNode* lightNode = mSceneMgr->getSceneNode(mapLightName + "_node");
                    Ogre::SceneNode* lightFlickerNode = mSceneMgr->getSceneNode(mapLightName
                            + "_flicker_node");
                    lightFlickerNode->detachObject(light);
                    lightSceneNode->removeChild(lightNode);
                    mSceneMgr->destroyLight(light);
                    
                    if (mSceneMgr->hasEntity(mapLightName))
                    {
                        Ogre::Entity* ent = mSceneMgr->getEntity("MapLightIndicator_"
                            + curMapLight->getName());
                        lightNode->detachObject(ent);
                    }
                    mSceneMgr->destroySceneNode(lightFlickerNode->getName());
                    mSceneMgr->destroySceneNode(lightNode->getName());
                }
}

void RenderManager::rrDestroyMapLightVisualIndicator ( const RenderRequest& renderRequest )
{
    
}

void RenderManager::rrCreateField ( const RenderRequest& renderRequest ) {}

void RenderManager::rrRefreshField ( const RenderRequest& renderRequest ) {}

void RenderManager::rrPickUpCreature ( const RenderRequest& renderRequest ) {}

void RenderManager::rrDropCreature ( const RenderRequest& renderRequest ) {}

void RenderManager::rrRotateCreaturesInHand ( const RenderRequest& renderRequest ) {}

void RenderManager::rrCreateCreatureVisualDebug ( const RenderRequest& renderRequest ) {}

void RenderManager::rrDestroyCreatureVisualDebug ( const RenderRequest& renderRequest ) {}

void RenderManager::rrSetObjectAnimationState ( const RenderRequest& renderRequest ) {}
void RenderManager::rrMoveSceneNode ( const RenderRequest& renderRequest ) {}
