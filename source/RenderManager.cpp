/*!
 *  \file   RenderManager.cpp
 *  \date   26 March 2001
 *  \author oln
 *  \brief  handles the render requests
 */

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
//#include <RTShaderSystem/OgreShaderGenerator.h>
#include <RTShaderSystem/OgreShaderExPerPixelLighting.h>
#include <RTShaderSystem/OgreShaderExNormalMapLighting.h>

#include "Globals.h"
#include "GameMap.h"
#include "RenderRequest.h"
#include "Functions.h"
#include "Room.h"
#include "RoomObject.h"
#include "MapLight.h"
#include "Creature.h"
#include "Weapon.h"
#include "MissileObject.h"
#include "Trap.h"
#include "ProtectedObject.h"
#include "Field.h"
#include "Player.h"
#include "ODApplication.h"
#include "ResourceManager.h"
#include "Seat.h"
#include "MapLoader.h"

#include "RenderManager.h"
#include "LogManager.h"

template<> RenderManager* Ogre::Singleton<RenderManager>::ms_Singleton = 0;

const Ogre::Real RenderManager::BLENDER_UNITS_PER_OGRE_UNIT = 10.0;

RenderManager::RenderManager() :
        roomSceneNode(0),
        creatureSceneNode(0),
        lightSceneNode(0),
        fieldSceneNode(0),
        mainCamera(0),
        initialized(false),
        sceneManager(ODApplication::getSingletonPtr()->getRoot()->createSceneManager(Ogre::ST_EXTERIOR_CLOSE))
{
    sem_init(&renderQueueSemaphore, 0, 1);
    sem_init(&renderQueueEmptySemaphore, 0, 0);

    //Initialise RTshader system
    if (!Ogre::RTShader::ShaderGenerator::initialize()) {
        //TODO - exit properly
        LogManager::getSingletonPtr()->logMessage("FATAL:"
                "Failed to initialise shader generator, exiting", Ogre::LML_CRITICAL);
        exit(1);
    }
}

RenderManager::~RenderManager()
{
}

/*! \brief Sets up the main camera
*
*/
void RenderManager::createCamera()
{
    mainCamera = sceneManager->createCamera("PlayerCam");
    mainCamera->setNearClipDistance(.05);
    mainCamera->setFarClipDistance(300.0);
    mainCamera->setAutoTracking(false, sceneManager->getRootSceneNode()
                                ->createChildSceneNode("CameraTarget"), Ogre::Vector3(0, 0, 0));
}

/*! \brief setup the viewports
*
*/
void RenderManager::createViewports()
{
    viewport = ODApplication::getSingleton().getWindow()->
               addViewport(mainCamera);
    viewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0));
    mainCamera->setAspectRatio(Ogre::Real(viewport->getActualWidth()) / Ogre::Real(
                                   viewport->getActualHeight()));
}

/*! \brief setup the scene
*
*/
void RenderManager::createScene()
{


    //Set up the shader generator
    shaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
    //shaderGenerator->setTargetLanguage("glsl");
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        ResourceManager::getSingleton().getShaderCachePath(), "FileSystem", "Graphics");

    //FIXME - this is a workaround for an issue where the shader cache files are not found.
    //Haven't found out why this started happening. Think it worked in 3faa1aa285df504350f9704bdf20eb851fc5be3d
    //atleast.
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        ResourceManager::getSingleton().getShaderCachePath() + "../", "FileSystem", "Graphics");
    shaderGenerator->setShaderCachePath(ResourceManager::getSingleton().getShaderCachePath());
    
    shaderGenerator->addSceneManager(sceneManager);

    viewport->setMaterialScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

    


    /* TODO: move level loading to a better place
    *       (own class to exclude from global skope?)
    *       and generalize it for the future when we have more levels
    */
    // Read in the default game map
    std::string levelPath = ResourceManager::getSingletonPtr()->
                            getResourcePath() + "levels_git/Test.level";
    {
        //Check if the level from git exists. If not, use the standard one.
        std::ifstream file(levelPath.c_str(), std::ios_base::in);
        if (!file.is_open())
        {
            levelPath = ResourceManager::getSingletonPtr()->getResourcePath()
                        + "levels/Test.level";
        }
    }

    gameMap->levelFileName = "Test";
    MapLoader::readGameMapFromFile(levelPath, *gameMap);

    rtssTest();

    // Create ogre entities for the tiles, rooms, and creatures
    gameMap->createAllEntities();

    sceneManager->setAmbientLight(Ogre::ColourValue(0.05, 0.05, 0.05));

    // Create the scene node that the camera attaches to
    Ogre::SceneNode* node = sceneManager->getRootSceneNode()
                            ->createChildSceneNode("CamNode1", Ogre::Vector3(1, -1, 16));
    node->pitch(Ogre::Degree(25), Ogre::Node::TS_WORLD);
    node->roll(Ogre::Degree(30), Ogre::Node::TS_WORLD);
    node->attachObject(mainCamera);

    // Create the single tile selection mesh
    Ogre::Entity* ent = sceneManager->createEntity("SquareSelector", "SquareSelector.mesh");
    node = sceneManager->getRootSceneNode()->createChildSceneNode(
               "SquareSelectorNode");
    node->translate(Ogre::Vector3(0, 0, 0));
    node->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                              BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT));
    node->attachObject(ent);
    Ogre::SceneNode *node2 = node->createChildSceneNode("Hand_node");
    node2->setPosition(0.0 / BLENDER_UNITS_PER_OGRE_UNIT, 0.0
                       / BLENDER_UNITS_PER_OGRE_UNIT, 3.0 / BLENDER_UNITS_PER_OGRE_UNIT);
    node2->scale(Ogre::Vector3(1.0 / BLENDER_UNITS_PER_OGRE_UNIT, 1.0
                               / BLENDER_UNITS_PER_OGRE_UNIT, 1.0 / BLENDER_UNITS_PER_OGRE_UNIT));

    // Create the light which follows the single tile selection mesh
    Ogre::Light* light = sceneManager->createLight("MouseLight");
    light->setType(Ogre::Light::LT_POINT);
    light->setDiffuseColour(Ogre::ColourValue(0.65, 0.65, 0.45));
    light->setSpecularColour(Ogre::ColourValue(0.65, 0.65, 0.45));
    light->setPosition(0, 0, 6);
    light->setAttenuation(30, 0.6, 0.7, 0.17);
    //node->attachObject(light);

}

