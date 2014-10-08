/*!
 *  \file   RenderManager.cpp
 *  \date   26 March 2001
 *  \author oln, paul424
 *  \brief  handles the render requests
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

#include "RenderManager.h"

#include "GameMap.h"
#include "RenderRequest.h"
#include "ODServer.h"
#include "Room.h"
#include "RoomObject.h"
#include "MapLight.h"
#include "Creature.h"
#include "Weapon.h"
#include "MissileObject.h"
#include "Trap.h"
#include "Player.h"
#include "ResourceManager.h"
#include "Seat.h"
#include "MapLoader.h"
#include "MovableGameEntity.h"

#include "LogManager.h"
#include "GameEntity.h"

#include <OgreMesh.h>
#include <OgreBone.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreSubEntity.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreMovableObject.h>
#include <OgreEntity.h>
#include <OgreSubMesh.h>
#include <OgreCompositorManager.h>
#include <OgreViewport.h>
#include <OgreRoot.h>
#include <Overlay/OgreOverlaySystem.h>

//#include <RTShaderSystem/OgreShaderGenerator.h>
#include <RTShaderSystem/OgreShaderExPerPixelLighting.h>
#include <RTShaderSystem/OgreShaderExNormalMapLighting.h>
#include <sstream>

using std::stringstream;

template<> RenderManager* Ogre::Singleton<RenderManager>::msSingleton = 0;

const Ogre::Real RenderManager::BLENDER_UNITS_PER_OGRE_UNIT = 10.0;

RenderManager::RenderManager(Ogre::OverlaySystem* overlaySystem) :
    mVisibleCreatures(true),
    mGameMap(NULL),
    mViewport(NULL),
    mShaderGenerator(NULL),
    mInitialized(false)
{
    // Use Ogre::SceneType enum instead of string to identify the scene manager type; this is more robust!
    mSceneManager = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_INTERIOR, "SceneManager");
    mSceneManager->addRenderQueueListener(overlaySystem);

    mRockSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("Rock_scene_node");
    mCreatureSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("Creature_scene_node");
    mRoomSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("Room_scene_node");
    mLightSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("Light_scene_node");
}

RenderManager::~RenderManager()
{
}

void RenderManager::triggerCompositor(const std::string& compositorName)
{
    Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, compositorName.c_str(), true);
}

void RenderManager::createScene(Ogre::Viewport* nViewport)
{
    LogManager::getSingleton().logMessage("Creating scene...", Ogre::LML_NORMAL);

    mViewport = nViewport;

    //Set up the shader generator
    mShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
    //shaderGenerator->setTargetLanguage("glsl");
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        ResourceManager::getSingleton().getShaderCachePath(), "FileSystem", "Graphics");

    //FIXME - this is a workaround for an issue where the shader cache files are not found.
    //Haven't found out why this started happening. Think it worked in 3faa1aa285df504350f9704bdf20eb851fc5be3d
    //atleast.
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        ResourceManager::getSingleton().getShaderCachePath() + "../", "FileSystem", "Graphics");
    mShaderGenerator->setShaderCachePath(ResourceManager::getSingleton().getShaderCachePath());

    mShaderGenerator->addSceneManager(mSceneManager);

    mViewport->setMaterialScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

    rtssTest();

    // Sets the overall world lighting.
    mSceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));

    // Create the scene nodes that will follow the mouse pointer.
    // Create the single tile selection mesh
    Ogre::Entity* ent = mSceneManager->createEntity("SquareSelector", "SquareSelector.mesh");
    Ogre::SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode("SquareSelectorNode");
    node->translate(Ogre::Vector3(0, 0, 0));
    node->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                              BLENDER_UNITS_PER_OGRE_UNIT, 0.45 * BLENDER_UNITS_PER_OGRE_UNIT));
    node->attachObject(ent);
    Ogre::SceneNode *node2 = node->createChildSceneNode("Hand_node");
    node2->setPosition((Ogre::Real)(0.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                       (Ogre::Real)(0.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                       (Ogre::Real)(3.0 / BLENDER_UNITS_PER_OGRE_UNIT));
    node2->scale(Ogre::Vector3((Ogre::Real)(1.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                               (Ogre::Real)(1.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                               (Ogre::Real)(1.0 / BLENDER_UNITS_PER_OGRE_UNIT)));

    // Create the light which follows the single tile selection mesh
    Ogre::Light* light = mSceneManager->createLight("MouseLight");
    light->setType(Ogre::Light::LT_POINT);
    light->setDiffuseColour(Ogre::ColourValue(0.65, 0.65, 0.45));
    light->setSpecularColour(Ogre::ColourValue(0.65, 0.65, 0.45));
    light->setPosition(0, 0, 6);
    light->setAttenuation(50, 1.0, 0.09, 0.032);

    LogManager::getSingleton().logMessage("Creating compositor...", Ogre::LML_NORMAL);
    Ogre::CompositorManager::getSingleton().addCompositor(mViewport, "B&W");
}

bool RenderManager::handleRenderRequest(const RenderRequest& renderRequest)
{
    //TODO - could we improve this somehow? Maybe an array of function pointers?
    switch (renderRequest.type)
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

    case RenderRequest::detachTile:
        rrDetachTile(renderRequest);
        break;

    case RenderRequest::attachTile:
        rrAttachTile(renderRequest);
        break;

    case RenderRequest::detachCreature:
        rrDetachCreature(renderRequest);
        break;

    case RenderRequest::attachCreature:
        rrAttachCreature(renderRequest);
        break;

    case RenderRequest::toggleCreatureVisibility:
        rrToggleCreaturesVisibility();
        break;

    case RenderRequest::temporalMarkTile:
        rrTemporalMarkTile(renderRequest);
        break;

    case RenderRequest::showSquareSelector:
        rrShowSquareSelector(renderRequest);
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

    case RenderRequest::deleteRoomObject:
    {
        RoomObject* curRoomObject = static_cast<RoomObject*>(renderRequest.p);
        delete curRoomObject;
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
        std::cerr << "WARNING: Unhandled render request! Request type number: " << renderRequest.type << std::endl;
        return false;
    }
    return true;
}

void RenderManager::processRenderRequests()
{
    /* If the renderQueue now contains 0 objects we should process this object and then
    * release any of the other threads which were waiting on a renderQueue flush.
    * FIXME: Noting is actually being done based on this, this should be used to implement
    * a function making it easy to allow functions to wait on this.
    */
    while (!mRenderQueue.empty())
    {
        // Remove the first item from the render queue


        RenderRequest *curReq = mRenderQueue.front();
        mRenderQueue.pop_front();

        // Handle the request
        handleRenderRequest (*curReq);

        delete curReq;
        curReq = NULL;
    }
}

