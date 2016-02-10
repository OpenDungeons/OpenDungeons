/*!
 *  \file   RenderManager.cpp
 *  \date   26 March 2001
 *  \author oln, paul424
 *  \brief  handles the render requests
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

#include "render/RenderManager.h"

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/GameEntity.h"
#include "entities/MapLight.h"
#include "entities/MovableGameEntity.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "entities/Weapon.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/TileSet.h"
#include "render/CreatureOverlayStatus.h"
#include "rooms/Room.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/ResourceManager.h"

#include <OgreBone.h>
#include <OgreCamera.h>
#include <OgreCompositorManager.h>
#include <OgreEntity.h>
#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreMovableObject.h>
#include <OgreParticleSystem.h>
#include <OgreQuaternion.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreSubEntity.h>
#include <OgreSubMesh.h>
#include <OgreRoot.h>
#include <OgreTechnique.h>
#include <OgreViewport.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlaySystem.h>
#include <RTShaderSystem/OgreShaderGenerator.h>

#include <sstream>

template<> RenderManager* Ogre::Singleton<RenderManager>::msSingleton = nullptr;

const uint8_t RenderManager::OD_RENDER_QUEUE_ID_GUI = 101;

const Ogre::Real RenderManager::BLENDER_UNITS_PER_OGRE_UNIT = 10.0f;

const Ogre::Real KEEPER_HAND_POS_Z = 20.0;
const Ogre::Real RenderManager::KEEPER_HAND_WORLD_Z = KEEPER_HAND_POS_Z / RenderManager::BLENDER_UNITS_PER_OGRE_UNIT;

const Ogre::Real KEEPER_HAND_CREATURE_PICKED_OFFSET = 5.0;
const Ogre::Real KEEPER_HAND_CREATURE_PICKED_SCALE = 5.0;

RenderManager::RenderManager(Ogre::OverlaySystem* overlaySystem) :
    mHandAnimationState(nullptr),
    mViewport(nullptr),
    mShaderGenerator(nullptr),
    mHandKeeperNode(nullptr),
    mHandLight(nullptr),
    mCurrentFOVy(0.0f),
    mFactorWidth(0.0f),
    mFactorHeight(0.0f),
    mCreatureTextOverlayDisplayed(false),
    mHandKeeperHandVisibility(0)
{
    // Use Ogre::SceneType enum instead of string to identify the scene manager type; this is more robust!
    mSceneManager = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_INTERIOR, "SceneManager");
    mSceneManager->addRenderQueueListener(overlaySystem);

    mCreatureSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("Creature_scene_node");
    mTileSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("Tile_scene_node");
    mRoomSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("Room_scene_node");
    mLightSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("Light_scene_node");
    mMainMenuSceneNode = mSceneManager->getRootSceneNode()->createChildSceneNode("MainMenu_scene_node");
}

RenderManager::~RenderManager()
{
}

void RenderManager::initGameRenderer(GameMap* gameMap)
{
    mCreatureTextOverlayDisplayed = false;

    // Create the light which follows the single tile selection mesh
    if(mHandLight == nullptr)
    {
        mHandLight = mSceneManager->createLight("MouseLight");
        mHandLight->setType(Ogre::Light::LT_POINT);
        mHandLight->setDiffuseColour(Ogre::ColourValue(0.65, 0.65, 0.45));
        mHandLight->setSpecularColour(Ogre::ColourValue(0.65, 0.65, 0.45));
        mHandLight->setAttenuation(7, 1.0, 0.00, 0.3);
    }

    //Add a too small to be visible dummy dirt tile to the hand node
    //so that there will always be a dirt tile "visible"
    //This is an ugly workaround for issue where destroying some entities messes
    //up the lighing for some of the rtshader materials.
    const std::string& defaultTileMesh = gameMap->getMeshForDefaultTile();
    if(!mSceneManager->hasEntity(defaultTileMesh + "_dummyEnt"))
    {
        Ogre::SceneNode* dummyNode = mHandKeeperNode->createChildSceneNode(defaultTileMesh + "_dummyNode");
        dummyNode->setScale(Ogre::Vector3(0.00000001f, 0.00000001f, 0.00000001f));
        Ogre::Entity* dummyEnt = mSceneManager->createEntity(defaultTileMesh + "_dummyEnt", defaultTileMesh);
        dummyEnt->setLightMask(0);
        dummyEnt->setCastShadows(false);
        dummyNode->attachObject(dummyEnt);
        mDummyEntities.push_back(dummyNode);
    }

    // We load every creature class and attach them to the keeper hand.
    // That's an ugly workaround to avoid a crash that occurs when CEGUI refreshes
    // ogre open gl renderer after removing some creatures
    // Note that from what I've seen, loading only one creature like "Troll.mesh" should be
    // enough. However, it doesn't work with other creatures (like "Kobold.mesh"). Since we
    // don't really know why, it is safer to load every creature
    for(uint32_t i = 0; i < gameMap->numClassDescriptions(); ++i)
    {
        const CreatureDefinition* def = gameMap->getClassDescription(i);
        // We check if the mesh is already loaded. If not, we load it
        if(mSceneManager->hasEntity(def->getClassName() + "_dummyEnt"))
            continue;

        Ogre::SceneNode* dummyNode = mHandKeeperNode->createChildSceneNode(def->getClassName() + "_dummyNode");
        dummyNode->setScale(Ogre::Vector3(0.00000001f, 0.00000001f, 0.00000001f));
        Ogre::Entity* dummyEnt = mSceneManager->createEntity(def->getClassName() + "_dummyEnt", def->getMeshName());
        dummyEnt->setLightMask(0);
        dummyEnt->setCastShadows(false);
        dummyNode->attachObject(dummyEnt);
        mDummyEntities.push_back(dummyNode);
    }
}

void RenderManager::stopGameRenderer(GameMap* gameMap)
{
    // We do not remove the entities from mDummyEntities as it is a workaround avoiding a crash and removing
    // them can cause the crash to happen

    // Remove the light following the keeper hand
    if(mHandLight != nullptr)
    {
        mSceneManager->destroyLight(mHandLight);
        mHandLight = nullptr;
    }
}

void RenderManager::triggerCompositor(const std::string& compositorName)
{
    Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, compositorName.c_str(), true);
}

void RenderManager::createScene(Ogre::Viewport* nViewport)
{
    OD_LOG_INF("Creating scene...");

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

    // Sets the overall world lighting.
    mSceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));

    Ogre::ParticleSystem::setDefaultNonVisibleUpdateTimeout(5);

    // Create the nodes that will follow the mouse pointer.
    Ogre::Entity* keeperHandEnt = mSceneManager->createEntity("keeperHandEnt", "Keeperhand.mesh");
    keeperHandEnt->setLightMask(0);
    keeperHandEnt->setCastShadows(false);
    mHandAnimationState = keeperHandEnt->getAnimationState("Idle");
    mHandAnimationState->setTimePosition(0);
    mHandAnimationState->setLoop(true);
    mHandAnimationState->setEnabled(true);
    // Note that we need to render something on OD_RENDER_QUEUE_ID_GUI otherwise, Ogre
    // will not call the render function with queue id = OD_RENDER_QUEUE_ID_GUI and the
    // GUI will not be displayed
    keeperHandEnt->setRenderQueueGroup(OD_RENDER_QUEUE_ID_GUI);

    Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
    Ogre::Overlay* handKeeperOverlay = overlayManager.create(keeperHandEnt->getName() + "_Ov");
    mHandKeeperNode = mSceneManager->createSceneNode(keeperHandEnt->getName() + "_node");
    mHandKeeperNode->attachObject(keeperHandEnt);
    mHandKeeperNode->setScale(Ogre::Vector3::UNIT_SCALE * 0.012f * KEEPER_HAND_POS_Z);
    mHandKeeperNode->setPosition(0.0f, 0.0f, -KEEPER_HAND_POS_Z);
    handKeeperOverlay->add3D(mHandKeeperNode);
    handKeeperOverlay->show();

    mHandKeeperNode->setVisible(mHandKeeperHandVisibility == 0);
}

Ogre::Light* RenderManager::addPointLightMenu(const std::string& name, const Ogre::Vector3& pos,
        const Ogre::ColourValue& diffuse, const Ogre::ColourValue& specular, Ogre::Real attenuationRange,
        Ogre::Real attenuationConstant, Ogre::Real attenuationLinear, Ogre::Real attenuationQuadratic)
{
    if(mSceneManager->hasLight(name))
    {
        OD_LOG_ERR("There is already a light=" + name);
        return nullptr;
    }

    Ogre::Light* light = mSceneManager->createLight(name);
    light->setType(Ogre::Light::LT_POINT);
    light->setDiffuseColour(diffuse);
    light->setSpecularColour(specular);
    light->setAttenuation(attenuationRange, attenuationConstant, attenuationLinear, attenuationQuadratic);
    return light;
}

void RenderManager::removePointLightMenu(Ogre::Light* light)
{
    mSceneManager->destroyLight(light);
}

Ogre::Entity* RenderManager::addEntityMenu(const std::string& meshName, const std::string& entityName,
        const Ogre::Vector3& scale, const Ogre::Vector3& pos)
{
    if(mSceneManager->hasEntity(entityName))
    {
        OD_LOG_ERR("There is already an entity=" + entityName);
        return nullptr;
    }

    Ogre::Entity* ent = mSceneManager->createEntity(entityName, meshName);
    Ogre::MeshPtr meshPtr = ent->getMesh();
    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    Ogre::SceneNode* node = mMainMenuSceneNode->createChildSceneNode(ent->getName() + "_node");
    node->attachObject(ent);
    node->setScale(scale);
    node->setPosition(pos);

    return ent;
}

void RenderManager::removeEntityMenu(Ogre::Entity* ent)
{
    Ogre::SceneNode* entNode = mSceneManager->getSceneNode(ent->getName() + "_node");
    entNode->detachObject(ent);
    mMainMenuSceneNode->removeChild(entNode);
    mSceneManager->destroyEntity(ent);
    mSceneManager->destroySceneNode(entNode->getName());
}

Ogre::AnimationState* RenderManager::setMenuEntityAnimation(const std::string& entityName, const std::string& animation, bool loop)
{
    if(!mSceneManager->hasEntity(entityName))
    {
        OD_LOG_ERR("There is no entity=" + entityName);
        return nullptr;
    }

    Ogre::Entity* ent = mSceneManager->getEntity(entityName);
    if(!ent->hasAnimationState(animation))
    {
        OD_LOG_ERR("Entity=" + ent->getName() + ", has no animation=" + animation);
        return nullptr;
    }

    return setEntityAnimation(ent, animation, loop);
}

bool RenderManager::updateMenuEntityAnimation(Ogre::AnimationState* animState, Ogre::Real timeSinceLastFrame)
{
    animState->addTime(timeSinceLastFrame);
    return animState->hasEnded();
}

Ogre::SceneNode* RenderManager::getMenuEntityNode(const std::string& entityName, Ogre::Vector3& pos)
{
    std::string nodeName = entityName + "_node";
    if(!mSceneManager->hasSceneNode(nodeName))
    {
        OD_LOG_ERR("No node for entityName=" + entityName + ", node name=" + nodeName);
        return nullptr;
    }

    Ogre::SceneNode* node = mSceneManager->getSceneNode(nodeName);
    pos = node->getPosition();
    return node;
}

void RenderManager::updateMenuEntityPosition(Ogre::SceneNode* node, const Ogre::Vector3& pos)
{
    node->setPosition(pos);
}

void RenderManager::orientMenuEntityPosition(Ogre::SceneNode* node, const Ogre::Vector3& direction)
{
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

Ogre::ParticleSystem* RenderManager::addEntityParticleEffectMenu(Ogre::SceneNode* node,
        const std::string& particleName, const std::string& particleScript)
{
    Ogre::ParticleSystem* particleSystem = mSceneManager->createParticleSystem(particleName, particleScript);

    node->attachObject(particleSystem);

    return particleSystem;
}

void RenderManager::removeEntityParticleEffectMenu(Ogre::SceneNode* node,
        Ogre::ParticleSystem* particleSystem)
{
    node->detachObject(particleSystem);
    mSceneManager->destroyParticleSystem(particleSystem);
}

void RenderManager::updateRenderAnimations(Ogre::Real timeSinceLastFrame)
{
    if(mHandAnimationState != nullptr)
    {
        mHandAnimationState->addTime(timeSinceLastFrame);
        if(mHandAnimationState->hasEnded())
        {
            Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
            mHandAnimationState = setEntityAnimation(ent, "Idle", true);
        }
    }
}

void RenderManager::rrRefreshTile(const Tile& tile, const GameMap& gameMap, const Player& localPlayer)
{
    if (tile.getEntityNode() == nullptr)
        return;

    std::string tileName = tile.getOgreNamePrefix() + tile.getName();
    bool displayTilesetMesh = tile.shouldDisplayTileMesh();
    std::string meshName;

    // We only mark vision on ground tiles (except lava and water)
    bool vision = true;
    switch(tile.getTileVisual())
    {
        case TileVisual::claimedGround:
        case TileVisual::dirtGround:
        case TileVisual::goldGround:
        case TileVisual::rockGround:
            vision = tile.getLocalPlayerHasVision();
            break;
        default:
            break;
    }

    bool isMarked = tile.getMarkedForDigging(&localPlayer);
    const TileSetValue& tileSetValue = gameMap.getMeshForTile(&tile);

    // We display the tile mesh if needed
    if(displayTilesetMesh)
    {
        // We compute the mesh
        meshName = tileSetValue.getMeshName();
    }

    Ogre::Entity* tileMeshEnt = nullptr;
    const std::string tileMeshName = tileName + "_tileMesh";
    if(mSceneManager->hasEntity(tileMeshName))
    {
        tileMeshEnt = mSceneManager->getEntity(tileMeshName);
        if(tileMeshEnt->getMesh()->getName().compare(meshName) != 0)
        {
            // Unlink and delete the old mesh
            mSceneManager->getSceneNode(tileMeshName + "_node")->detachObject(tileMeshEnt);
            mSceneManager->destroyEntity(tileMeshEnt);
            tileMeshEnt = nullptr;
        }
    }

    Ogre::SceneNode* tileMeshNode = nullptr;
    std::string tileMeshNodeName = tileMeshName + "_node";
    if(mSceneManager->hasSceneNode(tileMeshNodeName))
        tileMeshNode = mSceneManager->getSceneNode(tileMeshNodeName);

    if((tileMeshEnt == nullptr) && !meshName.empty())
    {
        tileMeshEnt = mSceneManager->createEntity(tileMeshName, meshName);
        // If the node does not exist, we create it
        if(tileMeshNode == nullptr)
            tileMeshNode = tile.getEntityNode()->createChildSceneNode(tileMeshNodeName);
        // Link the tile mesh back to the relevant scene node so OGRE will render it
        tileMeshNode->attachObject(tileMeshEnt);

        Ogre::MeshPtr meshPtr = tileMeshEnt->getMesh();
        unsigned short src, dest;
        if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
        {
            meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
        }
    }

    // We rescale and set the orientation that may have changed
    if(tileMeshNode != nullptr)
    {
        tileMeshNode->setScale(gameMap.getTileSetScale());
        tileMeshNode->resetOrientation();

        // We rotate depending on the tileset
        Ogre::Quaternion q;
        if(tileSetValue.getRotationX() != 0.0)
            q = q * Ogre::Quaternion(Ogre::Degree(tileSetValue.getRotationX()), Ogre::Vector3::UNIT_X);

        if(tileSetValue.getRotationY() != 0.0)
            q = q * Ogre::Quaternion(Ogre::Degree(tileSetValue.getRotationY()), Ogre::Vector3::UNIT_Y);

        if(tileSetValue.getRotationZ() != 0.0)
            q = q * Ogre::Quaternion(Ogre::Degree(tileSetValue.getRotationZ()), Ogre::Vector3::UNIT_Z);

        if(q != Ogre::Quaternion::IDENTITY)
            tileMeshNode->rotate(q);
    }

    if(tileMeshEnt != nullptr)
    {
        // We replace the material if required by the tileset
        if(!tileSetValue.getMaterialName().empty())
            tileMeshEnt->setMaterialName(tileSetValue.getMaterialName());

        Seat* seatColor = nullptr;
        if(tile.shouldColorTileMesh())
            seatColor = tile.getSeat();

        colourizeEntity(tileMeshEnt, seatColor, isMarked, vision);
    }

    // We display the custom mesh if there is one
    const std::string customMeshName = tileName + "_customMesh";
    meshName = tile.getMeshName();
    Ogre::Entity* customMeshEnt = nullptr;
    if(mSceneManager->hasEntity(customMeshName))
    {
        customMeshEnt = mSceneManager->getEntity(customMeshName);
        if(customMeshEnt->getMesh()->getName().compare(meshName) != 0)
        {
            // Unlink and delete the old mesh
            mSceneManager->getSceneNode(customMeshName + "_node")->detachObject(customMeshEnt);
            mSceneManager->destroyEntity(customMeshEnt);
            customMeshEnt = nullptr;
        }
    }

    if((customMeshEnt == nullptr) && !meshName.empty())
    {
        // If the node does not exist, we create it
        std::string customMeshNodeName = customMeshName + "_node";
        Ogre::SceneNode* customMeshNode;
        if(!mSceneManager->hasSceneNode(customMeshNodeName))
            customMeshNode = tile.getEntityNode()->createChildSceneNode(customMeshNodeName);
        else
            customMeshNode = mSceneManager->getSceneNode(customMeshNodeName);

        customMeshEnt = mSceneManager->createEntity(customMeshName, meshName);

        customMeshNode->attachObject(customMeshEnt);
        customMeshNode->setScale(tile.getScale());
        customMeshNode->resetOrientation();

        Ogre::MeshPtr meshPtr = customMeshEnt->getMesh();
        unsigned short src, dest;
        if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
        {
            meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
        }
    }

    if(customMeshEnt != nullptr)
    {
        Seat* seatColor = nullptr;
        if(tile.shouldColorCustomMesh())
            seatColor = tile.getSeat();

        colourizeEntity(customMeshEnt, seatColor, isMarked, vision);
    }
}

void RenderManager::rrCreateTile(Tile& tile, const GameMap& gameMap, const Player& localPlayer)
{
    std::string tileName = tile.getOgreNamePrefix() + tile.getName();
    Ogre::SceneNode* node = mTileSceneNode->createChildSceneNode(tileName + "_node");
    tile.setParentSceneNode(node->getParentSceneNode());
    tile.setEntityNode(node);
    node->setPosition(static_cast<Ogre::Real>(tile.getX()), static_cast<Ogre::Real>(tile.getY()), 0);

    rrRefreshTile(tile, gameMap, localPlayer);
}

void RenderManager::rrDestroyTile(Tile& tile)
{
    if (tile.getEntityNode() == nullptr)
        return;

    std::string tileName = tile.getOgreNamePrefix() + tile.getName();
    std::string selectorName = tileName + "_selection_indicator";
    if(mSceneManager->hasEntity(selectorName))
    {
        Ogre::SceneNode* selectorNode = mSceneManager->getSceneNode(selectorName + "Node");
        Ogre::Entity* selectorEnt = mSceneManager->getEntity(selectorName);
        tile.getEntityNode()->removeChild(selectorNode);
        selectorNode->detachObject(selectorEnt);
        mSceneManager->destroySceneNode(selectorNode);
        mSceneManager->destroyEntity(selectorEnt);
    }

    const std::string tileMeshName = tileName + "_tileMesh";
    if(mSceneManager->hasSceneNode(tileMeshName + "_node"))
    {
        Ogre::SceneNode* tileMeshNode = mSceneManager->getSceneNode(tileMeshName + "_node");
        if(mSceneManager->hasEntity(tileMeshName))
        {
            Ogre::Entity* ent = mSceneManager->getEntity(tileMeshName);
            tileMeshNode->detachObject(ent);
            mSceneManager->destroyEntity(ent);
        }
        tile.getEntityNode()->removeChild(tileMeshNode);
        mSceneManager->destroySceneNode(tileMeshNode);
    }

    const std::string customMeshName = tileName + "_customMesh";
    if(mSceneManager->hasSceneNode(customMeshName + "_node"))
    {
        Ogre::SceneNode* customMeshNode = mSceneManager->getSceneNode(customMeshName + "_node");
        if(mSceneManager->hasEntity(customMeshName))
        {
            Ogre::Entity* ent = mSceneManager->getEntity(customMeshName);
            customMeshNode->detachObject(ent);
            mSceneManager->destroyEntity(ent);
        }
        tile.getEntityNode()->removeChild(customMeshNode);
        mSceneManager->destroySceneNode(customMeshNode);
    }

    mSceneManager->destroySceneNode(tile.getEntityNode());
    tile.setParentSceneNode(nullptr);
    tile.setEntityNode(nullptr);
}

void RenderManager::rrTemporalMarkTile(Tile* curTile)
{
    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    Ogre::Entity* ent;

    bool bb = curTile->getSelected();

    std::string tileName = curTile->getOgreNamePrefix() + curTile->getName();
    std::string selectorName = tileName + "_selection_indicator";
    if (mSceneMgr->hasEntity(selectorName))
    {
        ent = mSceneMgr->getEntity(selectorName);
    }
    else
    {
        std::string tileNodeName = tileName + "_node";
        ent = mSceneMgr->createEntity(selectorName, "SquareSelector.mesh");
        ent->setLightMask(0);
        ent->setCastShadows(false);
        Ogre::SceneNode* tileNode = mSceneManager->getSceneNode(tileNodeName);
        Ogre::SceneNode* selectorNode = tileNode->createChildSceneNode(selectorName + "Node");
        selectorNode->setInheritScale(false);
        selectorNode->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                  BLENDER_UNITS_PER_OGRE_UNIT, 0.45 * BLENDER_UNITS_PER_OGRE_UNIT));
        selectorNode->attachObject(ent);
    }

    ent->setVisible(bb);
}

void RenderManager::rrCreateRenderedMovableEntity(RenderedMovableEntity* renderedMovableEntity)
{
    std::string meshName = renderedMovableEntity->getMeshName();
    std::string tempString = renderedMovableEntity->getOgreNamePrefix() + renderedMovableEntity->getName();

    Ogre::SceneNode* node = mRoomSceneNode->createChildSceneNode(tempString + "_node");

    node->setPosition(renderedMovableEntity->getPosition());
    node->setScale(renderedMovableEntity->getScale());
    node->roll(Ogre::Degree(renderedMovableEntity->getRotationAngle()));
    Ogre::Entity* ent = nullptr;
    if(!meshName.empty())
    {
        ent = mSceneManager->createEntity(tempString, meshName + ".mesh");
        node->attachObject(ent);
    }

    renderedMovableEntity->setParentSceneNode(node->getParentSceneNode());
    renderedMovableEntity->setEntityNode(node);

    // If it is required, we hide the tile
    if((renderedMovableEntity->getHideCoveredTile()) &&
       (renderedMovableEntity->getOpacity() >= 1.0))
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

    if ((ent != nullptr) && (renderedMovableEntity->getOpacity() < 1.0f))
        setEntityOpacity(ent, renderedMovableEntity->getOpacity());
}

void RenderManager::rrDestroyRenderedMovableEntity(RenderedMovableEntity* curRenderedMovableEntity)
{
    std::string tempString = curRenderedMovableEntity->getOgreNamePrefix()
                             + curRenderedMovableEntity->getName();
    Ogre::SceneNode* node = curRenderedMovableEntity->getEntityNode();
    if(mSceneManager->hasEntity(tempString))
    {
        Ogre::Entity* ent = mSceneManager->getEntity(tempString);
        node->detachObject(ent);
        mSceneManager->destroyEntity(ent);
    }
    mSceneManager->destroySceneNode(node);
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

void RenderManager::rrUpdateEntityOpacity(RenderedMovableEntity* entity)
{
    std::string entStr = entity->getOgreNamePrefix() + entity->getName();
    Ogre::Entity* ogreEnt = mSceneManager->hasEntity(entStr) ? mSceneManager->getEntity(entStr) : nullptr;
    if (ogreEnt == nullptr)
    {
        OD_LOG_INF("Update opacity: Couldn't find entity: " + entStr);
        return;
    }

    setEntityOpacity(ogreEnt, entity->getOpacity());

    // We add the tile if it is required and the opacity is 1. Otherwise, we show it (in case the trap gets deactivated)
    bool tileVisible = (!entity->getHideCoveredTile() || (entity->getOpacity() < 1.0f));
    Tile* posTile = entity->getPositionTile();
    if(posTile != nullptr)
    {
        std::string tileName = posTile->getOgreNamePrefix() + posTile->getName();
        if (mSceneManager->hasEntity(tileName))
        {
            Ogre::Entity* ogreEntity = mSceneManager->getEntity(tileName);
            ogreEntity->setVisible(tileVisible);
        }
    }
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

    Ogre::SceneNode* node = mCreatureSceneNode->createChildSceneNode(creatureName + "_node");
    curCreature->setEntityNode(node);
    node->setPosition(curCreature->getPosition());
    node->setScale(scale);
    node->attachObject(ent);
    curCreature->setParentSceneNode(node->getParentSceneNode());

    Ogre::Camera* cam = mViewport->getCamera();
    CreatureOverlayStatus* creatureOverlay = new CreatureOverlayStatus(curCreature, ent, cam);
    curCreature->setOverlayStatus(creatureOverlay);

    creatureOverlay->displayHealthOverlay(mCreatureTextOverlayDisplayed ? -1.0 : 0.0);
}

void RenderManager::rrDestroyCreature(Creature* curCreature)
{
    if(curCreature->getOverlayStatus() != nullptr)
    {
        delete curCreature->getOverlayStatus();
        curCreature->setOverlayStatus(nullptr);
    }

    std::string creatureName = curCreature->getOgreNamePrefix() + curCreature->getName();
    if (mSceneManager->hasEntity(creatureName))
    {
        Ogre::SceneNode* creatureNode = curCreature->getEntityNode();
        Ogre::Entity* ent = mSceneManager->getEntity(creatureName);
        creatureNode->detachObject(ent);
        mCreatureSceneNode->removeChild(creatureNode);
        curCreature->setParentSceneNode(nullptr);
        curCreature->setEntityNode(nullptr);
        mSceneManager->destroyEntity(ent);
        mSceneManager->destroySceneNode(creatureNode->getName());
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
    if(entity->getEntityNode() == nullptr)
    {
        OD_LOG_ERR("entity=" + entity->getName());
        return;
    }

    entity->getEntityNode()->setScale(entity->getScale());
}

void RenderManager::rrCreateWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand)
{
    Ogre::Entity* ent = mSceneManager->getEntity(curCreature->getOgreNamePrefix() + curCreature->getName());
    std::string weaponName = curWeapon->getOgreNamePrefix() + hand;
    if(!ent->getSkeleton()->hasBone(weaponName))
    {
        OD_LOG_WRN("Tried to add weapons to entity \"" + ent->getName() + " \" using model \"" +
                              ent->getMesh()->getName() + "\" that is missing the required bone \"" +
                              curWeapon->getOgreNamePrefix() + hand + "\"");
        return;
    }
    Ogre::Bone* weaponBone = ent->getSkeleton()->getBone(
                                curWeapon->getOgreNamePrefix() + hand);
    Ogre::Entity* weaponEntity = mSceneManager->createEntity(curWeapon->getOgreNamePrefix()
                                + hand + "_" + curCreature->getName(),
                                curWeapon->getMeshName());

    // Rotate by -90 degrees around the x-axis from the bone's rotation.
    Ogre::Quaternion rotationQuaternion;
    rotationQuaternion.FromAngleAxis(Ogre::Degree(-90.0), Ogre::Vector3(1.0,
                                    0.0, 0.0));

    ent->attachObjectToBone(weaponBone->getName(), weaponEntity,
                            rotationQuaternion);
}

void RenderManager::rrDestroyWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand)
{
    std::string weaponEntityName = curWeapon->getOgreNamePrefix() + hand + "_" + curCreature->getName();
    if(mSceneManager->hasEntity(weaponEntityName))
    {
        Ogre::Entity* weaponEntity = mSceneManager->getEntity(weaponEntityName);
        weaponEntity->detachFromParent();
        mSceneManager->destroyEntity(weaponEntity);
    }
}

void RenderManager::rrCreateMapLight(MapLight* curMapLight, bool displayVisual)
{
    // Create the light and attach it to the lightSceneNode.
    std::string mapLightName = curMapLight->getOgreNamePrefix() + curMapLight->getName();
    Ogre::Light* light = mSceneManager->createLight(mapLightName + "_light");
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
        Ogre::Entity* lightEntity = mSceneManager->createEntity(mapLightName, "Lamp.mesh");
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
    if (mSceneManager->hasLight(mapLightName + "_light"))
    {
        Ogre::Light* light = mSceneManager->getLight(mapLightName + "_light");
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
    if (mSceneManager->hasLight(mapLightName + "_light"))
    {
        Ogre::SceneNode* mapLightNode = mSceneManager->getSceneNode(mapLightName + "_node");
        std::string mapLightIndicatorName = mapLightName;
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

void RenderManager::rrPickUpEntity(GameEntity* curEntity, Player* localPlayer)
{
    Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
    if(ent->hasAnimationState("Pickup"))
        mHandAnimationState = setEntityAnimation(ent, "Pickup", false);

    // Detach the entity from its scene node
    Ogre::SceneNode* curEntityNode = curEntity->getEntityNode();
    curEntity->getParentSceneNode()->removeChild(curEntityNode);

    // We make sure the creature will be rendered over the scene by adding it to the same render queue as the keeper hand (and
    // by clearing the depth buffer in ODFrameListener)
    changeRenderQueueRecursive(curEntityNode, OD_RENDER_QUEUE_ID_GUI);

    // Attach the creature to the hand scene node
    mHandKeeperNode->addChild(curEntityNode);
    Ogre::Vector3 scale = curEntity->getScale();
    scale *= KEEPER_HAND_CREATURE_PICKED_SCALE;
    curEntityNode->setScale(scale);

    rrOrderHand(localPlayer);
}

void RenderManager::rrDropHand(GameEntity* curEntity, Player* localPlayer)
{
    Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
    if(ent->hasAnimationState("Drop"))
        mHandAnimationState = setEntityAnimation(ent, "Drop", false);

    // Detach the entity from the "hand" scene node
    Ogre::SceneNode* curEntityNode = curEntity->getEntityNode();
    mHandKeeperNode->removeChild(curEntityNode);

    // We put the creature back to the default render queue
    changeRenderQueueRecursive(curEntityNode, Ogre::RenderQueueGroupID::RENDER_QUEUE_MAIN);

    // Attach the creature from the creature scene node
    curEntity->getParentSceneNode()->addChild(curEntityNode);
    Ogre::Vector3 position = curEntity->getPosition();
    curEntityNode->setPosition(position);
    curEntityNode->setScale(curEntity->getScale());

    rrOrderHand(localPlayer);
}

void RenderManager::rrOrderHand(Player* localPlayer)
{
    // Move the other creatures in the player's hand to make room for the one just picked up.
    int i = 0;
    const std::vector<GameEntity*>& objectsInHand = localPlayer->getObjectsInHand();
    for (GameEntity* tmpEntity : objectsInHand)
    {
        Ogre::Vector3 pos;
        pos.x = static_cast<Ogre::Real>(i % 6 + 1) * KEEPER_HAND_CREATURE_PICKED_OFFSET;
        pos.y = static_cast<Ogre::Real>(i / 6) * KEEPER_HAND_CREATURE_PICKED_OFFSET;
        pos.z = 0;
        tmpEntity->getEntityNode()->setPosition(pos);
        ++i;
    }
}

void RenderManager::rrRotateHand(Player* localPlayer)
{
    // Loop over the creatures in our hand and redraw each of them in their new location.
    int i = 0;
    const std::vector<GameEntity*>& objectsInHand = localPlayer->getObjectsInHand();
    for (GameEntity* tmpEntity : objectsInHand)
    {
        Ogre::SceneNode* tmpEntityNode = mSceneManager->getSceneNode(tmpEntity->getOgreNamePrefix() + tmpEntity->getName() + "_node");
        tmpEntityNode->setPosition(static_cast<Ogre::Real>(i % 6 + 1), static_cast<Ogre::Real>(i / 6), static_cast<Ogre::Real>(0.0));
        ++i;
    }
}

void RenderManager::rrCreateCreatureVisualDebug(Creature* curCreature, Tile* curTile)
{
    if (curTile != nullptr && curCreature != nullptr)
    {
        std::stringstream tempSS;
        tempSS << "Vision_indicator_" << curCreature->getName() << "_"
            << curTile->getX() << "_" << curTile->getY();

        Ogre::Entity* visIndicatorEntity = mSceneManager->createEntity(tempSS.str(),
                                           "Cre_vision_indicator.mesh");
        Ogre::SceneNode* visIndicatorNode = mCreatureSceneNode->createChildSceneNode(tempSS.str()
                                            + "_node");
        visIndicatorNode->attachObject(visIndicatorEntity);
        visIndicatorNode->setPosition(Ogre::Vector3(static_cast<Ogre::Real>(curTile->getX()),
                                                    static_cast<Ogre::Real>(curTile->getY()),
                                                    static_cast<Ogre::Real>(0)));
        visIndicatorNode->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT));
    }
}

void RenderManager::rrDestroyCreatureVisualDebug(Creature* curCreature, Tile* curTile)
{
    std::stringstream tempSS;
    tempSS << "Vision_indicator_" << curCreature->getName() << "_"
        << curTile->getX() << "_" << curTile->getY();
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
            << tile->getX() << "_" << tile->getY();

        Ogre::Entity* visIndicatorEntity = mSceneManager->createEntity(tempSS.str(),
                                           "Cre_vision_indicator.mesh");
        Ogre::SceneNode* visIndicatorNode = mCreatureSceneNode->createChildSceneNode(tempSS.str()
                                            + "_node");
        visIndicatorNode->attachObject(visIndicatorEntity);
        visIndicatorNode->setPosition(Ogre::Vector3(static_cast<Ogre::Real>(tile->getX()),
                                                    static_cast<Ogre::Real>(tile->getY()),
                                                    static_cast<Ogre::Real>(0)));
        visIndicatorNode->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT));
    }
}

void RenderManager::rrDestroySeatVisionVisualDebug(int seatId, Tile* tile)
{
    std::stringstream tempSS;
    tempSS << "Seat_Vision_indicator" << seatId << "_"
        << tile->getX() << "_" << tile->getY();
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
    std::string objectName = curAnimatedObject->getOgreNamePrefix()
        + curAnimatedObject->getName();
    if (!mSceneManager->hasEntity(objectName))
        return;

    Ogre::Entity* objectEntity = mSceneManager->getEntity(objectName);

    // Can't animate entities without skeleton
    if (!objectEntity->hasSkeleton())
        return;

    std::string anim = animation;

    // Handle the case where this entity does not have the requested animation.
    while (!objectEntity->getSkeleton()->hasAnimation(anim))
    {
        // Try to change the unexisting animation to a close existing one.
        if (anim == EntityAnimation::sleep_anim)
        {
            anim = EntityAnimation::die_anim;
            continue;
        }
        else if (anim == EntityAnimation::die_anim)
        {
            anim = EntityAnimation::idle_anim;
            break;
        }

        if (anim == EntityAnimation::flee_anim)
        {
            anim = EntityAnimation::walk_anim;
        }
        else if (anim == EntityAnimation::dig_anim || anim == EntityAnimation::claim_anim)
        {
            anim = EntityAnimation::attack_anim;
        }
        else
        {
            anim = EntityAnimation::idle_anim;
            break;
        }
    }

    if (!objectEntity->getSkeleton()->hasAnimation(anim))
        return;

    Ogre::AnimationState* animState = setEntityAnimation(objectEntity, anim, loop);
    curAnimatedObject->setAnimationState(animState);
}
void RenderManager::rrMoveEntity(GameEntity* entity, const Ogre::Vector3& position)
{
    if(entity->getEntityNode() == nullptr)
    {
        OD_LOG_ERR("Entity do not have node=" + entity->getName());
        return;
    }

    entity->getEntityNode()->setPosition(position);
}

void RenderManager::rrMoveMapLightFlicker(MapLight* mapLight, const Ogre::Vector3& position)
{
    if(mapLight->getFlickerNode() == nullptr)
    {
        OD_LOG_ERR("MapLight do not have flicker=" + mapLight->getName());
        return;
    }

    mapLight->getFlickerNode()->setPosition(position);
}

Ogre::ParticleSystem* RenderManager::rrEntityAddParticleEffect(GameEntity* entity, const std::string& particleName,
    const std::string& particleScript)
{
    Ogre::SceneNode* node = entity->getEntityNode();
    if(particleScript.empty())
        return nullptr;

    Ogre::ParticleSystem* particleSystem = mSceneManager->createParticleSystem(particleName, particleScript);

    node->attachObject(particleSystem);

    return particleSystem;
}

void RenderManager::rrEntityRemoveParticleEffect(GameEntity* entity, Ogre::ParticleSystem* particleSystem)
{
    if(particleSystem == nullptr)
        return;

    Ogre::SceneNode* node = entity->getEntityNode();
    node->detachObject(particleSystem);
    mSceneManager->destroyParticleSystem(particleSystem);
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
    Ogre::AnimationStateSet* animationSet = objectEntity->getAllAnimationStates();
    if(animationSet != nullptr)
    {
        for(Ogre::AnimationStateIterator asi =
            animationSet->getAnimationStateIterator(); asi.hasMoreElements(); asi.moveNext())
        {
            std::string animName = asi.peekNextValue()->getAnimationName();
            ret += "\nAnimation: " + animName;
        }
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

void RenderManager::colourizeEntity(Ogre::Entity *ent, const Seat* seat, bool markedForDigging, bool playerHasVision)
{
    // Colorize the the textures
    // Loop over the sub entities in the mesh
    for (unsigned int i = 0; i < ent->getNumSubEntities(); ++i)
    {
        Ogre::SubEntity *tempSubEntity = ent->getSubEntity(i);

        std::string materialName = tempSubEntity->getMaterialName();
        // If the material name have been modified, we restore the original name
        std::size_t index = materialName.find("##");
        if(index != std::string::npos)
            materialName = materialName.substr(0, index);

        materialName = colourizeMaterial(materialName, seat, markedForDigging, playerHasVision);
        tempSubEntity->setMaterialName(materialName);
    }
}

std::string RenderManager::colourizeMaterial(const std::string& materialName, const Seat* seat, bool markedForDigging, bool playerHasVision)
{
    if (seat == nullptr && !markedForDigging && playerHasVision)
        return materialName;

    std::stringstream tempSS;

    tempSS.str("");

    tempSS << materialName << "##";

    // Create the material name.
    if(seat != nullptr)
        tempSS << "Color_" << seat->getColorId() << "_" ;
    else
        tempSS << "Color_null_" ;

    if (markedForDigging)
        tempSS << "dig_";
    else if(!playerHasVision)
        tempSS << "novision_";

    Ogre::MaterialPtr requestedMaterial = Ogre::MaterialManager::getSingleton().getByName(tempSS.str());

    //cout << "\nCloning material:  " << tempSS.str();

    // If this texture has been copied and colourized, we can return
    if (!requestedMaterial.isNull())
        return tempSS.str();

    // If not yet, then do so

    // Check to see if we find a seat with the requested color, if not then just use the original, uncolored material.
    if (seat == nullptr && !markedForDigging && playerHasVision)
        return materialName;

    Ogre::MaterialPtr oldMaterial = Ogre::MaterialManager::getSingleton().getByName(materialName);

    //std::cout << "\nMaterial does not exist, creating a new one.";
    Ogre::MaterialPtr newMaterial = oldMaterial->clone(tempSS.str());
    bool cloned = mShaderGenerator->cloneShaderBasedTechniques(oldMaterial->getName(), oldMaterial->getGroup(),
                                                 newMaterial->getName(), newMaterial->getGroup());
    if(!cloned)
    {
        OD_LOG_ERR("Failed to clone rtss for material: " + materialName);
    }

    // Loop over the techniques for the new material
    for (unsigned int j = 0; j < newMaterial->getNumTechniques(); ++j)
    {
        Ogre::Technique* technique = newMaterial->getTechnique(j);
        if (technique->getNumPasses() == 0)
            continue;

        if (markedForDigging)
        {
            // Color the material with yellow on the latest pass
            // so we're sure to see the taint.
            Ogre::ColourValue color(1.0, 1.0, 0.0, 1.0);
            for (uint16_t i = 0; i < technique->getNumPasses(); ++i)
            {
                Ogre::Pass* pass = technique->getPass(i);
                pass->setSpecular(color);
                pass->setAmbient(color);
                pass->setDiffuse(color);
                pass->setEmissive(color);
            }
        }
        else if(!playerHasVision)
        {
            // Color the material with dark color on the latest pass
            // so we're sure to see the taint.
            Ogre::Pass* pass = technique->getPass(0);
            Ogre::ColourValue color(0.2, 0.2, 0.2, 1.0);
            pass->setSpecular(color);
            pass->setAmbient(color);
            pass->setDiffuse(color);
        }
        if (seat != nullptr)
        {
            // Color the material with the Seat's color.
            Ogre::Pass* pass = technique->getPass(technique->getNumPasses() - 1);
            Ogre::ColourValue color = seat->getColorValue();
            color.a = 1.0;
            pass->setAmbient(color);
            pass->setDiffuse(color);
            pass->setSpecular(color);
        }
    }

    return tempSS.str();
}

void RenderManager::rrCarryEntity(Creature* carrier, GameEntity* carried)
{
    Ogre::Entity* carrierEnt = mSceneManager->getEntity(carrier->getOgreNamePrefix() + carrier->getName());
    Ogre::Entity* carriedEnt = mSceneManager->getEntity(carried->getOgreNamePrefix() + carried->getName());
    Ogre::SceneNode* carrierNode = mSceneManager->getSceneNode(carrierEnt->getName() + "_node");
    Ogre::SceneNode* carriedNode = mSceneManager->getSceneNode(carriedEnt->getName() + "_node");
    carried->getParentSceneNode()->removeChild(carriedNode);
    carriedNode->setInheritScale(false);
    carrierNode->addChild(carriedNode);
    // We want the carried object to be at half tile (z = 0.5)
    Ogre::Real z = 0.5 / carrier->getScale().z;
    carriedNode->setPosition(Ogre::Vector3(0, 0, z));
}

void RenderManager::rrReleaseCarriedEntity(Creature* carrier, GameEntity* carried)
{
    Ogre::Entity* carrierEnt = mSceneManager->getEntity(carrier->getOgreNamePrefix() + carrier->getName());
    Ogre::Entity* carriedEnt = mSceneManager->getEntity(carried->getOgreNamePrefix() + carried->getName());
    Ogre::SceneNode* carrierNode = mSceneManager->getSceneNode(carrierEnt->getName() + "_node");
    Ogre::SceneNode* carriedNode = mSceneManager->getSceneNode(carriedEnt->getName() + "_node");
    carrierNode->removeChild(carriedNode);
    carried->getParentSceneNode()->addChild(carriedNode);
    carriedNode->setInheritScale(true);
}

void RenderManager::rrSetCreaturesTextOverlay(GameMap& gameMap, bool value)
{
    mCreatureTextOverlayDisplayed = value;
    for(Creature* creature : gameMap.getCreatures())
        creature->getOverlayStatus()->displayHealthOverlay(mCreatureTextOverlayDisplayed ? -1.0 : 0.0);
}

void RenderManager::rrTemporaryDisplayCreaturesTextOverlay(Creature* creature, Ogre::Real timeToDisplay)
{
    creature->getOverlayStatus()->displayHealthOverlay(timeToDisplay);
}

void RenderManager::rrToggleHandSelectorVisibility()
{
    if((mHandKeeperHandVisibility & 0x01) == 0)
        mHandKeeperHandVisibility |= 0x01;
    else
        mHandKeeperHandVisibility &= ~0x01;

    mHandKeeperNode->setVisible(mHandKeeperHandVisibility == 0);
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

    Ogre::MaterialPtr requestedMaterial = Ogre::MaterialManager::getSingleton().getByName(newMaterialName.str());

    // If this texture has been copied and colourized, we can return
    if (!requestedMaterial.isNull())
        return newMaterialName.str();

    // If not yet, then do so
    Ogre::MaterialPtr oldMaterial = Ogre::MaterialManager::getSingleton().getByName(materialName);
    //std::cout << "\nMaterial does not exist, creating a new one.";
    Ogre::MaterialPtr newMaterial = oldMaterial->clone(newMaterialName.str());
    bool cloned = mShaderGenerator->cloneShaderBasedTechniques(oldMaterial->getName(), oldMaterial->getGroup(),
                                                               newMaterial->getName(), newMaterial->getGroup());
    if(!cloned)
    {
        OD_LOG_ERR("Failed to clone rtss for material: " + materialName);
    }

    // Loop over the techniques for the new material
    for (auto i = 0; i < newMaterial->getNumTechniques(); ++i)
    {
        Ogre::Technique* technique = newMaterial->getTechnique(i);
        for(auto j = 0; j < technique->getNumPasses(); ++j)
        {
            // Set alpha value for all passes
            Ogre::Pass* pass = technique->getPass(j);
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
    }

    return newMaterialName.str();
}

void RenderManager::moveCursor(float relX, float relY)
{
    Ogre::Camera* cam = mViewport->getCamera();
    if(cam->getFOVy() != mCurrentFOVy)
    {
        mCurrentFOVy = cam->getFOVy();
        Ogre::Radian angle = cam->getFOVy() * 0.5f;
        Ogre::Real tan = Ogre::Math::Tan(angle);
        Ogre::Real shortestSize = KEEPER_HAND_POS_Z * tan * 2.0f;
        Ogre::Real width = mViewport->getActualWidth();
        Ogre::Real height = mViewport->getActualHeight();
        if(width > height)
        {
            mFactorHeight = shortestSize;
            mFactorWidth = shortestSize * width / height;
        }
        else
        {
            mFactorWidth = shortestSize;
            mFactorHeight = shortestSize * height / width;
        }
    }

    mHandKeeperNode->setPosition(mFactorWidth * (relX - 0.5f), mFactorHeight * (0.5f - relY), -KEEPER_HAND_POS_Z);
}

void RenderManager::moveWorldCoords(Ogre::Real x, Ogre::Real y)
{
    if(mHandLight != nullptr)
        mHandLight->setPosition(x, y, KEEPER_HAND_WORLD_Z);
}

void RenderManager::entitySlapped()
{
    Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
    if(ent->hasAnimationState("Slap"))
        mHandAnimationState = setEntityAnimation(ent, "Slap", false);
}

std::string RenderManager::rrBuildSkullFlagMaterial(const std::string& materialNameBase,
        const Ogre::ColourValue& color)
{
    std::string materialNameToUse = materialNameBase + "_" + Helper::toString(color);

    Ogre::MaterialPtr requestedMaterial = Ogre::MaterialPtr(Ogre::MaterialManager::getSingleton().getByName(materialNameToUse));

    // If this texture has been copied and colourized, we can return
    if (!requestedMaterial.isNull())
        return materialNameToUse;

    Ogre::MaterialPtr oldMaterial = Ogre::MaterialManager::getSingleton().getByName(materialNameBase);

    Ogre::MaterialPtr newMaterial = oldMaterial->clone(materialNameToUse);
    if(!mShaderGenerator->cloneShaderBasedTechniques(oldMaterial->getName(), oldMaterial->getGroup(),
            newMaterial->getName(), newMaterial->getGroup()))
    {
        OD_LOG_ERR("Failed to clone rtss for material: " + materialNameBase);
    }

    for (unsigned int j = 0; j < newMaterial->getNumTechniques(); ++j)
    {
        Ogre::Technique* technique = newMaterial->getTechnique(j);
        if (technique->getNumPasses() == 0)
            continue;

        for (uint16_t i = 0; i < technique->getNumPasses(); ++i)
        {
            Ogre::Pass* pass = technique->getPass(i);
            pass->getTextureUnitState(0)->setColourOperationEx(Ogre::LayerBlendOperationEx::LBX_MODULATE,
                Ogre::LayerBlendSource::LBS_TEXTURE, Ogre::LayerBlendSource::LBS_MANUAL, Ogre::ColourValue(),
                color);
        }
    }

    return materialNameToUse;
}

void RenderManager::rrMinimapRendering(bool postRender)
{
    if(mHandLight != nullptr)
        mHandLight->setVisible(postRender);

    mLightSceneNode->setVisible(postRender);
}

void RenderManager::changeRenderQueueRecursive(Ogre::SceneNode* node, uint8_t renderQueueId)
{
    for(uint32_t i = 0; i < node->numAttachedObjects(); ++i)
    {
        Ogre::MovableObject* obj = node->getAttachedObject(i);
        obj->setRenderQueueGroup(renderQueueId);
    }

    for(uint32_t i = 0; i < node->numChildren(); ++i)
    {
        Ogre::SceneNode* childNode = dynamic_cast<Ogre::SceneNode *>(node->getChild(i));
        if(childNode == nullptr)
            continue;

        changeRenderQueueRecursive(childNode, renderQueueId);
    }
}

Ogre::AnimationState* RenderManager::setEntityAnimation(Ogre::Entity* ent, const std::string& animation, bool loop)
{
    Ogre::AnimationStateSet* animationSet = ent->getAllAnimationStates();
    if(animationSet == nullptr)
        return nullptr;

    for(Ogre::AnimationStateIterator asi =
        animationSet->getAnimationStateIterator(); asi.hasMoreElements(); asi.moveNext())
    {
        asi.peekNextValue()->setEnabled(false);
    }

    Ogre::AnimationState* animState = ent->getAnimationState(animation);
    animState->setTimePosition(0);
    animState->setLoop(loop);
    animState->setEnabled(true);

    return animState;
}