void RenderManager::setSceneNodes(Ogre::SceneNode* roomSceneNode,
                                  Ogre::SceneNode* creatureSceneNode, Ogre::SceneNode* lightSceneNode, Ogre::SceneNode* fieldSceneNode )
{
    this->roomSceneNode = roomSceneNode;
    this->creatureSceneNode = creatureSceneNode;

    this->lightSceneNode = lightSceneNode;
    this->fieldSceneNode = fieldSceneNode;
}

/*! \brief Handle the renderRequest requested
*/
bool RenderManager::handleRenderRequest ( const RenderRequest& renderRequest )
{
    //TODO - could we improve this somehow? Maybe an array of function pointers?
    switch ( renderRequest.type )
    {
    case RenderRequest::refreshTile:
        rrRefreshTile(renderRequest);
        break;

    case RenderRequest::createTile:
        rrCreateTile(renderRequest);
        break;

    case RenderRequest::destroyTile:
        rrDestroyTile(renderRequest);
        break;

    case RenderRequest::createRoom:
        rrCreateRoom(renderRequest);
        break;

    case RenderRequest::destroyRoom:
        rrDestroyRoom(renderRequest);
        break;

    case RenderRequest::createRoomObject:
        rrCreateRoomObject(renderRequest);
        break;

    case RenderRequest::destroyRoomObject:
        rrDestroyRoomObject(renderRequest);
        break;

    case RenderRequest::createTrap:
        rrCreateTrap(renderRequest);
        break;

    case RenderRequest::destroyTrap:
        rrDestroyTrap(renderRequest);
        break;

    case RenderRequest::createTreasuryIndicator:
        rrCreateTreasuryIndicator(renderRequest);
        break;

    case RenderRequest::destroyTreasuryIndicator:
        rrDestroyTreasuryIndicator(renderRequest);
        break;

    case RenderRequest::deleteRoom:
    {
        Room* curRoom = static_cast<Room*> (renderRequest.p);
        delete curRoom;
        break;
    }

    case RenderRequest::deleteTrap:
    {
        Trap* curTrap = static_cast<Trap*> (renderRequest.p);
        delete curTrap;
        break;
    }

    case RenderRequest::deleteTile:
    {
        Tile* curTile = static_cast<Tile*> (renderRequest.p);
        delete curTile;
        break;
    }

    case RenderRequest::createCreature:
        rrCreateCreature(renderRequest);
        break;

    case RenderRequest::destroyCreature:
        rrDestroyCreature(renderRequest);
        break;

    case RenderRequest::orientSceneNodeToward:
        rrOrientSceneNodeToward(renderRequest);
        break;

    case RenderRequest::reorientSceneNode:
        rrReorientSceneNode(renderRequest);
        break;

    case RenderRequest::scaleSceneNode:
        rrScaleSceneNode(renderRequest);
        break;

    case RenderRequest::createWeapon:
        rrCreateWeapon(renderRequest);
        break;

    case RenderRequest::destroyWeapon:
        rrDestroyWeapon(renderRequest);
        break;

    case RenderRequest::createMissileObject:
        rrCreateMissileObject(renderRequest);
        break;

    case RenderRequest::destroyMissileObject:
        rrDestroyMissileObject(renderRequest);
        break;

    case RenderRequest::createMapLight:
        rrCreateMapLight(renderRequest);
        break;

    case RenderRequest::destroyMapLight:
        rrDestroyMapLight(renderRequest);
        break;

    case RenderRequest::destroyMapLightVisualIndicator:
        rrDestroyMapLightVisualIndicator(renderRequest);
        break;

    case RenderRequest::deleteMapLight:
    {
        MapLight* curMapLight = static_cast<MapLight*> (renderRequest.p);
        delete curMapLight;
        break;
    }

    case RenderRequest::createField:
        rrCreateField(renderRequest);
        break;

    case RenderRequest::refreshField:
        rrRefreshField(renderRequest);
        break;

    case RenderRequest::destroyField:
        //FIXME: should there be something here?
        break;

    case RenderRequest::pickUpCreature:
        rrPickUpCreature(renderRequest);
        break;

    case RenderRequest::dropCreature:
        rrDropCreature(renderRequest);
        break;

    case RenderRequest::rotateCreaturesInHand:
        rrRotateCreaturesInHand(renderRequest);
        break;

    case RenderRequest::createCreatureVisualDebug:
        rrCreateCreatureVisualDebug(renderRequest);
        break;

    case RenderRequest::destroyCreatureVisualDebug:
        rrDestroyCreatureVisualDebug(renderRequest);
        break;

    case RenderRequest::setObjectAnimationState:
        rrSetObjectAnimationState(renderRequest);
        break;

    case RenderRequest::deleteCreature:
    {
        Creature* curCreature = static_cast<Creature*>(renderRequest.p);
        delete curCreature;
        break;
    }

    case RenderRequest::deleteWeapon:
    {
        Weapon* curWeapon = static_cast<Weapon*>(renderRequest.p);
        delete curWeapon;
        break;
    }

    case RenderRequest::deleteMissileObject:
    {
        MissileObject* curMissileObject = static_cast<MissileObject*>(renderRequest.p);
        delete curMissileObject;
        break;
    }

    case RenderRequest::moveSceneNode:
        rrMoveSceneNode(renderRequest);
        break;

    case RenderRequest::noRequest:
        break;

    default:
        std::cerr << "\n\n\nERROR:"
                  "Unhandled render request!\n\n\n";
        return false;
    }
    return true;
}