void RenderManager::queueRenderRequest_priv(RenderRequest* renderRequest)
{
    renderRequest->turnNumber = mGameMap->getTurnNumber();

    mRenderQueue.push_back(renderRequest);
}

void RenderManager::rrRefreshTile(const RenderRequest& renderRequest)
{
    int rt = 0;
    Tile* curTile = static_cast<Tile*>(renderRequest.p);
    std::string tileName = curTile->getOgreNamePrefix() + curTile->getName();

    if (!mSceneManager->hasSceneNode(tileName + "_node"))
        return;

    // Unlink and delete the old mesh
    mSceneManager->getSceneNode(tileName + "_node")->detachObject(tileName);
    mSceneManager->destroyEntity(tileName);

    std::string meshName = Tile::meshNameFromNeighbors(curTile->getType(),
                                                       curTile->getFullnessMeshNumber(),
                                                       mGameMap->getNeighborsTypes(curTile),
                                                       mGameMap->getNeighborsFullness(curTile),
                                                       rt);

    Ogre::Entity* ent = mSceneManager->createEntity(tileName, meshName);

    if(curTile->getType() == Tile::gold)
    {
        for(unsigned int ii = 0; ii < ent->getNumSubEntities(); ++ii)
        {
            ent->getSubEntity(ii)->setMaterialName("Gold");
        }
    }
    else if(curTile->getType() == Tile::rock)
    {
        for(unsigned int ii = 0; ii < ent->getNumSubEntities(); ++ii)
        {
            ent->getSubEntity(ii)->setMaterialName("Rock");
        }

    }
    else if(curTile->getType() == Tile::lava)
    {
        for(unsigned int ii = 0; ii < ent->getNumSubEntities(); ++ii)
        {
            Ogre::SubEntity* subEnt = ent->getSubEntity(ii);
            if (subEnt->getMaterialName() == "Water")
                subEnt->setMaterialName("Lava");
        }
    }

    colourizeEntity(ent, curTile->getSeat(), curTile->getMarkedForDigging(mGameMap->getLocalPlayer()));

    // Link the tile mesh back to the relevant scene node so OGRE will render it
    Ogre::SceneNode* node = mSceneManager->getSceneNode(tileName + "_node");
    node->attachObject(ent);
    node->resetOrientation();
    node->roll(Ogre::Degree((Ogre::Real)(-1 * rt * 90)));
}


void RenderManager::rrCreateTile(const RenderRequest& renderRequest)
{
    int rt = 0;
    Tile* curTile = static_cast<Tile*> (renderRequest.p);

    std::string meshName = Tile::meshNameFromNeighbors(curTile->getType(),
                                                       curTile->getFullnessMeshNumber(),
                                                       mGameMap->getNeighborsTypes(curTile),
                                                       mGameMap->getNeighborsFullness(curTile),
                                                       rt);

    Ogre::Entity* ent = mSceneManager->createEntity(curTile->getOgreNamePrefix() + curTile->getName(), meshName);

    if(curTile->getType() == Tile::gold)
    {
        for(unsigned int ii = 0; ii < ent->getNumSubEntities(); ++ii)
        {
            ent->getSubEntity(ii)->setMaterialName("Gold");
        }
    }
    else if(curTile->getType() == Tile::rock)
    {
        for(unsigned int ii = 0; ii < ent->getNumSubEntities(); ++ii)
        {
            ent->getSubEntity(ii)->setMaterialName("Rock");
        }
    }
    else if(curTile->getType() == Tile::lava)
    {
        for(unsigned int ii = 0; ii < ent->getNumSubEntities(); ++ii)
        {
            Ogre::SubEntity* subEnt = ent->getSubEntity(ii);
            if (subEnt->getMaterialName() == "Water")
                subEnt->setMaterialName("Lava");
        }
    }

    if (curTile->getType() == Tile::claimed)
    {
        colourizeEntity(ent, curTile->getSeat(), curTile->getMarkedForDigging(mGameMap->getLocalPlayer()));
    }

    Ogre::SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode(curTile->getOgreNamePrefix() + curTile->getName() + "_node");

    Ogre::MeshPtr meshPtr = ent->getMesh();
    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    node->setPosition(static_cast<Ogre::Real>(curTile->x), static_cast<Ogre::Real>(curTile->y), 0);

    node->attachObject(ent);

    node->setScale(Ogre::Vector3((Ogre::Real)(4.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                                 (Ogre::Real)(4.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                                 (Ogre::Real)(5.0 / BLENDER_UNITS_PER_OGRE_UNIT)));
    node->resetOrientation();
    node->roll(Ogre::Degree((Ogre::Real)(-1 * rt * 90)));
}

void RenderManager::rrDestroyTile (const RenderRequest& renderRequest)
{
    Tile* curTile = static_cast<Tile*>(renderRequest.p);

    if (mSceneManager->hasEntity(curTile->getOgreNamePrefix() + curTile->getName()))
    {
        Ogre::Entity* ent = mSceneManager->getEntity(curTile->getOgreNamePrefix() + curTile->getName());
        Ogre::SceneNode* node = mSceneManager->getSceneNode(curTile->getOgreNamePrefix() + curTile->getName() + "_node");
        node->detachAllObjects();
        mSceneManager->destroySceneNode(node->getName());
        mSceneManager->destroyEntity(ent);
    }
}

void RenderManager::rrTemporalMarkTile(const RenderRequest& renderRequest)
{
    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    Ogre::Entity* ent;
    std::stringstream ss;
    std::stringstream ss2;
    Tile* curTile = static_cast<Tile*>(renderRequest.p);

    bool bb = curTile->getSelected();

    ss.str(std::string());
    ss << curTile->getOgreNamePrefix();
    ss << curTile->getName();
    ss << "_selection_indicator";

    if (mSceneMgr->hasEntity(ss.str()))
    {
        ent = mSceneMgr->getEntity(ss.str());
    }
    else
    {
        ss2.str(std::string());
        ss2 << curTile->getOgreNamePrefix();
        ss2 << curTile->getName();
        ss2 << "_node";
        ent = mSceneMgr->createEntity(ss.str(), "SquareSelector.mesh");
        Ogre::SceneNode* node = mSceneManager->getSceneNode(ss2.str())->createChildSceneNode(ss.str()+"Node");
        node->setInheritScale(false);
        node->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                  BLENDER_UNITS_PER_OGRE_UNIT, 0.45 * BLENDER_UNITS_PER_OGRE_UNIT));
        node->attachObject(ent);
    }

    ent->setVisible(bb);
}

