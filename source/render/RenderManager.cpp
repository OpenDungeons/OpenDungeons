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

#include "render/RenderManager.h"

#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "rooms/Room.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/MapLight.h"
#include "entities/Creature.h"
#include "entities/Weapon.h"
#include "traps/Trap.h"
#include "game/Player.h"
#include "utils/ResourceManager.h"
#include "game/Seat.h"
#include "gamemap/MapLoader.h"
#include "entities/MovableGameEntity.h"

#include "utils/LogManager.h"
#include "entities/GameEntity.h"

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
    mHandAnimationState(nullptr),
    mViewport(nullptr),
    mShaderGenerator(nullptr),
    mInitialized(false)
{
    // Use Ogre::SceneType enum instead of string to identify the scene manager type; this is more robust!
    mSceneManager = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_INTERIOR, "SceneManager");
    mSceneManager->addRenderQueueListener(overlaySystem);

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
    Ogre::Entity* squareSelectorEnt = mSceneManager->createEntity("SquareSelector", "SquareSelector.mesh");
    Ogre::SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode("SquareSelectorNode");
    node->translate(Ogre::Vector3(0, 0, 0));
    node->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                              BLENDER_UNITS_PER_OGRE_UNIT, 0.45 * BLENDER_UNITS_PER_OGRE_UNIT));
    node->attachObject(squareSelectorEnt);
    Ogre::SceneNode *node2 = node->createChildSceneNode("Hand_node");
    node2->setPosition((Ogre::Real)(0.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                       (Ogre::Real)(0.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                       (Ogre::Real)(3.0 / BLENDER_UNITS_PER_OGRE_UNIT));
    node2->scale(Ogre::Vector3((Ogre::Real)(1.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                               (Ogre::Real)(1.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                               (Ogre::Real)(1.0 / BLENDER_UNITS_PER_OGRE_UNIT)));

    Ogre::Entity* keeperHandEnt = mSceneManager->createEntity("keeperHandEnt", "Keeperhand.mesh");
    mHandAnimationState = keeperHandEnt->getAnimationState("Walk");
    mHandAnimationState->setTimePosition(0);
    mHandAnimationState->setLoop(true);
    mHandAnimationState->setEnabled(true);

    Ogre::SceneNode* node3 = node->createChildSceneNode("KeeperHand_node");
    node3->setPosition((Ogre::Real)(0.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                       (Ogre::Real)(-1.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                       (Ogre::Real)(4.0 / BLENDER_UNITS_PER_OGRE_UNIT));
    node3->scale(Ogre::Vector3((Ogre::Real)(0.2 / BLENDER_UNITS_PER_OGRE_UNIT),
                               (Ogre::Real)(0.2 / BLENDER_UNITS_PER_OGRE_UNIT),
                               (Ogre::Real)(0.2 / BLENDER_UNITS_PER_OGRE_UNIT)));
    node3->attachObject(keeperHandEnt);

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

void RenderManager::updateRenderAnimations(Ogre::Real timeSinceLastFrame)
{
    if(mHandAnimationState == nullptr)
        return;

    mHandAnimationState->addTime(timeSinceLastFrame);
    if(mHandAnimationState->hasEnded())
    {
        Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
        mHandAnimationState = ent->getAnimationState("Walk");
        mHandAnimationState->setTimePosition(0);
        mHandAnimationState->setLoop(true);
    }

}

void RenderManager::rrRefreshTile(Tile* curTile, Player* localPlayer)
{
    int rt = 0;
    std::string tileName = curTile->getOgreNamePrefix() + curTile->getName();

    if (!mSceneManager->hasSceneNode(tileName + "_node"))
        return;

    if(mSceneManager->hasEntity(tileName))
    {
        // Unlink and delete the old mesh
        mSceneManager->getSceneNode(tileName + "_node")->detachObject(tileName);
        mSceneManager->destroyEntity(tileName);
    }

    bool colourizeTile = true;
    std::string meshName = curTile->getMeshName();
    if(meshName.empty())
    {
        // We compute the mesh
        meshName = Tile::meshNameFromNeighbors(curTile->getType(),
           curTile->getFullnessMeshNumber(),
           curTile->getGameMap()->getNeighborsTypes(curTile),
           curTile->getGameMap()->getNeighborsFullness(curTile),
           rt);
    }
    else
    {
        rt = 0;
        colourizeTile = false;
    }

    Ogre::Entity* ent = mSceneManager->createEntity(tileName, meshName);

    if(curTile->getType() == Tile::gold && curTile->getFullness() > 0.0)
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

    if(colourizeTile)
        colourizeEntity(ent, curTile->getSeat(), curTile->getMarkedForDigging(localPlayer));

    // Link the tile mesh back to the relevant scene node so OGRE will render it
    Ogre::SceneNode* node = mSceneManager->getSceneNode(tileName + "_node");
    node->attachObject(ent);
    node->setScale(curTile->getScale());
    node->resetOrientation();
    node->roll(Ogre::Degree((Ogre::Real)(-1 * rt * 90)));
}


void RenderManager::rrCreateTile(Tile* curTile, Player* localPlayer)
{
    int rt = 0;
    std::string meshName = Tile::meshNameFromNeighbors(curTile->getType(),
                                                       curTile->getFullnessMeshNumber(),
                                                       curTile->getGameMap()->getNeighborsTypes(curTile),
                                                       curTile->getGameMap()->getNeighborsFullness(curTile),
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
        colourizeEntity(ent, curTile->getSeat(), curTile->getMarkedForDigging(localPlayer));
    }

    Ogre::SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode(curTile->getOgreNamePrefix() + curTile->getName() + "_node");
    curTile->setParentSceneNode(node->getParentSceneNode());

    Ogre::MeshPtr meshPtr = ent->getMesh();
    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    node->setPosition(static_cast<Ogre::Real>(curTile->x), static_cast<Ogre::Real>(curTile->y), 0);

    node->attachObject(ent);
    curTile->setParentSceneNode(node->getParentSceneNode());
    curTile->setEntityNode(node);

    node->setScale(curTile->getScale());
    node->resetOrientation();
    node->roll(Ogre::Degree((Ogre::Real)(-1 * rt * 90)));
}

void RenderManager::rrDestroyTile(Tile* curTile)
{
    if (mSceneManager->hasEntity(curTile->getOgreNamePrefix() + curTile->getName()))
    {
        Ogre::Entity* ent = mSceneManager->getEntity(curTile->getOgreNamePrefix() + curTile->getName());
        Ogre::SceneNode* node = mSceneManager->getSceneNode(curTile->getOgreNamePrefix() + curTile->getName() + "_node");
        node->detachAllObjects();
        mSceneManager->destroySceneNode(node->getName());
        mSceneManager->destroyEntity(ent);
        curTile->setParentSceneNode(nullptr);
        curTile->setEntityNode(nullptr);
    }
}

void RenderManager::rrTemporalMarkTile(Tile* curTile)
{
    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    Ogre::Entity* ent;
    std::stringstream ss;
    std::stringstream ss2;

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

void RenderManager::rrDetachEntity(GameEntity* curEntity)
{
    Ogre::SceneNode* node = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");
    curEntity->getParentSceneNode()->removeChild(node);
}

void RenderManager::rrAttachEntity(GameEntity* curEntity)
{
    Ogre::SceneNode* entityNode = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");
    curEntity->getParentSceneNode()->addChild(entityNode);
}

void RenderManager::rrCreateRenderedMovableEntity(RenderedMovableEntity* renderedMovableEntity)
{
    std::string meshName = renderedMovableEntity->getMeshName();
    std::string tempString = renderedMovableEntity->getOgreNamePrefix() + renderedMovableEntity->getName();

    Ogre::Entity* ent = mSceneManager->createEntity(tempString, meshName + ".mesh");
    Ogre::SceneNode* node = mRoomSceneNode->createChildSceneNode(tempString + "_node");

    node->setPosition(renderedMovableEntity->getPosition());
    node->setScale(renderedMovableEntity->getScale());
    node->roll(Ogre::Degree(renderedMovableEntity->getRotationAngle()));
    node->attachObject(ent);

    renderedMovableEntity->setParentSceneNode(node->getParentSceneNode());
    renderedMovableEntity->setEntityNode(node);

    // If it is required, we hide the tile
    if(renderedMovableEntity->getHideCoveredTile())
    {
        Tile* posTile = renderedMovableEntity->getPositionTile();
        if(posTile == nullptr)
            return;

        std::string tileName = posTile->getOgreNamePrefix() + posTile->getName();
        if (!mSceneManager->hasEntity(tileName))
            return;

        Ogre::Entity* entity = mSceneManager->getEntity(tileName);
        entity->setVisible(false);
    }

    if (renderedMovableEntity->getOpacity() < 1.0f)
        setEntityOpacity(ent, renderedMovableEntity->getOpacity());
}

void RenderManager::rrDestroyRenderedMovableEntity(RenderedMovableEntity* curRenderedMovableEntity)
{
    std::string tempString = curRenderedMovableEntity->getOgreNamePrefix()
                             + curRenderedMovableEntity->getName();
    Ogre::Entity* ent = mSceneManager->getEntity(tempString);
    Ogre::SceneNode* node = mSceneManager->getSceneNode(tempString + "_node");
    node->detachObject(ent);
    mSceneManager->destroySceneNode(node->getName());
    mSceneManager->destroyEntity(ent);
    curRenderedMovableEntity->setParentSceneNode(nullptr);
    curRenderedMovableEntity->setEntityNode(nullptr);

    // If it was hidden, we display the tile
    if(curRenderedMovableEntity->getHideCoveredTile())
    {
        Tile* posTile = curRenderedMovableEntity->getPositionTile();
        if(posTile == nullptr)
            return;

        std::string tileName = posTile->getOgreNamePrefix() + posTile->getName();
        if (!mSceneManager->hasEntity(tileName))
            return;

        Ogre::Entity* entity = mSceneManager->getEntity(tileName);
        if (posTile->getCoveringBuilding() != nullptr)
            entity->setVisible(posTile->getCoveringBuilding()->shouldDisplayGroundTile());
        else
            entity->setVisible(true);
    }
}

void RenderManager::rrUpdateEntityOpacity(MovableGameEntity* entity)
{
    std::string entStr = entity->getOgreNamePrefix() + entity->getName();
    Ogre::Entity* ogreEnt = mSceneManager->hasEntity(entStr) ? mSceneManager->getEntity(entStr) : nullptr;
    if (ogreEnt == nullptr)
    {
        LogManager::getSingleton().logMessage("Update opacity: Couldn't find entity: " + entStr);
        return;
    }

    setEntityOpacity(ogreEnt, entity->getOpacity());
}

void RenderManager::rrCreateCreature(Creature* curCreature)
{
    const std::string& meshName = curCreature->getDefinition()->getMeshName();
    const Ogre::Vector3& scale = curCreature->getScale();

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
    curCreature->setEntityNode(node);
    node->setPosition(curCreature->getPosition());
    node->setScale(scale);
    node->attachObject(ent);
    curCreature->setParentSceneNode(node->getParentSceneNode());
}

void RenderManager::rrDestroyCreature(Creature* curCreature)
{
    std::string creatureName = curCreature->getOgreNamePrefix() + curCreature->getName();
    if (mSceneManager->hasEntity(creatureName))
    {
        Ogre::Entity* ent = mSceneManager->getEntity(creatureName);
        Ogre::SceneNode* node = mSceneManager->getSceneNode(creatureName + "_node");
        node->detachObject(ent);
        mCreatureSceneNode->removeChild(node);
        curCreature->setParentSceneNode(nullptr);
        curCreature->setEntityNode(nullptr);
        mSceneManager->destroyEntity(ent);
        mSceneManager->destroySceneNode(node->getName());
    }
}

void RenderManager::rrOrientEntityToward(MovableGameEntity* gameEntity, const Ogre::Vector3& direction)
{
    Ogre::SceneNode* node = mSceneManager->getSceneNode(gameEntity->getOgreNamePrefix() + gameEntity->getName() + "_node");
    Ogre::Vector3 tempVector = node->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Y;

    // Work around 180 degree quaternion rotation quirk
    if ((1.0f + tempVector.dotProduct(direction)) < 0.0001f)
    {
        node->roll(Ogre::Degree(180));
    }
    else
    {
        node->rotate(tempVector.getRotationTo(direction));
    }
}

void RenderManager::rrScaleEntity(GameEntity* entity)
{
    OD_ASSERT_TRUE_MSG(entity->getEntityNode() != nullptr, "entity=" + entity->getName());
    if(entity->getEntityNode() == nullptr)
        return;

    entity->getEntityNode()->setScale(entity->getScale());
}

void RenderManager::rrCreateWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand)
{
    Ogre::Entity* ent = mSceneManager->getEntity(curCreature->getOgreNamePrefix() + curCreature->getName());
    //colourizeEntity(ent, curCreature->color);
    Ogre::Entity* weaponEntity = mSceneManager->createEntity(curWeapon->getOgreNamePrefix()
                                 + hand + "_" + curCreature->getName(),
                                 curWeapon->getMeshName());
    Ogre::Bone* weaponBone = ent->getSkeleton()->getBone(
                                 curWeapon->getOgreNamePrefix() + hand);

    // Rotate by -90 degrees around the x-axis from the bone's rotation.
    Ogre::Quaternion rotationQuaternion;
    rotationQuaternion.FromAngleAxis(Ogre::Degree(-90.0), Ogre::Vector3(1.0,
                                     0.0, 0.0));

    ent->attachObjectToBone(weaponBone->getName(), weaponEntity,
                            rotationQuaternion);
}

void RenderManager::rrDestroyWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand)
{
     Ogre::Entity* ent = mSceneManager->getEntity(curWeapon->getOgreNamePrefix()
         + hand + "_" + curCreature->getName());
     mSceneManager->destroyEntity(ent);
}

void RenderManager::rrCreateMapLight(MapLight* curMapLight, bool displayVisual)
{
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
    curMapLight->setEntityNode(mapLightNode);
    curMapLight->setParentSceneNode(mapLightNode->getParentSceneNode());
    mapLightNode->setPosition(curMapLight->getPosition());

    if (displayVisual)
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
    curMapLight->setFlickerNode(flickerNode);
}

void RenderManager::rrDestroyMapLight(MapLight* curMapLight)
{
    std::string mapLightName = curMapLight->getOgreNamePrefix() + curMapLight->getName();
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

void RenderManager::rrDestroyMapLightVisualIndicator(MapLight* curMapLight)
{
    std::string mapLightName = curMapLight->getOgreNamePrefix() + curMapLight->getName();
    if (mSceneManager->hasLight(mapLightName))
    {
        Ogre::SceneNode* mapLightNode = mSceneManager->getSceneNode(mapLightName + "_node");
        std::string mapLightIndicatorName = MapLight::MAPLIGHT_INDICATOR_PREFIX
                                            + curMapLight->getName();
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

void RenderManager::rrPickUpEntity(MovableGameEntity* curEntity, Player* localPlayer)
{
    // Detach the entity from its scene node
    Ogre::SceneNode* curEntityNode = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");
    curEntity->getParentSceneNode()->removeChild(curEntityNode);

    // Attach the creature to the hand scene node
    mSceneManager->getSceneNode("Hand_node")->addChild(curEntityNode);
    Ogre::Vector3 scale = curEntity->getScale();
    scale *= 0.33;
    curEntityNode->setScale(scale);

    // Move the other creatures in the player's hand to make room for the one just picked up.
    int i = 0;
    const std::vector<MovableGameEntity*>& objectsInHand = localPlayer->getObjectsInHand();
    for (MovableGameEntity* tmpEntity : objectsInHand)
    {
        Ogre::SceneNode* tmpEntityNode = mSceneManager->getSceneNode(tmpEntity->getOgreNamePrefix() + tmpEntity->getName() + "_node");
        tmpEntityNode->setPosition((Ogre::Real)(i % 6 + 1), (Ogre::Real)(i / (int)6), (Ogre::Real)0.0);
        ++i;
    }
}

void RenderManager::rrDropHand(MovableGameEntity* curEntity, Player* localPlayer)
{
    // Detach the entity from the "hand" scene node
    Ogre::SceneNode* curEntityNode = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");
    mSceneManager->getSceneNode("Hand_node")->removeChild(curEntityNode);

    // Attach the creature from the creature scene node
    curEntity->getParentSceneNode()->addChild(curEntityNode);
    curEntityNode->setPosition(curEntity->getPosition());
    curEntityNode->setScale(curEntity->getScale());;

    // Move the other creatures in the player's hand to replace the dropped one
    int i = 0;
    const std::vector<MovableGameEntity*>& objectsInHand = localPlayer->getObjectsInHand();
    for (MovableGameEntity* tmpEntity : objectsInHand)
    {
        Ogre::SceneNode* tmpEntityNode = mSceneManager->getSceneNode(tmpEntity->getOgreNamePrefix() + tmpEntity->getName() + "_node");
        tmpEntityNode->setPosition((Ogre::Real)(i % 6 + 1), (Ogre::Real)(i / (int)6), (Ogre::Real)0.0);
        ++i;
    }
}

void RenderManager::rrRotateHand(Player* localPlayer)
{
    // Loop over the creatures in our hand and redraw each of them in their new location.
    int i = 0;
    const std::vector<MovableGameEntity*>& objectsInHand = localPlayer->getObjectsInHand();
    for (MovableGameEntity* tmpEntity : objectsInHand)
    {
        Ogre::SceneNode* tmpEntityNode = mSceneManager->getSceneNode(tmpEntity->getOgreNamePrefix() + tmpEntity->getName() + "_node");
        tmpEntityNode->setPosition((Ogre::Real)(i % 6 + 1), (Ogre::Real)(i / (int)6), (Ogre::Real)0.0);
        ++i;
    }
}

void RenderManager::rrCreateCreatureVisualDebug(Creature* curCreature, Tile* curTile)
{
    if (curTile != nullptr && curCreature != nullptr)
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

void RenderManager::rrDestroyCreatureVisualDebug(Creature* curCreature, Tile* curTile)
{
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

void RenderManager::rrCreateSeatVisionVisualDebug(int seatId, Tile* tile)
{
    if (tile != nullptr)
    {
        std::stringstream tempSS;
        tempSS << "Seat_Vision_indicator" << seatId << "_"
            << tile->x << "_" << tile->y;

        Ogre::Entity* visIndicatorEntity = mSceneManager->createEntity(tempSS.str(),
                                           "Cre_vision_indicator.mesh");
        Ogre::SceneNode* visIndicatorNode = mCreatureSceneNode->createChildSceneNode(tempSS.str()
                                            + "_node");
        visIndicatorNode->attachObject(visIndicatorEntity);
        visIndicatorNode->setPosition(Ogre::Vector3((Ogre::Real)tile->x, (Ogre::Real)tile->y, (Ogre::Real)0));
        visIndicatorNode->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT));
    }
}

void RenderManager::rrDestroySeatVisionVisualDebug(int seatId, Tile* tile)
{
    std::stringstream tempSS;
    tempSS << "Seat_Vision_indicator" << seatId << "_"
        << tile->x << "_" << tile->y;
    if (mSceneManager->hasEntity(tempSS.str()))
    {
        Ogre::Entity* visIndicatorEntity = mSceneManager->getEntity(tempSS.str());
        Ogre::SceneNode* visIndicatorNode = mSceneManager->getSceneNode(tempSS.str() + "_node");

        visIndicatorNode->detachAllObjects();
        mSceneManager->destroyEntity(visIndicatorEntity);
        mSceneManager->destroySceneNode(visIndicatorNode);
    }
}

void RenderManager::rrSetObjectAnimationState(MovableGameEntity* curAnimatedObject, const std::string& animation, bool loop)
{
    Ogre::Entity* objectEntity = mSceneManager->getEntity(
                                     curAnimatedObject->getOgreNamePrefix()
                                     + curAnimatedObject->getName());

    // Can't animate entities without skeleton
    if (!objectEntity->hasSkeleton())
        return;

    std::string anim = animation;

    // Handle the case where this entity does not have the requested animation.
    if (!objectEntity->getSkeleton()->hasAnimation(anim))
    {
        // Try to change the unexisting animation to a close existing one.
        if (anim == "Flee")
            anim = "Walk";
        else
            anim = "Idle";
    }

    if (objectEntity->getSkeleton()->hasAnimation(anim))
    {
        // Disable the animation for all of the animations on this entity.
        Ogre::AnimationStateIterator animationStateIterator(
            objectEntity->getAllAnimationStates()->getAnimationStateIterator());
        while (animationStateIterator.hasMoreElements())
        {
            animationStateIterator.getNext()->setEnabled(false);
        }

        curAnimatedObject->setAnimationState(objectEntity->getAnimationState(anim));
        curAnimatedObject->getAnimationState()->setTimePosition(0);
        curAnimatedObject->getAnimationState()->setLoop(loop);
        curAnimatedObject->getAnimationState()->setEnabled(true);
    }
}
void RenderManager::rrMoveEntity(GameEntity* entity, const Ogre::Vector3& position)
{
    OD_ASSERT_TRUE_MSG(entity->getEntityNode() != nullptr, "Entity do not have node=" + entity->getName());
    if(entity->getEntityNode() == nullptr)
        return;

    entity->getEntityNode()->setPosition(position);
}

void RenderManager::rrMoveMapLightFlicker(MapLight* mapLight, const Ogre::Vector3& position)
{
    OD_ASSERT_TRUE_MSG(mapLight->getFlickerNode() != nullptr, "MapLight do not have flicker=" + mapLight->getName());
    if(mapLight->getFlickerNode() == nullptr)
        return;

    mapLight->getFlickerNode()->setPosition(position);
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
        ret += "\nAnimation: " + animName;
    }

    Ogre::Skeleton::BoneIterator boneIterator = objectEntity->getSkeleton()->getBoneIterator();
    while (boneIterator.hasMoreElements())
    {
        std::string boneName = boneIterator.getNext()->getName();
        ret += "\nBone: " + boneName;
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
    //generateRTSSShadersForMaterial("Dormitory", "Dirt_dark_nor3.png");
    //TODO - fix this model so it doesn't use the material name 'material'
    generateRTSSShadersForMaterial("Material", "Forge_normalmap.png");
    generateRTSSShadersForMaterial("Troll2", "Troll2_nor2.png");
    generateRTSSShadersForMaterial("Kobold_skin/TEXFACE/kobold_skin6.png");
    generateRTSSShadersForMaterial("Kobold_skin/TWOSIDE/TEXFACE/kobold_skin6.png");
    generateRTSSShadersForMaterial("Wizard/TWOSIDE", "Wizard_nor.png");
    generateRTSSShadersForMaterial("Wizard", "Wizard_nor.png");
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

    // Create the material name.
    if(seat != nullptr)
        tempSS << "Color_" << seat->getColorId() << "_" ;
    else
        tempSS << "Color_0_" ;

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
        else if (seat != nullptr)
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

void RenderManager::rrCarryEntity(Creature* carrier, MovableGameEntity* carried)
{
    Ogre::Entity* carrierEnt = mSceneManager->getEntity(carrier->getOgreNamePrefix() + carrier->getName());
    Ogre::Entity* carriedEnt = mSceneManager->getEntity(carried->getOgreNamePrefix() + carried->getName());
    Ogre::SceneNode* carrierNode = mSceneManager->getSceneNode(carrierEnt->getName() + "_node");
    Ogre::SceneNode* carriedNode = mSceneManager->getSceneNode(carriedEnt->getName() + "_node");
    carried->getParentSceneNode()->removeChild(carriedNode);
    carriedNode->setInheritScale(false);
    carriedNode->setPosition(carrierNode->getPosition());
    carrierNode->addChild(carriedNode);
}

void RenderManager::rrReleaseCarriedEntity(Creature* carrier, MovableGameEntity* carried)
{
    Ogre::Entity* carrierEnt = mSceneManager->getEntity(carrier->getOgreNamePrefix() + carrier->getName());
    Ogre::Entity* carriedEnt = mSceneManager->getEntity(carried->getOgreNamePrefix() + carried->getName());
    Ogre::SceneNode* carrierNode = mSceneManager->getSceneNode(carrierEnt->getName() + "_node");
    Ogre::SceneNode* carriedNode = mSceneManager->getSceneNode(carriedEnt->getName() + "_node");
    carrierNode->removeChild(carriedNode);
    carried->getParentSceneNode()->addChild(carriedNode);
    carriedNode->setInheritScale(true);
}

void RenderManager::setEntityOpacity(Ogre::Entity* ent, float opacity)
{
    for (unsigned int i = 0; i < ent->getNumSubEntities(); ++i)
    {
        Ogre::SubEntity* subEntity = ent->getSubEntity(i);
        subEntity->setMaterialName(setMaterialOpacity(subEntity->getMaterialName(), opacity));
    }
}

std::string RenderManager::setMaterialOpacity(const std::string& materialName, float opacity)
{
    if (opacity < 0.0f || opacity > 1.0f)
        return materialName;

    std::stringstream newMaterialName;
    newMaterialName.str("");

    // Check whether the material name has alreay got an _alpha_ suffix and remove it.
    size_t alphaPos = materialName.find("_alpha_");
    // Create the material name accordingly.
    if (alphaPos == std::string::npos)
        newMaterialName << materialName;
    else
        newMaterialName << materialName.substr(0, alphaPos);

    // Only precise the opactiy when its useful, otherwise give the original material name.
    if (opacity != 1.0f)
        newMaterialName << "_alpha_" << static_cast<int>(opacity * 255.0f);

    Ogre::MaterialPtr newMaterial = Ogre::MaterialPtr(Ogre::MaterialManager::getSingleton().getByName(newMaterialName.str()));

    // If this texture has been copied and colourized, we can return
    if (!newMaterial.isNull())
        return newMaterialName.str();

    // If not yet, then do so
    newMaterial = Ogre::MaterialPtr(Ogre::MaterialManager::getSingleton().getByName(materialName))->clone(newMaterialName.str());

    // Loop over the techniques for the new material
    for (unsigned int j = 0; j < newMaterial->getNumTechniques(); ++j)
    {
        Ogre::Technique* technique = newMaterial->getTechnique(j);
        if (technique->getNumPasses() == 0)
            continue;

        // Color the material with yellow on the latest pass
        // so we're sure to see the taint.
        Ogre::Pass* pass = technique->getPass(technique->getNumPasses() - 1);
        Ogre::ColourValue color = pass->getEmissive();
        color.a = opacity;
        pass->setEmissive(color);

        color = pass->getSpecular();
        color.a = opacity;
        pass->setSpecular(color);

        color = pass->getAmbient();
        color.a = opacity;
        pass->setAmbient(color);

        color = pass->getDiffuse();
        color.a = opacity;
        pass->setDiffuse(color);

        if (opacity < 1.0f)
        {
            pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
            pass->setDepthWriteEnabled(false);
        }
        else
        {
            // Use sane default, but this should never happen...
            pass->setSceneBlending(Ogre::SBT_MODULATE);
            pass->setDepthWriteEnabled(true);
        }
    }

    return newMaterialName.str();
}

void RenderManager::moveCursor(Ogre::Real x, Ogre::Real y)
{
    mSceneManager->getSceneNode("SquareSelectorNode")->setPosition(x, y, 0.0);
    Ogre::Light* mouseLight = mSceneManager->getLight("MouseLight");
    mouseLight->setPosition(x, y, 2.0);
}

void RenderManager::entitySlapped()
{
    Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
    mHandAnimationState = ent->getAnimationState("Idle");
    mHandAnimationState->setTimePosition(0);
    mHandAnimationState->setLoop(false);
    mHandAnimationState->setEnabled(true);
}