/*! \brief Loop through the render requests in the queue and process them
*/
void RenderManager::processRenderRequests()
{
    /* If the renderQueue now contains 0 objects we should process this object and then
    * release any of the other threads which were waiting on a renderQueue flush.
    * FIXME: Noting is actually being done based on this, this should be used to implement
    * a function making it easy to allow functions to wait on this.
    */
    sem_wait ( &renderQueueSemaphore );
    while (!renderQueue.empty())
    {
        // Remove the first item from the render queue


        RenderRequest *curReq = renderQueue.front();
        renderQueue.pop_front();
        sem_post ( &renderQueueSemaphore );

        // Handle the request
        handleRenderRequest ( *curReq );

        /* Decrement the number of outstanding references to things from the turn number the event was queued on.
        * (Locked in queueRenderRequest)
        */
        gameMap->threadUnlockForTurn ( curReq->turnNumber );

        delete curReq;
        curReq = NULL;

        /* If we have finished processing the last renderRequest that was in the queue we
        * can release all of the threads that were waiting for the queue to be flushed.
        */
        // FIXME - should this be here, or outside the while loop?
        for (unsigned int i = 0, numThreadsWaiting = numThreadsWaitingOnRenderQueueEmpty.get();
                i < numThreadsWaiting; ++i)
        {
            sem_post(&renderQueueEmptySemaphore);
        }

        sem_wait ( &renderQueueSemaphore );
    }
    sem_post ( &renderQueueSemaphore );

}

/*! \brief Put a render request in the queue (implementation)
*/
void RenderManager::queueRenderRequest_priv(RenderRequest* renderRequest)
{
    renderRequest->turnNumber = turnNumber.get();
    //Unlocked in processRenderRequests
    gameMap->threadLockForTurn(renderRequest->turnNumber);

    sem_wait(&renderQueueSemaphore);
    renderQueue.push_back(renderRequest);
    sem_post(&renderQueueSemaphore);
}