void RenderManager::rrDetachTile(const RenderRequest& renderRequest)
{
    GameEntity* curEntity = static_cast<GameEntity*>(renderRequest.p);
    Ogre::SceneNode* tileNode = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");

    curEntity->pSN=(tileNode->getParentSceneNode());
    curEntity->pSN->removeChild(tileNode);
}

void RenderManager::rrAttachTile(const RenderRequest& renderRequest)
{
    GameEntity* curEntity = static_cast<GameEntity*>(renderRequest.p);
    Ogre::SceneNode* creatureNode = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");

    Ogre::SceneNode* parentNode = creatureNode->getParentSceneNode();
    if (parentNode == nullptr)
    {
        curEntity->pSN->addChild(creatureNode);
    }
    else
    {
        curEntity->pSN = parentNode;
    }
}

void RenderManager::rrDetachCreature(const RenderRequest& renderRequest)
{
    GameEntity* curEntity = static_cast<GameEntity*>(renderRequest.p);
    Ogre::SceneNode* creatureNode = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");

    curEntity->pSN->removeChild(creatureNode);
}

void RenderManager::rrAttachCreature(const RenderRequest& renderRequest)
{
    GameEntity* curEntity = static_cast<GameEntity*>(renderRequest.p);
    Ogre::SceneNode* creatureNode = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");

    curEntity->pSN->addChild(creatureNode);
}

void RenderManager::rrToggleCreaturesVisibility()
{
    mVisibleCreatures = !mVisibleCreatures;

    if(mVisibleCreatures)
    {
        for(std::vector<Creature*>::iterator it = mGameMap->creatures.begin(); it != mGameMap->creatures.end(); ++it)
        {
            if((*it)->isMeshExisting() && (*it)->mSceneNode != NULL)

            // (*it)->pSN=((*it)->sceneNode->getParentSceneNode());
            //pSN->removeChild((*it)->
                (*it)->pSN->addChild((*it)->mSceneNode);
            //  addAnimatedObject(*it);
            // (*it)->createMesh();
        }
    }
    else
    {
        for(std::vector<Creature*>::iterator it = mGameMap->creatures.begin(); it != mGameMap->creatures.end(); ++it)
        {
            if((*it)->isMeshExisting() && (*it)->mSceneNode!=NULL)
            {
                (*it)->pSN=((*it)->mSceneNode->getParentSceneNode());
                (*it)->pSN->removeChild((*it)->mSceneNode);
            }

            // removeAnimatedObject(*it);
            // (*it)->destroyMesh();
        }
    }
}

void RenderManager::rrShowSquareSelector(const RenderRequest& renderRequest)
{
    int* xPos = static_cast<int*>(renderRequest.p);
    int* yPos = static_cast<int*>(renderRequest.p2);

    mSceneManager->getEntity("SquareSelector")->setVisible(true);
    mSceneManager->getSceneNode("SquareSelectorNode")->setPosition((Ogre::Real)*xPos,
                                                                  (Ogre::Real)*yPos,
                                                                  (Ogre::Real)0);
}

void RenderManager::rrCreateRoom(const RenderRequest& renderRequest)
{
    Room* curRoom = static_cast<Room*>(renderRequest.p);
    Tile* curTile = static_cast<Tile*>(renderRequest.p2);

    // We do not display ground tile if not required
    if(!curRoom->shouldDisplayMeshOnGround())
        return;

    std::stringstream tempSS;
    tempSS << curRoom->getOgreNamePrefix() << curRoom->getNameTile(curTile);
    // Create the room ground tile

    Ogre::Entity* ent = mSceneManager->createEntity(tempSS.str(), curRoom->getMeshName() + ".mesh");
    Ogre::SceneNode* node = mRoomSceneNode->createChildSceneNode(tempSS.str() + "_node");

    node->setPosition(static_cast<Ogre::Real>(curTile->x),
                       static_cast<Ogre::Real>(curTile->y),
                       static_cast<Ogre::Real>(0.02f));
    node->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                 BLENDER_UNITS_PER_OGRE_UNIT,
                                 BLENDER_UNITS_PER_OGRE_UNIT));
    node->attachObject(ent);
}

