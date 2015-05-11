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

#include <OgreMesh.h>
#include <OgreBone.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreMaterialManager.h>
#include <OgreParticleSystem.h>
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
#include <OgreQuaternion.h>
#include <Overlay/OgreOverlaySystem.h>
#include <RTShaderSystem/OgreShaderGenerator.h>
#include <sstream>

using std::stringstream;

template<> RenderManager* Ogre::Singleton<RenderManager>::msSingleton = nullptr;

const Ogre::Real RenderManager::BLENDER_UNITS_PER_OGRE_UNIT = 10.0f;

const Ogre::Real RenderManager::KEEPER_HAND_WORLD_Z = 20.0f / RenderManager::BLENDER_UNITS_PER_OGRE_UNIT;

const Ogre::Real KEEPER_HAND_POS_Z = 10;

RenderManager::RenderManager(Ogre::OverlaySystem* overlaySystem) :
    mHandAnimationState(nullptr),
    mViewport(nullptr),
    mShaderGenerator(nullptr),
    mHandLight(nullptr),
    mHandSquareSelectorNode(nullptr),
    mHandKeeperMesh(nullptr),
    mCurrentFOVy(0.0f),
    mFactorWidth(0.0f),
    mFactorHeight(0.0f),
    mCreatureTextOverlayDisplayed(false),
    mHandSquareSelectorVisibility(0),
    mHandKeeperHandVisibility(0)
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

void RenderManager::clearRenderer()
{
    mCreatureTextOverlayDisplayed = false;
}

void RenderManager::triggerCompositor(const std::string& compositorName)
{
    Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, compositorName.c_str(), true);
}