//NOTE: This function has not yet been tested.
void RenderManager::waitOnRenderQueueFlush()
{
    numThreadsWaitingOnRenderQueueEmpty.lock();
    unsigned int tempUnsigned = numThreadsWaitingOnRenderQueueEmpty.rawGet();
    ++tempUnsigned;
    numThreadsWaitingOnRenderQueueEmpty.rawSet(tempUnsigned);
    numThreadsWaitingOnRenderQueueEmpty.unlock();

    sem_wait(&renderQueueEmptySemaphore);
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
/*        Ogre::Entity* ent = createEntity(curTile->name,
                            Tile::meshNameFromFullness(curTile->getType(),
                                                       curTile->getFullnessMeshNumber()), "Claimedwall2_nor3.png");*/

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

    Ogre::Entity* ent = sceneManager->createEntity( curTile->name,
                        Tile::meshNameFromFullness(curTile->getType(),
                                                   curTile->getFullnessMeshNumber()));
    if (curTile->getType() == Tile::claimed)
    {
        colourizeEntity ( ent, curTile->getColor() );
    }
    Ogre::SceneNode* node = sceneManager->getRootSceneNode()->createChildSceneNode (
                                curTile->name + "_node" );

    Ogre::MeshPtr meshPtr = ent->getMesh();
    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    //node->setPosition(Ogre::Vector3(x/BLENDER_UNITS_PER_OGRE_UNIT, y/BLENDER_UNITS_PER_OGRE_UNIT, 0));
    node->attachObject ( ent );
    node->setPosition ( static_cast<Ogre::Real>(curTile->x)
                        , static_cast<Ogre::Real>(curTile->y),
                        0);
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
        Ogre::Entity* ent = sceneManager->getEntity ( curTile->name );
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
    Ogre::Entity* ent = sceneManager->createEntity ( tempSS.str(), curRoom->meshName
                        + ".mesh" );
    Ogre::SceneNode* node = roomSceneNode->createChildSceneNode ( tempSS.str()
                            + "_node" );
    node->setPosition ( static_cast<Ogre::Real>(curTile->x),
                        static_cast<Ogre::Real>(curTile->y),
                        static_cast<Ogre::Real>(0.0f));
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
        Ogre::Entity* ent = sceneManager->getEntity(tempSS.str());
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
    //TODO - find out why this was here
    //Room* curRoom = static_cast<Room*> ( renderRequest.p2 );

    std::string tempString = curRoomObject->getOgreNamePrefix()
                             + curRoomObject->getName();
    Ogre::Entity* ent = sceneManager->createEntity(tempString,
                        curRoomObject->getMeshName() + ".mesh");
    Ogre::SceneNode* node = roomSceneNode->createChildSceneNode(tempString
                            + "_node");
    node->setPosition(Ogre::Vector3(curRoomObject->x, curRoomObject->y, 0.0));
    node->roll(Ogre::Degree(curRoomObject->rotationAngle));
    node->attachObject(ent);
}

void RenderManager::rrDestroyRoomObject ( const RenderRequest& renderRequest )
{
    RoomObject* curRoomObject = static_cast<RoomObject*> (renderRequest.p);
    //Room* curRoom = static_cast<Room*> ( renderRequest.p2 );

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
    Ogre::SceneNode* node = roomSceneNode->createChildSceneNode(tempString
                            + "_node");
    node->setPosition(static_cast<Ogre::Real>(curTile->x),
                      static_cast<Ogre::Real>(curTile->y),
                      0.0f);
    node->attachObject(ent);
}

void RenderManager::rrDestroyTrap ( const RenderRequest& renderRequest )
{
    Trap* curTrap = static_cast<Trap*>( renderRequest.p);
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );

    std::stringstream tempSS;
    tempSS << "Trap_" << curTrap->getName() + "_tile_" << curTile->x << "_"
    << curTile->y;
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
        node = sceneManager->getSceneNode(tempSS.str() + "_node");
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
    Ogre::MeshPtr meshPtr = ent->getMesh();

    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    //Disabled temporarily for normal-mapping
    //colourizeEntity(ent, curCreature->color);
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
        Ogre::SceneNode* node = sceneManager->getSceneNode(curCreature->name + "_node");
        node->detachObject(ent);
        creatureSceneNode->removeChild(node);
        sceneManager->destroyEntity(ent);
        sceneManager->destroySceneNode(curCreature->name + "_node");
    }
    curCreature->sceneNode = NULL;
}

void RenderManager::rrOrientSceneNodeToward ( const RenderRequest& renderRequest )
{
    Ogre::SceneNode* node = sceneManager->getSceneNode(renderRequest.str);
    Ogre::Vector3 tempVector = node->getOrientation()
                               * Ogre::Vector3::NEGATIVE_UNIT_Y;

    // Work around 180 degree quaternion rotation quirk
    if ((1.0f + tempVector.dotProduct(renderRequest.vec)) < 0.0001f)
    {
        node->roll(Ogre::Degree(180));
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
        node->scale(renderRequest.vec);
    }
}