void RenderManager::rrDestroyRoom(const RenderRequest& renderRequest)
{
    Room* curRoom = static_cast<Room*>(renderRequest.p);
    Tile* curTile = static_cast<Tile*>(renderRequest.p2);

    std::stringstream tempSS;
    tempSS << curRoom->getOgreNamePrefix() << curRoom->getNameTile(curTile);

    std::string tempString = tempSS.str();
    // Buildings do not necessarily use ground mesh. So, we remove it only if it exists
    if(!mSceneManager->hasEntity(tempString))
        return;

    Ogre::Entity* ent = mSceneManager->getEntity(tempString);
    Ogre::SceneNode* node = mSceneManager->getSceneNode(tempString + "_node");
    node->detachObject(ent);
    mRoomSceneNode->removeChild(node);
    mSceneManager->destroyEntity(ent);
    mSceneManager->destroySceneNode(node->getName());
}

void RenderManager::rrCreateRoomObject(const RenderRequest& renderRequest)
{
    RoomObject* curRoomObject = static_cast<RoomObject*> (renderRequest.p);
    std::string name = renderRequest.str;
    std::string meshName = renderRequest.str2;
    std::string tempString = curRoomObject->getOgreNamePrefix() + name;

    Ogre::Entity* ent = mSceneManager->createEntity(tempString, meshName + ".mesh");
    Ogre::SceneNode* node = mRoomSceneNode->createChildSceneNode(tempString + "_node");

    node->setPosition(curRoomObject->getPosition());
    node->setScale(Ogre::Vector3(0.7, 0.7, 0.7));
    node->roll(Ogre::Degree(curRoomObject->mRotationAngle));
    node->attachObject(ent);
}

void RenderManager::rrDestroyRoomObject(const RenderRequest& renderRequest)
{
    RoomObject* curRoomObject = static_cast<RoomObject*> (renderRequest.p);

    std::string tempString = curRoomObject->getOgreNamePrefix()
                             + curRoomObject->getName();
    Ogre::Entity* ent = mSceneManager->getEntity(tempString);
    Ogre::SceneNode* node = mSceneManager->getSceneNode(tempString + "_node");
    node->detachObject(ent);
    mSceneManager->destroySceneNode(node->getName());
    mSceneManager->destroyEntity(ent);
}

void RenderManager::rrCreateTrap(const RenderRequest& renderRequest)
{
    Trap* curTrap = static_cast<Trap*>(renderRequest.p);
    Tile* curTile = static_cast<Tile*>(renderRequest.p2);

    // We do not display ground tile if not required
    if(!curTrap->shouldDisplayMeshOnGround())
        return;

    std::stringstream tempSS;
    tempSS << curTrap->getOgreNamePrefix() << curTrap->getNameTile(curTile);
    std::string tempString = tempSS.str();
    Ogre::Entity* ent = mSceneManager->createEntity(tempString, curTrap->getMeshName() + ".mesh");
    Ogre::SceneNode* node = mRoomSceneNode->createChildSceneNode(tempString + "_node");
    node->setPosition(static_cast<Ogre::Real>(curTile->x),
                      static_cast<Ogre::Real>(curTile->y),
                      0.0f);
    node->attachObject(ent);
}

void RenderManager::rrDestroyTrap(const RenderRequest& renderRequest)
{
    Trap* curTrap = static_cast<Trap*>(renderRequest.p);
    Tile* curTile = static_cast<Tile*>(renderRequest.p2);

    std::stringstream tempSS;
    tempSS << curTrap->getOgreNamePrefix() << curTrap->getNameTile(curTile);
    std::string tempString = tempSS.str();
    // Buildings do not necessarily use ground mesh. So, we remove it only if it exists
    if(!mSceneManager->hasEntity(tempString))
        return;

    Ogre::Entity* ent = mSceneManager->getEntity(tempString);
    Ogre::SceneNode* node = mSceneManager->getSceneNode(tempString + "_node");
    node->detachObject(ent);
    mSceneManager->destroySceneNode(node->getName());
    mSceneManager->destroyEntity(ent);
}

void RenderManager::rrCreateCreature(const RenderRequest& renderRequest)
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    std::string meshName = renderRequest.str;
    Ogre::Vector3 scale = renderRequest.vec;

    assert(curCreature != 0);
    //assert(curCreature->getDefinition() != 0);

    // Load the mesh for the creature
    std::string creatureName = curCreature->getOgreNamePrefix() + curCreature->getName();
    Ogre::Entity* ent = mSceneManager->createEntity(creatureName, meshName);
    Ogre::MeshPtr meshPtr = ent->getMesh();

    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    //Disabled temporarily for normal-mapping
    //colourizeEntity(ent, curCreature->color);
    Ogre::SceneNode* node = mCreatureSceneNode->createChildSceneNode(creatureName + "_node");
    curCreature->mSceneNode = node;
    node->setPosition(curCreature->getPosition());
    node->setScale(scale);
    node->attachObject(ent);
    curCreature->pSN = (node->getParentSceneNode());
    // curCreature->pSN->removeChild(node);
}