void RenderManager::createScene(Ogre::Viewport* nViewport)
{
    LogManager::getSingleton().logMessage("Creating scene...");

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

    // Create the scene nodes that will follow the mouse pointer.
    // Create the single tile selection mesh
    Ogre::Entity* squareSelectorEnt = mSceneManager->createEntity("SquareSelector", "SquareSelector.mesh");
    squareSelectorEnt->setLightMask(0);
    squareSelectorEnt->setCastShadows(false);
    mHandSquareSelectorNode = mSceneManager->getRootSceneNode()->createChildSceneNode("SquareSelectorNode");
    mHandSquareSelectorNode->translate(Ogre::Vector3(0, 0, 0));
    mHandSquareSelectorNode->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                              BLENDER_UNITS_PER_OGRE_UNIT, 0.45 * BLENDER_UNITS_PER_OGRE_UNIT));
    mHandSquareSelectorNode->attachObject(squareSelectorEnt);
    Ogre::SceneNode *node2 = mHandSquareSelectorNode->createChildSceneNode("Hand_node");
    node2->setPosition(static_cast<Ogre::Real>(0.0),
                       static_cast<Ogre::Real>(0.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                       static_cast<Ogre::Real>(3.0 / BLENDER_UNITS_PER_OGRE_UNIT));
    node2->scale(Ogre::Vector3(static_cast<Ogre::Real>(1.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                               static_cast<Ogre::Real>(1.0 / BLENDER_UNITS_PER_OGRE_UNIT),
                               static_cast<Ogre::Real>(1.0 / BLENDER_UNITS_PER_OGRE_UNIT)));

    Ogre::ParticleSystem::setDefaultNonVisibleUpdateTimeout(5);

    Ogre::Entity* keeperHandEnt = mSceneManager->createEntity("keeperHandEnt", "Keeperhand.mesh");
    keeperHandEnt->setLightMask(0);
    keeperHandEnt->setCastShadows(false);
    mHandAnimationState = keeperHandEnt->getAnimationState("Idle");
    mHandAnimationState->setTimePosition(0);
    mHandAnimationState->setLoop(true);
    mHandAnimationState->setEnabled(true);

    Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
    Ogre::Overlay* handKeeperOverlay = overlayManager.create(keeperHandEnt->getName() + "_Ov");
    mHandKeeperMesh = mSceneManager->createSceneNode(keeperHandEnt->getName() + "_node");
    mHandKeeperMesh->attachObject(keeperHandEnt);
    mHandKeeperMesh->setScale(0.12f, 0.12f, 0.12f);
    mHandKeeperMesh->setPosition(0.0f, 0.0f, -KEEPER_HAND_POS_Z);
    handKeeperOverlay->add3D(mHandKeeperMesh);
    mHandKeeperMesh->setVisible(true);
    handKeeperOverlay->show();

    //Add a too small to be visible dummy dirt tile to the hand node
    //so that there will allways be a dirt tile "visible"
    //This is an ugly workaround for issue where destroying some entities messes
    //up the lighing for some of the rtshader materials.
    Ogre::SceneNode* dummyNode = mHandKeeperMesh->createChildSceneNode("Dummy_node");
    dummyNode->setScale(Ogre::Vector3(0.00000001f, 0.00000001f, 0.00000001f));
    Ogre::Entity* dummyEnt = mSceneManager->createEntity("Dirt_fl_0000.mesh");
    dummyEnt->setLightMask(0);
    dummyEnt->setCastShadows(false);
    dummyNode->attachObject(dummyEnt);

    // Create the light which follows the single tile selection mesh
    mHandLight = mSceneManager->createLight("MouseLight");
    mHandLight->setType(Ogre::Light::LT_POINT);
    mHandLight->setDiffuseColour(Ogre::ColourValue(0.65, 0.65, 0.45));
    mHandLight->setSpecularColour(Ogre::ColourValue(0.65, 0.65, 0.45));
    mHandLight->setPosition(0.0f, 0.0f, KEEPER_HAND_WORLD_Z);
    mHandLight->setAttenuation(7, 1.0, 0.00, 0.3);

    mHandSquareSelectorNode->setVisible(mHandSquareSelectorVisibility == 0);
    mHandKeeperMesh->setVisible(mHandKeeperHandVisibility == 0);
}

void RenderManager::updateRenderAnimations(Ogre::Real timeSinceLastFrame)
{
    if(mHandAnimationState == nullptr)
        return;

    mHandAnimationState->addTime(timeSinceLastFrame);
    if(mHandAnimationState->hasEnded())
    {
        Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
        mHandAnimationState = ent->getAnimationState("Idle");
        mHandAnimationState->setTimePosition(0);
        mHandAnimationState->setLoop(true);
    }

}

void RenderManager::rrRefreshTile(const Tile& tile, const GameMap& gameMap, const Player& localPlayer)
{
    if (tile.getEntityNode() == nullptr)
        return;

    std::string tileName = tile.getOgreNamePrefix() + tile.getName();
    Ogre::Vector3 scale;
    std::string meshName = tile.getMeshName();
    const Seat* seatColorize = nullptr;
    const TileSetValue& tileSetValue = gameMap.getMeshForTile(&tile);
    bool isCustomMesh = false;
    if(meshName.empty())
    {
        // We compute the mesh
        meshName = tileSetValue.getMeshName();
        scale = gameMap.getTileSetScale();
    }
    else
    {
        //Tile has a covering building.
        isCustomMesh = true;
        scale = tile.getScale();
    }

    bool newMesh = false;
    if(mSceneManager->hasEntity(tileName))
    {
        Ogre::Entity* oldEnt = mSceneManager->getEntity(tileName);
        if(oldEnt->getMesh()->getName().compare(meshName) != 0)
        {
            // Unlink and delete the old mesh
            mSceneManager->getSceneNode(tileName + "_node")->detachObject(tileName);
            mSceneManager->destroyEntity(tileName);
            newMesh = true;
        }
    }
    else
    {
        newMesh = true;
    }
    Ogre::Entity* ent = nullptr;
    if(newMesh)
    {
        ent = mSceneManager->createEntity(tileName, meshName);
        // Link the tile mesh back to the relevant scene node so OGRE will render it
        Ogre::SceneNode* node = mSceneManager->getSceneNode(tileName + "_node");
        node->attachObject(ent);
        node->setScale(scale);
        node->resetOrientation();

        // If it is a custom mesh, we assume there is no need to rotate. If not, we
        // rotate depending on the tileset
        if(!isCustomMesh)
        {
            Ogre::Quaternion q;
            if(tileSetValue.getRotationX() != 0.0)
                q = q * Ogre::Quaternion(Ogre::Degree(tileSetValue.getRotationX()), Ogre::Vector3::UNIT_X);

            if(tileSetValue.getRotationY() != 0.0)
                q = q * Ogre::Quaternion(Ogre::Degree(tileSetValue.getRotationY()), Ogre::Vector3::UNIT_Y);

            if(tileSetValue.getRotationZ() != 0.0)
                q = q * Ogre::Quaternion(Ogre::Degree(tileSetValue.getRotationZ()), Ogre::Vector3::UNIT_Z);

            if(q != Ogre::Quaternion::IDENTITY)
                node->rotate(q);
        }

        Ogre::MeshPtr meshPtr = ent->getMesh();
        unsigned short src, dest;
        if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
        {
            meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
        }
    }
    else
    {
        ent = mSceneManager->getEntity(tileName);
    }

    if(!isCustomMesh)
    {
        // We replace the material if required to by the tileset
        if(!tileSetValue.getMaterialName().empty())
            ent->setMaterialName(tileSetValue.getMaterialName());

        // On client side, the seat is set only when the tile is claimed. So, if the
        // seat is not nullptr, we are sure it is fully claimed and we can colorize
        // the tile
        seatColorize = tile.getSeat();
    }

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
    colourizeEntity(ent, seatColorize, isMarked, vision);
}

void RenderManager::rrCreateTile(Tile& tile, const GameMap& gameMap, const Player& localPlayer)
{
    std::string tileName = tile.getOgreNamePrefix() + tile.getName();
    Ogre::SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode(tileName + "_node");
    tile.setParentSceneNode(node->getParentSceneNode());
    tile.setEntityNode(node);
    node->setPosition(static_cast<Ogre::Real>(tile.getX()), static_cast<Ogre::Real>(tile.getY()), 0);

    rrRefreshTile(tile, gameMap, localPlayer);
}

void RenderManager::rrDestroyTile(Tile& tile)
{
    if (tile.getEntityNode() == nullptr)
        return;

    Ogre::Entity* ent = mSceneManager->getEntity(tile.getOgreNamePrefix() + tile.getName());
    std::string selectorName = ent->getName() + "_selection_indicator";
    if(mSceneManager->hasEntity(selectorName))
    {
        Ogre::SceneNode* selectorNode = mSceneManager->getSceneNode(selectorName + "Node");
        Ogre::Entity* selectorEnt = mSceneManager->getEntity(selectorName);
        tile.getEntityNode()->removeChild(selectorNode);
        selectorNode->detachObject(selectorEnt);
        mSceneManager->destroySceneNode(selectorNode);
        mSceneManager->destroyEntity(selectorEnt);
    }
    tile.getEntityNode()->detachObject(ent);
    mSceneManager->destroySceneNode(tile.getEntityNode());
    mSceneManager->destroyEntity(ent);
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

    Ogre::Entity* ent = mSceneManager->createEntity(tempString, meshName + ".mesh");
    Ogre::SceneNode* node = mRoomSceneNode->createChildSceneNode(tempString + "_node");

    node->setPosition(renderedMovableEntity->getPosition());
    node->setScale(renderedMovableEntity->getScale());
    node->roll(Ogre::Degree(renderedMovableEntity->getRotationAngle()));
    node->attachObject(ent);

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

void RenderManager::rrUpdateEntityOpacity(RenderedMovableEntity* entity)
{
    std::string entStr = entity->getOgreNamePrefix() + entity->getName();
    Ogre::Entity* ogreEnt = mSceneManager->hasEntity(entStr) ? mSceneManager->getEntity(entStr) : nullptr;
    if (ogreEnt == nullptr)
    {
        LogManager::getSingleton().logMessage("Update opacity: Couldn't find entity: " + entStr);
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
    OD_ASSERT_TRUE_MSG(entity->getEntityNode() != nullptr, "entity=" + entity->getName());
    if(entity->getEntityNode() == nullptr)
        return;

    entity->getEntityNode()->setScale(entity->getScale());
}

void RenderManager::rrCreateWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand)
{
    Ogre::Entity* ent = mSceneManager->getEntity(curCreature->getOgreNamePrefix() + curCreature->getName());
    std::string weaponName = curWeapon->getOgreNamePrefix() + hand;
    if(!ent->getSkeleton()->hasBone(weaponName))
    {
        LogManager::getSingleton().logMessage("WARNING: Tried to add weapons to entity \"" + ent->getName() + " \" using model \"" +
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
    {
        mHandAnimationState = ent->getAnimationState("Pickup");
        mHandAnimationState->setTimePosition(0);
        mHandAnimationState->setLoop(false);
        mHandAnimationState->setEnabled(true);
    }

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
    const std::vector<GameEntity*>& objectsInHand = localPlayer->getObjectsInHand();
    for (GameEntity* tmpEntity : objectsInHand)
    {
        Ogre::SceneNode* tmpEntityNode = mSceneManager->getSceneNode(tmpEntity->getOgreNamePrefix() + tmpEntity->getName() + "_node");
        tmpEntityNode->setPosition(static_cast<Ogre::Real>(i % 6 + 1), static_cast<Ogre::Real>(i / 6), static_cast<Ogre::Real>(0.0));
        ++i;
    }
}

void RenderManager::rrDropHand(GameEntity* curEntity, Player* localPlayer)
{
    Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
    if(ent->hasAnimationState("Drop"))
    {
        mHandAnimationState = ent->getAnimationState("Drop");
        mHandAnimationState->setTimePosition(0);
        mHandAnimationState->setLoop(false);
        mHandAnimationState->setEnabled(true);
    }

    // Detach the entity from the "hand" scene node
    Ogre::SceneNode* curEntityNode = mSceneManager->getSceneNode(curEntity->getOgreNamePrefix() + curEntity->getName() + "_node");
    mSceneManager->getSceneNode("Hand_node")->removeChild(curEntityNode);

    // Attach the creature from the creature scene node
    curEntity->getParentSceneNode()->addChild(curEntityNode);
    curEntityNode->setPosition(curEntity->getPosition());
    curEntityNode->setScale(curEntity->getScale());

    // Move the other creatures in the player's hand to replace the dropped one
    int i = 0;
    const std::vector<GameEntity*>& objectsInHand = localPlayer->getObjectsInHand();
    for (GameEntity* tmpEntity : objectsInHand)
    {
        Ogre::SceneNode* tmpEntityNode = mSceneManager->getSceneNode(tmpEntity->getOgreNamePrefix() + tmpEntity->getName() + "_node");
        tmpEntityNode->setPosition(static_cast<Ogre::Real>(i % 6 + 1), static_cast<Ogre::Real>(i / 6), static_cast<Ogre::Real>(0.0));
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
    Ogre::Entity* objectEntity = mSceneManager->getEntity(
                                     curAnimatedObject->getOgreNamePrefix()
                                     + curAnimatedObject->getName());

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

void RenderManager::colourizeEntity(Ogre::Entity *ent, const Seat* seat, bool markedForDigging, bool playerHasVision)
{
    // Colorize the the textures
    // Loop over the sub entities in the mesh
    for (unsigned int i = 0; i < ent->getNumSubEntities(); ++i)
    {
        Ogre::SubEntity *tempSubEntity = ent->getSubEntity(i);
        tempSubEntity->setMaterialName(colourizeMaterial(tempSubEntity->getMaterialName(), seat, markedForDigging, playerHasVision));
    }
}

std::string RenderManager::colourizeMaterial(const std::string& materialName, const Seat* seat, bool markedForDigging, bool playerHasVision)
{
    if (seat == nullptr && !markedForDigging && playerHasVision)
        return materialName;

    std::stringstream tempSS;

    tempSS.str("");

    // Create the material name.
    if(seat != nullptr)
        tempSS << "Color_" << seat->getColorId() << "_" ;
    else
        tempSS << "Color_null_" ;

    if (markedForDigging)
        tempSS << "dig_";
    else if(!playerHasVision)
        tempSS << "novision_";

    tempSS << materialName;
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
        LogManager::getSingleton().logMessage("Failed to clone rtss for material: " + materialName, LogMessageLevel::CRITICAL);
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
    if((mHandSquareSelectorVisibility & 0x01) == 0)
        mHandSquareSelectorVisibility |= 0x01;
    else
        mHandSquareSelectorVisibility &= ~0x01;

    if((mHandKeeperHandVisibility & 0x01) == 0)
        mHandKeeperHandVisibility |= 0x01;
    else
        mHandKeeperHandVisibility &= ~0x01;

    mHandSquareSelectorNode->setVisible(mHandSquareSelectorVisibility == 0);
    mHandKeeperMesh->setVisible(mHandKeeperHandVisibility == 0);
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
        LogManager::getSingleton().logMessage("Failed to clone rtss for material: " + materialName, LogMessageLevel::CRITICAL);
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

    mHandKeeperMesh->setPosition(mFactorWidth * (relX - 0.5f), mFactorHeight * (0.5f - relY), -KEEPER_HAND_POS_Z);
}

void RenderManager::moveWorldCoords(Ogre::Real x, Ogre::Real y)
{
    mHandLight->setPosition(x, y, KEEPER_HAND_WORLD_Z);
}

void RenderManager::setHoveredTile(int tileX, int tileY)
{
    mHandSquareSelectorNode->setPosition(static_cast<Ogre::Real>(tileX), static_cast<Ogre::Real>(tileY), 0.0f);
}

void RenderManager::entitySlapped()
{
    Ogre::Entity* ent = mSceneManager->getEntity("keeperHandEnt");
    if(ent->hasAnimationState("Slap"))
    {
        mHandAnimationState = ent->getAnimationState("Slap");
        mHandAnimationState->setTimePosition(0);
        mHandAnimationState->setLoop(false);
        mHandAnimationState->setEnabled(true);
    }
}

std::string RenderManager::rrBuildSkullFlagMaterial(const std::string& materialNameBase,
        const Ogre::ColourValue& color)
{
    std::string materialNameToUse = materialNameBase + "_" + Ogre::StringConverter::toString(color);

    Ogre::MaterialPtr requestedMaterial = Ogre::MaterialPtr(Ogre::MaterialManager::getSingleton().getByName(materialNameToUse));

    // If this texture has been copied and colourized, we can return
    if (!requestedMaterial.isNull())
        return materialNameToUse;

    Ogre::MaterialPtr oldMaterial = Ogre::MaterialManager::getSingleton().getByName(materialNameBase);

    Ogre::MaterialPtr newMaterial = oldMaterial->clone(materialNameToUse);
    if(!mShaderGenerator->cloneShaderBasedTechniques(oldMaterial->getName(), oldMaterial->getGroup(),
            newMaterial->getName(), newMaterial->getGroup()))
    {
        OD_ASSERT_TRUE_MSG(false, "Failed to clone rtss for material: " + materialNameBase);
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