void RenderManager::rrCreateWeapon ( const RenderRequest& renderRequest )
{
    Weapon* curWeapon = static_cast<Weapon*>( renderRequest.p);
    Creature* curCreature = static_cast<Creature*>(renderRequest.p2);

    Ogre::Entity* ent = sceneManager->getEntity("Creature_" + curCreature->name);
    //colourizeEntity(ent, curCreature->color);
    Ogre::Entity* weaponEntity = sceneManager->createEntity("Weapon_"
                                 + curWeapon->handString + "_" + curCreature->name,
                                 curWeapon->meshName);
    Ogre::Bone* weaponBone = ent->getSkeleton()->getBone(
                                 "Weapon_" + curWeapon->handString);

    // Rotate by -90 degrees around the x-axis from the bone's rotation.
    Ogre::Quaternion rotationQuaternion;
    rotationQuaternion.FromAngleAxis(Ogre::Degree(-90.0), Ogre::Vector3(1.0,
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
    Ogre::Entity* ent = sceneManager->createEntity(curMissileObject->name,
                        curMissileObject->meshName + ".mesh");
    //TODO:  Make a new subroot scene node for these so lookups are faster since only a few missile objects should be onscreen at once.
    Ogre::SceneNode* node = creatureSceneNode->createChildSceneNode(
                                curMissileObject->name + "_node");
    node->setPosition(curMissileObject->getPosition());
    node->attachObject(ent);
}

void RenderManager::rrDestroyMissileObject ( const RenderRequest& renderRequest )
{
    MissileObject* curMissileObject = static_cast<MissileObject*>(renderRequest.p);
    if (sceneManager->hasEntity(curMissileObject->name))
    {
        Ogre::Entity* ent = sceneManager->getEntity(curMissileObject->name);
        Ogre::SceneNode* node = sceneManager->getSceneNode(curMissileObject->name
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
    if (renderRequest.b)
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
    if (sceneManager->hasLight(mapLightName))
    {
        Ogre::Light* light = sceneManager->getLight(mapLightName);
        Ogre::SceneNode* lightNode = sceneManager->getSceneNode(mapLightName + "_node");
        Ogre::SceneNode* lightFlickerNode = sceneManager->getSceneNode(mapLightName
                                            + "_flicker_node");
        lightFlickerNode->detachObject(light);
        lightSceneNode->removeChild(lightNode);
        sceneManager->destroyLight(light);

        if (sceneManager->hasEntity(mapLightName))
        {
            Ogre::Entity* mapLightIndicatorEntity = sceneManager->getEntity("MapLightIndicator_"
                                                    + curMapLight->getName());
            lightNode->detachObject(mapLightIndicatorEntity);
        }
        sceneManager->destroySceneNode(lightFlickerNode->getName());
        sceneManager->destroySceneNode(lightNode->getName());
    }
}

void RenderManager::rrDestroyMapLightVisualIndicator ( const RenderRequest& renderRequest )
{
    MapLight* curMapLight = static_cast<MapLight*> (renderRequest.p);
    std::string mapLightName = "MapLight_" + curMapLight->getName();
    if (sceneManager->hasLight(mapLightName))
    {
        Ogre::SceneNode* mapLightNode = sceneManager->getSceneNode(mapLightName + "_node");
        std::string mapLightIndicatorName = "MapLightIndicator_"
                                            + curMapLight->getName();
        if (sceneManager->hasEntity(mapLightIndicatorName))
        {
            Ogre::Entity* mapLightIndicatorEntity = sceneManager->getEntity(mapLightIndicatorName);
            mapLightNode->detachObject(mapLightIndicatorEntity);
            sceneManager->destroyEntity(mapLightIndicatorEntity);
            //NOTE: This line throws an error complaining 'scene node not found' that should not be happening.
            //sceneManager->destroySceneNode(node->getName());
        }
    }
}

void RenderManager::rrCreateField ( const RenderRequest& renderRequest )
{
    Field* curField = static_cast<Field*> (renderRequest.p);
    //FIXME these vars should have proper names
    double* tempDoublePtr = static_cast<double*>(renderRequest.p2);
    Ogre::Real tempDouble = static_cast<Ogre::Real>(*tempDoublePtr);
    delete tempDoublePtr;

    FieldType::iterator fieldItr = curField->begin();
    while (fieldItr != curField->end())
    {
        int x = fieldItr->first.first;
        int y = fieldItr->first.second;
        Ogre::Real tempDouble2 = static_cast<Ogre::Real>(fieldItr->second);
        //cout << "\ncreating field tile:  " << tempX << "
        //"\t" << tempY << "\t" << tempDouble;
        std::stringstream tempSS;
        tempSS << "Field_" << curField->name << "_" << x << "_"
        << y;
        Ogre::Entity* fieldIndicatorEntity = sceneManager->createEntity(tempSS.str(),
                                             "Field_indicator.mesh");
        Ogre::SceneNode* fieldIndicatorNode = fieldSceneNode->createChildSceneNode(tempSS.str()
                                              + "_node");
        fieldIndicatorNode->setPosition(static_cast<Ogre::Real>(x)
                                        , static_cast<Ogre::Real>(y)
                                        , tempDouble + tempDouble2);
        fieldIndicatorNode->attachObject(fieldIndicatorEntity);

        ++fieldItr;
    }
}

void RenderManager::rrRefreshField ( const RenderRequest& renderRequest )
{
    Field* curField = static_cast<Field*> (renderRequest.p);
    //FIXME these vars should have proper names
    double* tempDoublePtr = static_cast<double*>(renderRequest.p2);
    double tempDouble = *tempDoublePtr;
    delete tempDoublePtr;

    // Update existing meshes and create any new ones needed.
    FieldType::iterator fieldItr = curField->begin();
    while (fieldItr != curField->end())
    {


        int x = fieldItr->first.first;
        int y = fieldItr->first.second;
        double tempDouble2 = fieldItr->second;

        std::stringstream tempSS;
        tempSS << "Field_" << curField->name << "_" << x << "_"
        << y;

        Ogre::SceneNode* fieldIndicatorNode = NULL;

        if (sceneManager->hasEntity(tempSS.str()))
        {
            // The mesh alread exists, just get the existing one
            fieldIndicatorNode = sceneManager->getSceneNode(tempSS.str() + "_node");
        }
        else
        {
            // The mesh does not exist, create a new one
            Ogre::Entity* fieldIndicatorEntity = sceneManager->createEntity(tempSS.str(),
                                                 "Field_indicator.mesh");
            fieldIndicatorNode = fieldSceneNode->createChildSceneNode(
                                     tempSS.str() + "_node");
            fieldIndicatorNode->attachObject(fieldIndicatorEntity);
        }

        fieldIndicatorNode->setPosition(x, y, tempDouble + tempDouble2);
        ++fieldItr;
    }

    //TODO:  This is not done yet.
    // Delete any meshes not in the field currently
}

void RenderManager::rrPickUpCreature ( const RenderRequest& renderRequest )
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    // Detach the creature from the creature scene node
    Ogre::SceneNode* creatureNode = sceneManager->getSceneNode(curCreature->name + "_node");
    //FIXME this variable name is a bit misleading
    creatureSceneNode->removeChild(creatureNode);

    // Attatch the creature to the hand scene node
    sceneManager->getSceneNode("Hand_node")->addChild(creatureNode);
    //FIXME we should probably use setscale for this, because of rounding.
    creatureNode->scale(0.333, 0.333, 0.333);

    // Move the other creatures in the player's hand to make room for the one just picked up.
    for (unsigned int i = 0; i < gameMap->me->numCreaturesInHand(); ++i)
    {
        curCreature = gameMap->me->getCreatureInHand(i);
        creatureNode = sceneManager->getSceneNode(curCreature->name + "_node");
        creatureNode->setPosition(i % 6 + 1, (i / (int) 6), 0.0);
    }
}

void RenderManager::rrDropCreature ( const RenderRequest& renderRequest )
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    Player* curPlayer = static_cast<Player*> (renderRequest.p2);
    // Detach the creature from the "hand" scene node
    Ogre::SceneNode* creatureNode = sceneManager->getSceneNode(curCreature->name + "_node");
    sceneManager->getSceneNode("Hand_node")->removeChild(creatureNode);

    // Attach the creature from the creature scene node
    creatureSceneNode->addChild(creatureNode);
    creatureNode->setPosition(curCreature->getPosition());
    creatureNode->scale(3.0, 3.0, 3.0);

    // Move the other creatures in the player's hand to replace the dropped one
    for (unsigned int i = 0; i < curPlayer->numCreaturesInHand(); ++i)
    {
        curCreature = curPlayer->getCreatureInHand(i);
        creatureNode = sceneManager->getSceneNode(curCreature->name + "_node");
        creatureNode->setPosition(i % 6 + 1, (i / (int) 6), 0.0);
    }
}

void RenderManager::rrRotateCreaturesInHand ( const RenderRequest& )
{
    // Loop over the creatures in our hand and redraw each of them in their new location.
    for (unsigned int i = 0; i < gameMap->me->numCreaturesInHand(); ++i)
    {
        Creature* curCreature = gameMap->me->getCreatureInHand(i);
        Ogre::SceneNode* creatureNode = sceneManager->getSceneNode(curCreature->name + "_node");
        creatureNode->setPosition(i % 6 + 1, (i / (int) 6), 0.0);
    }
}

void RenderManager::rrCreateCreatureVisualDebug ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*>(renderRequest.p);
    Creature* curCreature = static_cast<Creature*>( renderRequest.p2);

    if (curTile != NULL && curCreature != NULL)
    {
        std::stringstream tempSS;
        tempSS << "Vision_indicator_" << curCreature->getName() << "_"
        << curTile->x << "_" << curTile->y;

        Ogre::Entity* visIndicatorEntity = sceneManager->createEntity(tempSS.str(),
                                           "Cre_vision_indicator.mesh");
        Ogre::SceneNode* visIndicatorNode = creatureSceneNode->createChildSceneNode(tempSS.str()
                                            + "_node");
        visIndicatorNode->attachObject(visIndicatorEntity);
        visIndicatorNode->setPosition(Ogre::Vector3(curTile->x, curTile->y, 0));
        visIndicatorNode->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT));
    }
}