void RenderManager::rrDestroyCreature(const RenderRequest& renderRequest)
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    std::string creatureName = curCreature->getOgreNamePrefix() + curCreature->getName();
    if (mSceneManager->hasEntity(creatureName))
    {
        Ogre::Entity* ent = mSceneManager->getEntity(creatureName);
        Ogre::SceneNode* node = mSceneManager->getSceneNode(creatureName + "_node");
        node->detachObject(ent);
        mCreatureSceneNode->removeChild(node);
        mSceneManager->destroyEntity(ent);
        mSceneManager->destroySceneNode(node->getName());
    }
    curCreature->mSceneNode = NULL;
}

void RenderManager::rrOrientSceneNodeToward(const RenderRequest& renderRequest)
{
    MovableGameEntity* gameEntity = static_cast<MovableGameEntity*>(renderRequest.p);
    Ogre::SceneNode* node = mSceneManager->getSceneNode(gameEntity->getOgreNamePrefix() + gameEntity->getName() + "_node");
    Ogre::Vector3 tempVector = node->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Y;

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

void RenderManager::rrReorientSceneNode(const RenderRequest& renderRequest)
{
    Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(renderRequest.p);

    if (node != NULL)
    {
        node->rotate(renderRequest.quaternion);
    }
}

void RenderManager::rrScaleSceneNode(const RenderRequest& renderRequest)
{
    Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(renderRequest.p);

    if (node != NULL)
    {
        node->scale(renderRequest.vec);
    }
}

void RenderManager::rrCreateWeapon(const RenderRequest& renderRequest)
{
    Weapon* curWeapon = static_cast<Weapon*>( renderRequest.p);
    Creature* curCreature = static_cast<Creature*>(renderRequest.p2);

    Ogre::Entity* ent = mSceneManager->getEntity(curCreature->getOgreNamePrefix() + curCreature->getName());
    //colourizeEntity(ent, curCreature->color);
    Ogre::Entity* weaponEntity = mSceneManager->createEntity(curWeapon->getOgreNamePrefix()
                                 + curWeapon->getHandString() + "_" + curCreature->getName(),
                                 curWeapon->getMeshName());
    Ogre::Bone* weaponBone = ent->getSkeleton()->getBone(
                                 curWeapon->getOgreNamePrefix() + curWeapon->getHandString());

    // Rotate by -90 degrees around the x-axis from the bone's rotation.
    Ogre::Quaternion rotationQuaternion;
    rotationQuaternion.FromAngleAxis(Ogre::Degree(-90.0), Ogre::Vector3(1.0,
                                     0.0, 0.0));

    ent->attachObjectToBone(weaponBone->getName(), weaponEntity,
                            rotationQuaternion);
}

void RenderManager::rrDestroyWeapon(const RenderRequest& renderRequest)
{
    Weapon* curWeapon = static_cast<Weapon*>(renderRequest.p);
    Creature* curCreature = static_cast<Creature*>(renderRequest.p2);

    if (curWeapon->getName().compare("none") != 0)
    {
        Ogre::Entity* ent = mSceneManager->getEntity(curWeapon->getOgreNamePrefix()
                            + curWeapon->getHandString() + "_" + curCreature->getName());
        mSceneManager->destroyEntity(ent);
    }
}

void RenderManager::rrCreateMissileObject(const RenderRequest& renderRequest)
{
    MissileObject* curMissileObject = static_cast<MissileObject*>(renderRequest.p);
    Ogre::Entity* ent = mSceneManager->createEntity(curMissileObject->getOgreNamePrefix()
        + curMissileObject->getName(), curMissileObject->getMeshName() + ".mesh");
    //TODO:  Make a new subroot scene node for these so lookups are faster
    // since only a few missile objects should be onscreen at once.
    Ogre::SceneNode* node = mCreatureSceneNode->createChildSceneNode(
                                ent->getName() + "_node");
    node->setPosition(curMissileObject->getPosition());
    node->attachObject(ent);
}

void RenderManager::rrDestroyMissileObject(const RenderRequest& renderRequest)
{
    MissileObject* curMissileObject = static_cast<MissileObject*>(renderRequest.p);
    std::string moName = curMissileObject->getOgreNamePrefix() + curMissileObject->getName();
    if (mSceneManager->hasEntity(moName))
    {
        Ogre::Entity* ent = mSceneManager->getEntity(moName);
        Ogre::SceneNode* node = mSceneManager->getSceneNode(moName + "_node");
        node->detachObject(ent);
        mCreatureSceneNode->removeChild(node);
        mSceneManager->destroyEntity(ent);
        mSceneManager->destroySceneNode(node->getName());
    }
}

void RenderManager::rrCreateMapLight(const RenderRequest& renderRequest)
{
    MapLight* curMapLight = static_cast<MapLight*> (renderRequest.p);

    // Create the light and attach it to the lightSceneNode.
    std::string mapLightName = curMapLight->getOgreNamePrefix() + curMapLight->getName();
    Ogre::Light* light = mSceneManager->createLight(mapLightName);
    light->setDiffuseColour(curMapLight->getDiffuseColor());
    light->setSpecularColour(curMapLight->getSpecularColor());
    light->setAttenuation(curMapLight->getAttenuationRange(),
                          curMapLight->getAttenuationConstant(),
                          curMapLight->getAttenuationLinear(),
                          curMapLight->getAttenuationQuadratic());

    // Create the base node that the "flicker_node" and the mesh attach to.
    Ogre::SceneNode* mapLightNode = mLightSceneNode->createChildSceneNode(mapLightName + "_node");
    mapLightNode->setPosition(curMapLight->getPosition());

    //TODO - put this in request so we don't have to include the globals here.
    if (renderRequest.b)
    {
        // Create the MapLightIndicator mesh so the light can be drug around in the map editor.
        Ogre::Entity* lightEntity = mSceneManager->createEntity(MapLight::MAPLIGHT_INDICATOR_PREFIX
                                    + curMapLight->getName(), "Lamp.mesh");
        mapLightNode->attachObject(lightEntity);
    }

    // Create the "flicker_node" which moves around randomly relative to
    // the base node.  This node carries the light itself.
    Ogre::SceneNode* flickerNode = mapLightNode->createChildSceneNode(mapLightName + "_flicker_node");
    flickerNode->attachObject(light);
}

void RenderManager::rrDestroyMapLight(const RenderRequest& renderRequest)
{
    std::string mapLightName = renderRequest.str;
    if (mSceneManager->hasLight(mapLightName))
    {
        Ogre::Light* light = mSceneManager->getLight(mapLightName);
        Ogre::SceneNode* lightNode = mSceneManager->getSceneNode(mapLightName + "_node");
        Ogre::SceneNode* lightFlickerNode = mSceneManager->getSceneNode(mapLightName
                                            + "_flicker_node");
        lightFlickerNode->detachObject(light);
        mLightSceneNode->removeChild(lightNode);
        mSceneManager->destroyLight(light);

        if (mSceneManager->hasEntity(mapLightName))
        {
            Ogre::Entity* mapLightIndicatorEntity = mSceneManager->getEntity(
                mapLightName);
            lightNode->detachObject(mapLightIndicatorEntity);
        }
        mSceneManager->destroySceneNode(lightFlickerNode->getName());
        mSceneManager->destroySceneNode(lightNode->getName());
    }
}

void RenderManager::rrDestroyMapLightVisualIndicator(const RenderRequest& renderRequest)
{
    std::string mapLightName = renderRequest.str;
    if (mSceneManager->hasLight(mapLightName))
    {
        Ogre::SceneNode* mapLightNode = mSceneManager->getSceneNode(mapLightName + "_node");
        std::string mapLightIndicatorName = MapLight::MAPLIGHT_INDICATOR_PREFIX
                                            + renderRequest.str2;
        if (mSceneManager->hasEntity(mapLightIndicatorName))
        {
            Ogre::Entity* mapLightIndicatorEntity = mSceneManager->getEntity(mapLightIndicatorName);
            mapLightNode->detachObject(mapLightIndicatorEntity);
            mSceneManager->destroyEntity(mapLightIndicatorEntity);
            //NOTE: This line throws an error complaining 'scene node not found' that should not be happening.
            //mSceneManager->destroySceneNode(node->getName());
        }
    }
}

void RenderManager::rrPickUpCreature(const RenderRequest& renderRequest)
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    // Detach the creature from the creature scene node
    Ogre::SceneNode* creatureNode = mSceneManager->getSceneNode(curCreature->getOgreNamePrefix() + curCreature->getName() + "_node");
    //FIXME this variable name is a bit misleading
    mCreatureSceneNode->removeChild(creatureNode);

    // Attach the creature to the hand scene node
    mSceneManager->getSceneNode("Hand_node")->addChild(creatureNode);
    //FIXME we should probably use setscale for this, because of rounding.
    creatureNode->scale(0.333, 0.333, 0.333);

    // Move the other creatures in the player's hand to make room for the one just picked up.
    for (unsigned int i = 0; i < mGameMap->getLocalPlayer()->numCreaturesInHand(); ++i)
    {
        curCreature = mGameMap->getLocalPlayer()->getCreatureInHand(i);
        creatureNode = mSceneManager->getSceneNode(curCreature->getOgreNamePrefix() + curCreature->getName() + "_node");
        creatureNode->setPosition((Ogre::Real)(i % 6 + 1), (Ogre::Real)(i / (int)6), (Ogre::Real)0.0);
    }
}