void RenderManager::rrDestroyCreatureVisualDebug ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*>(renderRequest.p);
    Creature* curCreature = static_cast<Creature*>( renderRequest.p2);

    std::stringstream tempSS;
    tempSS << "Vision_indicator_" << curCreature->name << "_"
    << curTile->x << "_" << curTile->y;
    if (sceneManager->hasEntity(tempSS.str()))
    {
        Ogre::Entity* visIndicatorEntity = sceneManager->getEntity(tempSS.str());
        Ogre::SceneNode* visIndicatorNode = sceneManager->getSceneNode(tempSS.str() + "_node");

        visIndicatorNode->detachAllObjects();
        sceneManager->destroyEntity(visIndicatorEntity);
        sceneManager->destroySceneNode(visIndicatorNode);
    }
}

void RenderManager::rrSetObjectAnimationState ( const RenderRequest& renderRequest )
{
    Creature* curAnimatedObject = static_cast<Creature*>( renderRequest.p);
    Ogre::Entity* objectEntity = sceneManager->getEntity(
                                     curAnimatedObject->getOgreNamePrefix()
                                     + curAnimatedObject->getName());

    if (objectEntity->hasSkeleton()
            && objectEntity->getSkeleton()->hasAnimation(renderRequest.str))
    {
        // Disable the animation for all of the animations on this entity.
        Ogre::AnimationStateIterator animationStateIterator(
            objectEntity->getAllAnimationStates()->getAnimationStateIterator());
        while (animationStateIterator.hasMoreElements())
        {
            animationStateIterator.getNext()->setEnabled(false);
        }

        // Enable the animation specified in the RenderRequest object.
        // FIXME:, make a function rather than using a public var
        curAnimatedObject->animationState = objectEntity->getAnimationState(
                                                renderRequest.str);
        curAnimatedObject->animationState->setLoop(renderRequest.b);
        curAnimatedObject->animationState->setEnabled(true);
    }
    //TODO:  Handle the case where this entity does not have the requested animation.
}
void RenderManager::rrMoveSceneNode ( const RenderRequest& renderRequest )
{
    if (sceneManager->hasSceneNode(renderRequest.str))
    {
        Ogre::SceneNode* node = sceneManager->getSceneNode(renderRequest.str);
        node->setPosition(renderRequest.vec);
    }
}