void RenderManager::rrDropCreature(const RenderRequest& renderRequest)
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    Player* curPlayer = static_cast<Player*> (renderRequest.p2);
    // Detach the creature from the "hand" scene node
    Ogre::SceneNode* creatureNode = mSceneManager->getSceneNode(curCreature->getOgreNamePrefix() + curCreature->getName() + "_node");
    mSceneManager->getSceneNode("Hand_node")->removeChild(creatureNode);

    // Attach the creature from the creature scene node
    mCreatureSceneNode->addChild(creatureNode);
    creatureNode->setPosition(curCreature->getPosition());
    creatureNode->scale(3.0, 3.0, 3.0);

    // Move the other creatures in the player's hand to replace the dropped one
    for (unsigned int i = 0; i < curPlayer->numCreaturesInHand(); ++i)
    {
        curCreature = curPlayer->getCreatureInHand(i);
        creatureNode = mSceneManager->getSceneNode(curCreature->getOgreNamePrefix() + curCreature->getName() + "_node");
        creatureNode->setPosition((Ogre::Real)(i % 6 + 1), (Ogre::Real)(i / (int)6), (Ogre::Real)0.0);
    }
}

void RenderManager::rrRotateCreaturesInHand(const RenderRequest&)
{
    // Loop over the creatures in our hand and redraw each of them in their new location.
    for (unsigned int i = 0; i < mGameMap->getLocalPlayer()->numCreaturesInHand(); ++i)
    {
        Creature* curCreature = mGameMap->getLocalPlayer()->getCreatureInHand(i);
        Ogre::SceneNode* creatureNode = mSceneManager->getSceneNode(curCreature->getOgreNamePrefix() + curCreature->getName() + "_node");
        creatureNode->setPosition((Ogre::Real)(i % 6 + 1), (Ogre::Real)(i / (int)6), (Ogre::Real)0.0);
    }
}

void RenderManager::rrCreateCreatureVisualDebug(const RenderRequest& renderRequest)
{
    Tile* curTile = static_cast<Tile*>(renderRequest.p);
    Creature* curCreature = static_cast<Creature*>( renderRequest.p2);

    if (curTile != NULL && curCreature != NULL)
    {
        std::stringstream tempSS;
        tempSS << "Vision_indicator_" << curCreature->getName() << "_"
        << curTile->x << "_" << curTile->y;

        Ogre::Entity* visIndicatorEntity = mSceneManager->createEntity(tempSS.str(),
                                           "Cre_vision_indicator.mesh");
        Ogre::SceneNode* visIndicatorNode = mCreatureSceneNode->createChildSceneNode(tempSS.str()
                                            + "_node");
        visIndicatorNode->attachObject(visIndicatorEntity);
        visIndicatorNode->setPosition(Ogre::Vector3((Ogre::Real)curTile->x, (Ogre::Real)curTile->y, (Ogre::Real)0));
        visIndicatorNode->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT));
    }
}

void RenderManager::rrDestroyCreatureVisualDebug(const RenderRequest& renderRequest)
{
    Tile* curTile = static_cast<Tile*>(renderRequest.p);
    Creature* curCreature = static_cast<Creature*>(renderRequest.p2);

    std::stringstream tempSS;
    tempSS << "Vision_indicator_" << curCreature->getName() << "_"
    << curTile->x << "_" << curTile->y;
    if (mSceneManager->hasEntity(tempSS.str()))
    {
        Ogre::Entity* visIndicatorEntity = mSceneManager->getEntity(tempSS.str());
        Ogre::SceneNode* visIndicatorNode = mSceneManager->getSceneNode(tempSS.str() + "_node");

        visIndicatorNode->detachAllObjects();
        mSceneManager->destroyEntity(visIndicatorEntity);
        mSceneManager->destroySceneNode(visIndicatorNode);
    }
}

void RenderManager::rrSetObjectAnimationState(const RenderRequest& renderRequest)
{
    MovableGameEntity* curAnimatedObject = static_cast<MovableGameEntity*>(renderRequest.p);
    Ogre::Entity* objectEntity = mSceneManager->getEntity(
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
        curAnimatedObject->mAnimationState = objectEntity->getAnimationState(
                                                renderRequest.str);
        curAnimatedObject->mAnimationState->setTimePosition(0);
        curAnimatedObject->mAnimationState->setLoop(renderRequest.b);
        curAnimatedObject->mAnimationState->setEnabled(true);
    }
    //TODO:  Handle the case where this entity does not have the requested animation.
}
void RenderManager::rrMoveSceneNode(const RenderRequest& renderRequest)
{
    if (mSceneManager->hasSceneNode(renderRequest.str))
    {
        Ogre::SceneNode* node = mSceneManager->getSceneNode(renderRequest.str);
        node->setPosition(renderRequest.vec);
    }
}

std::string RenderManager::consoleListAnimationsForMesh(const std::string& meshName)
{
    if(!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(meshName + ".mesh"))
        return "\nmesh not available for " + meshName;

    std::string name = meshName + "consoleListAnimationsForMesh";
    Ogre::Entity* objectEntity = msSingleton->mSceneManager->createEntity(name, meshName + ".mesh");
    if (!objectEntity->hasSkeleton())
        return "\nNo skeleton for " + meshName;

    std::string ret;
    Ogre::AnimationStateIterator animationStateIterator(
            objectEntity->getAllAnimationStates()->getAnimationStateIterator());
    while (animationStateIterator.hasMoreElements())
    {
        std::string animName = animationStateIterator.getNext()->getAnimationName();
        ret += "\nAnimation " + animName;
    }
    msSingleton->mSceneManager->destroyEntity(objectEntity);
   return ret;
}