bool RenderManager::generateRTSSShadersForMaterial(const std::string& materialName,
                                                   const std::string& normalMapTextureName,
                                                   Ogre::RTShader::NormalMapLighting::NormalMapSpace nmSpace)
{

    bool success = shaderGenerator->createShaderBasedTechnique(materialName, Ogre::MaterialManager::DEFAULT_SCHEME_NAME,
                   Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

    if (!success)
    {
        LogManager::getSingletonPtr()->logMessage("Failed to create shader based technique for: " + materialName
                , Ogre::LML_NORMAL);
        return false;
    }

    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(materialName);
    //Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
    LogManager::getSingleton().logMessage("Technique and scheme - " + material->getTechnique(0)->getName() + " - "
        + material->getTechnique(0)->getSchemeName());
    LogManager::getSingleton().logMessage("Viewport scheme: - " + viewport->getMaterialScheme());
    
    Ogre::RTShader::RenderState* renderState = shaderGenerator->getRenderState(
                Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, materialName, 0);

    renderState->reset();

    if (normalMapTextureName.empty())
    {
        //per-pixel lighting
        Ogre::RTShader::SubRenderState* perPixelSRS =
            shaderGenerator->createSubRenderState(Ogre::RTShader::PerPixelLighting::Type);

        renderState->addTemplateSubRenderState(perPixelSRS);
    }
    else
    {
        Ogre::RTShader::SubRenderState* subRenderState = shaderGenerator->createSubRenderState(
            Ogre::RTShader::NormalMapLighting::Type);
        Ogre::RTShader::NormalMapLighting* normalMapSRS =
            static_cast<Ogre::RTShader::NormalMapLighting*>(subRenderState);
        normalMapSRS->setNormalMapSpace(nmSpace);
        normalMapSRS->setNormalMapTextureName(normalMapTextureName);

        renderState->addTemplateSubRenderState(normalMapSRS);
    }

    shaderGenerator->invalidateMaterial(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, materialName);
    LogManager::getSingletonPtr()->logMessage("Created shader based technique for: " + materialName
                , Ogre::LML_NORMAL);
    return true;
}


void RenderManager::rtssTest()
{
    generateRTSSShadersForMaterial("Claimed", "Claimed6Nor.png");
    generateRTSSShadersForMaterial("Claimedwall", "Claimedwall2_nor3.png");
    generateRTSSShadersForMaterial("Dirt", "Dirt_dark_nor3.png");
    generateRTSSShadersForMaterial("Quarters", "Dirt_dark_nor3.png");
    //TODO - fix this model so it doesn't use the material name 'material'
    generateRTSSShadersForMaterial("Material", "Forge_normalmap.png",
        Ogre::RTShader::NormalMapLighting::NMS_OBJECT
    );
    generateRTSSShadersForMaterial("Troll2", "Troll2_nor2.png");
    generateRTSSShadersForMaterial("Kobold_skin/TEXFACE/kobold_skin6.png");
    generateRTSSShadersForMaterial("Kobold_skin/TWOSIDE/TEXFACE/kobold_skin6.png");
    generateRTSSShadersForMaterial("Wizard/TWOSIDE", "Wizard_nor.png");
    generateRTSSShadersForMaterial("Wizard", "Wizard_nor.png");
    generateRTSSShadersForMaterial("TrainingDummmy", "leatherdummy2-nm.png");
    generateRTSSShadersForMaterial("TrainingPole", "trainingpole-tex-nm.png");
    generateRTSSShadersForMaterial("Kreatur", "Kreatur_nor2.png");
    generateRTSSShadersForMaterial("Wyvern", "Wyvern_red_normalmap.png");
    generateRTSSShadersForMaterial("Roundshield");
    generateRTSSShadersForMaterial("Staff");
    
    shaderGenerator->invalidateScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
    
}

Ogre::Entity* RenderManager::createEntity(const std::string& entityName, const std::string& meshName,
                                 const std::string& normalMapTextureName)
{
    //TODO - has to be changed a bit, shaders shouldn't be generated here.
    Ogre::Entity* ent = sceneManager->createEntity ( entityName, meshName);                     

    Ogre::MeshPtr meshPtr = ent->getMesh();
    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }
    //Generate rtss shaders
    Ogre::Mesh::SubMeshIterator it = meshPtr->getSubMeshIterator();
    while(it.hasMoreElements()) {
        Ogre::SubMesh* subMesh = it.getNext();
        LogManager::getSingleton().logMessage("Trying to generate shaders for material: " + subMesh->getMaterialName());
        generateRTSSShadersForMaterial(subMesh->getMaterialName(), normalMapTextureName);
    }
    return ent;
}