bool RenderManager::generateRTSSShadersForMaterial(const std::string& materialName,
                                                   const std::string& normalMapTextureName,
                                                   Ogre::RTShader::NormalMapLighting::NormalMapSpace nmSpace)
{
    std::cout << "RenderManager::generateRTSSShadersForMaterial(" << materialName << "," << normalMapTextureName << "," << nmSpace << ")" << std::endl;

    bool success = mShaderGenerator->createShaderBasedTechnique(materialName, Ogre::MaterialManager::DEFAULT_SCHEME_NAME,
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
    LogManager::getSingleton().logMessage("Viewport scheme: - " + mViewport->getMaterialScheme());

    Ogre::RTShader::RenderState* renderState = mShaderGenerator->getRenderState(
                Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, materialName, 0);

    renderState->reset();

    if (normalMapTextureName.empty())
    {
        //per-pixel lighting
        Ogre::RTShader::SubRenderState* perPixelSRS =
            mShaderGenerator->createSubRenderState(Ogre::RTShader::PerPixelLighting::Type);

        renderState->addTemplateSubRenderState(perPixelSRS);
    }
    else
    {
        Ogre::RTShader::SubRenderState* subRenderState = mShaderGenerator->createSubRenderState(
                    Ogre::RTShader::NormalMapLighting::Type);
        Ogre::RTShader::NormalMapLighting* normalMapSRS =
            static_cast<Ogre::RTShader::NormalMapLighting*>(subRenderState);
        normalMapSRS->setNormalMapSpace(nmSpace);
        normalMapSRS->setNormalMapTextureName(normalMapTextureName);

        renderState->addTemplateSubRenderState(normalMapSRS);
    }

    mShaderGenerator->invalidateMaterial(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, materialName);
    LogManager::getSingletonPtr()->logMessage("Created shader based technique for: " + materialName, Ogre::LML_NORMAL);
    return true;
}

void RenderManager::rtssTest()
{
    generateRTSSShadersForMaterial("Claimed", "Claimed6Nor.png");
    generateRTSSShadersForMaterial("Claimedwall", "Claimedwall2_nor3.png");
    //generateRTSSShadersForMaterial("Dirt", "Dirt_dark_nor3.png");
    generateRTSSShadersForMaterial("Dormitory", "Dirt_dark_nor3.png");
    //TODO - fix this model so it doesn't use the material name 'material'
    generateRTSSShadersForMaterial("Material", "Forge_normalmap.png");
    generateRTSSShadersForMaterial("Troll2", "Troll2_nor2.png");
    generateRTSSShadersForMaterial("Kobold_skin/TEXFACE/kobold_skin6.png");
    generateRTSSShadersForMaterial("Kobold_skin/TWOSIDE/TEXFACE/kobold_skin6.png");
    generateRTSSShadersForMaterial("Wizard/TWOSIDE", "Wizard_nor.png");
    generateRTSSShadersForMaterial("Wizard", "Wizard_nor.png");
    generateRTSSShadersForMaterial("TrainingPole", "trainingpole-tex-nm.png");
    generateRTSSShadersForMaterial("Kreatur", "Kreatur_nor2.png");
    generateRTSSShadersForMaterial("Wyvern", "Wyvern_red_normalmap.png");
    //generateRTSSShadersForMaterial("Gold", "Dirt_dark_nor3.png");
    generateRTSSShadersForMaterial("Roundshield");
    generateRTSSShadersForMaterial("Staff");

    mShaderGenerator->invalidateScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
}

Ogre::Entity* RenderManager::createEntity(const std::string& entityName, const std::string& meshName,
                                          const std::string& normalMapTextureName)
{
    std::cout << "RenderManager::createEntity(" << entityName << "," << meshName << "," << normalMapTextureName << ")" << std::endl;
    //TODO - has to be changed a bit, shaders shouldn't be generated here.
    Ogre::Entity* ent = mSceneManager->createEntity(entityName, meshName);

    Ogre::MeshPtr meshPtr = ent->getMesh();
    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }
    //Generate rtss shaders
    Ogre::Mesh::SubMeshIterator it = meshPtr->getSubMeshIterator();
    while (it.hasMoreElements())
    {
        Ogre::SubMesh* subMesh = it.getNext();
        LogManager::getSingleton().logMessage("Trying to generate shaders for material: " + subMesh->getMaterialName());
        generateRTSSShadersForMaterial(subMesh->getMaterialName(), normalMapTextureName);
    }
    return ent;
}

void RenderManager::colourizeEntity(Ogre::Entity *ent, Seat* seat, bool markedForDigging)
{
    //Disabled for normal mapping. This has to be implemented in some other way.

    // Colorize the the textures
    // Loop over the sub entities in the mesh
    if (seat == NULL && !markedForDigging)
        return;

    for (unsigned int i = 0; i < ent->getNumSubEntities(); ++i)
    {
        Ogre::SubEntity *tempSubEntity = ent->getSubEntity(i);
        tempSubEntity->setMaterialName(colourizeMaterial(tempSubEntity->getMaterialName(), seat, markedForDigging));
    }
}

std::string RenderManager::colourizeMaterial(const std::string& materialName, Seat* seat, bool markedForDigging)
{
    std::stringstream tempSS;
    Ogre::Technique *tempTechnique;
    Ogre::Pass *tempPass;

    tempSS.str("");
    int seatId = 0;
    if(seat != NULL)
        seatId = seat->getId();

    // Create the material name.
    if (seatId > 0)
        tempSS << "Color_" << seatId << "_" ;

    if (markedForDigging)
        tempSS << "dig_";

    tempSS << materialName;
    Ogre::MaterialPtr newMaterial = Ogre::MaterialPtr(Ogre::MaterialManager::getSingleton().getByName(tempSS.str()));

    //cout << "\nCloning material:  " << tempSS.str();

    // If this texture has been copied and colourized, we can return
    if (!newMaterial.isNull())
        return tempSS.str();

    // If not yet, then do so

    // Check to see if we find a seat with the requested color, if not then just use the original, uncolored material.
    if (seat == NULL && markedForDigging == false)
        return materialName;

    //std::cout << "\nMaterial does not exist, creating a new one.";
    newMaterial = Ogre::MaterialPtr(
                        Ogre::MaterialManager::getSingleton().getByName(materialName))->clone(tempSS.str());

    // Loop over the techniques for the new material
    for (unsigned int j = 0; j < newMaterial->getNumTechniques(); ++j)
    {
        tempTechnique = newMaterial->getTechnique(j);
        if (tempTechnique->getNumPasses() == 0)
            continue;

        if (markedForDigging)
        {
            // Color the material with yellow on the latest pass
            // so we're sure to see the taint.
            tempPass = tempTechnique->getPass(tempTechnique->getNumPasses() - 1);
            Ogre::ColourValue color(1.0, 1.0, 0.0, 0.3);
            tempPass->setEmissive(color);
            tempPass->setSpecular(color);
            tempPass->setAmbient(color);
            tempPass->setDiffuse(color);
        }
        else if (seatId > 0)
        {
            // Color the material with the Seat's color.
            tempPass = tempTechnique->getPass(0);
            Ogre::ColourValue color = seat->getColorValue();
            color.a = 0.3;
            tempPass->setEmissive(color);
            tempPass->setAmbient(color);
            // Remove the diffuse light to avoid the fluorescent effect.
            tempPass->setDiffuse(Ogre::ColourValue(0.0, 0.0, 0.0));
        }
    }

    return tempSS.str();

}