void RenderManager::colourizeEntity(Ogre::Entity *ent, int colour)
{
    //Disabled for normal mapping. This has to be implemented in some other way.

    // Colorize the the textures
    // Loop over the sub entities in the mesh
    if(colour != 0)
    {
        for (unsigned int i = 0; i < ent->getNumSubEntities(); ++i)
        {
            Ogre::SubEntity *tempSubEntity = ent->getSubEntity(i);
            tempSubEntity->setMaterialName(colourizeMaterial(
                    tempSubEntity->getMaterialName(), colour));
        }
    }

}

std::string RenderManager::colourizeMaterial(const std::string& materialName, int colour)
{
    std::stringstream tempSS;
    Ogre::Technique *tempTechnique;
    Ogre::Pass *tempPass;

    tempSS.str("");
    tempSS << "Color_" << colour << "_" << materialName;
    Ogre::MaterialPtr newMaterial = Ogre::MaterialPtr(
            Ogre::MaterialManager::getSingleton().getByName(tempSS.str()));

    //cout << "\nCloning material:  " << tempSS.str();

    // If this texture has not been copied and colourized yet then do so
    if (newMaterial.isNull())
    {
        // Check to see if we find a seat with the requested color, if not then just use the original, uncolored material.
        Seat* tempSeat = gameMap->getSeatByColor(colour);
        if (tempSeat == NULL)
            return materialName;

        std::cout << "\nMaterial does not exist, creating a new one.";
        newMaterial = Ogre::MaterialPtr(
                Ogre::MaterialManager::getSingleton().getByName(
                        materialName))->clone(tempSS.str());

        // Loop over the techniques for the new material
        for (unsigned int j = 0; j < newMaterial->getNumTechniques(); ++j)
        {
            tempTechnique = newMaterial->getTechnique(j);
            if (tempTechnique->getNumPasses() > 0)
            {
                tempPass = tempTechnique->getPass(0);
                //tempPass->setSelfIllumination(tempSeat->colourValue);
                tempPass->setSpecular(tempSeat->colourValue);
                tempPass->setDiffuse(tempSeat->colourValue);
                tempPass->setAmbient(tempSeat->colourValue);
            }
        }
        generateRTSSShadersForMaterial(tempSS.str(), "Claimed6Nor.png");
    }

    return tempSS.str();

}
