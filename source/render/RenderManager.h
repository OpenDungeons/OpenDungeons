/*!
 *  \file   RenderManager.h
 *  \date   26 March 2011
 *  \author oln
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

#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include <deque>
#include <string>
#include <OgreSingleton.h>
#include <OgreMath.h>
#include <cstdint>

class GameMap;
class Building;
class Seat;
class Tile;
class GameEntity;
class MovableGameEntity;
class MapLight;
class Creature;
class Player;
class RenderedMovableEntity;
class Weapon;

namespace Ogre
{
class AnimationState;
class OverlaySystem;
class SceneManager;
class SceneNode;
class ParticleSystem;

namespace RTShader {
    class ShaderGenerator;
}
} //End namespace Ogre

class RenderManager: public Ogre::Singleton<RenderManager>
{
public:
    RenderManager(Ogre::OverlaySystem* overlaySystem);
    ~RenderManager();

    static const uint8_t OD_RENDER_QUEUE_ID_GUI;

    inline Ogre::SceneManager* getSceneManager() const
    { return mSceneManager; }

    //! \brief Loop through the render requests in the queue and process them
    void updateRenderAnimations(Ogre::Real timeSinceLastFrame);

    //! \brief Initialize the renderer when a new game (Game or Editor) is launched
    void initGameRenderer(GameMap* gameMap);
    void stopGameRenderer(GameMap* gameMap);

    //! \brief starts the compositor compositorName.
    void triggerCompositor(const std::string& compositorName);

    //! \brief setup the scene
    void createScene(Ogre::Viewport*);

    //! \brief Set the entity's opacity
    void setEntityOpacity(Ogre::Entity* ent, float opacity);

    //! Beware this should be called on client side only (not from the server thread)
    //! moveCursor allows to move the cursor on GUI
    //!  moveWorldCoords sends the world coords where the map light is
    void moveCursor(float relX, float relY);
    void moveWorldCoords(Ogre::Real x, Ogre::Real y);
    void entitySlapped();

    static const Ogre::Real BLENDER_UNITS_PER_OGRE_UNIT;
    static const Ogre::Real KEEPER_HAND_WORLD_Z;

    //! Debug function to be used for dev only. Beware, it should not be called from the server thread
    static std::string consoleListAnimationsForMesh(const std::string& meshName);

    //Render request functions
    void rrRefreshTile(const Tile& tile, const GameMap& gameMap, const Player& localPlayer);
    void rrCreateTile(Tile& tile, const GameMap& gameMap, const Player& localPlayer);
    void rrDestroyTile(Tile& tile);
    void rrTemporalMarkTile(Tile* curTile);
    void rrCreateRenderedMovableEntity(RenderedMovableEntity* curRenderedMovableEntity);
    void rrDestroyRenderedMovableEntity(RenderedMovableEntity* curRenderedMovableEntity);
    void rrUpdateEntityOpacity(RenderedMovableEntity* entity);
    void rrCreateCreature(Creature* curCreature);
    void rrDestroyCreature(Creature* curCreature);
    void rrOrientEntityToward(MovableGameEntity* gameEntity, const Ogre::Vector3& direction);
    void rrScaleEntity(GameEntity* entity);
    void rrCreateWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand);
    void rrDestroyWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand);
    void rrCreateMapLight(MapLight* curMapLight, bool displayVisual);
    void rrDestroyMapLight(MapLight* curMapLight);
    void rrDestroyMapLightVisualIndicator(MapLight* curMapLight);
    void rrPickUpEntity(GameEntity* curEntity, Player* localPlayer);
    void rrDropHand(GameEntity* curEntity, Player* localPlayer);
    void rrRotateHand(Player* localPlayer);
    void rrCreateCreatureVisualDebug(Creature* curCreature, Tile* curTile);
    void rrDestroyCreatureVisualDebug(Creature* curCreature, Tile* curTile);
    void rrCreateSeatVisionVisualDebug(int seatId, Tile* tile);
    void rrDestroySeatVisionVisualDebug(int seatId, Tile* tile);
    void rrSetObjectAnimationState(MovableGameEntity* curAnimatedObject, const std::string& animation, bool loop);
    void rrMoveEntity(GameEntity* entity, const Ogre::Vector3& position);
    void rrMoveMapLightFlicker(MapLight* mapLight, const Ogre::Vector3& position);
    void rrCarryEntity(Creature* carrier, GameEntity* carried);
    void rrReleaseCarriedEntity(Creature* carrier, GameEntity* carried);
    Ogre::ParticleSystem* rrEntityAddParticleEffect(GameEntity* entity, const std::string& particleName,
        const std::string& particleScript);
    void rrEntityRemoveParticleEffect(GameEntity* entity, Ogre::ParticleSystem* particleSystem);
    void rrToggleHandSelectorVisibility();

    //! \brief Toggles the creatures text overlay
    void rrSetCreaturesTextOverlay(GameMap& gameMap, bool value);

    //! \brief Toggles the creatures text overlay
    void rrTemporaryDisplayCreaturesTextOverlay(Creature* creature, Ogre::Real timeToDisplay);

    std::string rrBuildSkullFlagMaterial(const std::string& materialNameBase,
        const Ogre::ColourValue& color);

    //! \brief Does requested stuff for rendering in the minimap. Each time the minimap is rendered, this function will be
    //! called once before rendering is done with postRender = false and once when it is rendered with postRender = true.
    //! That allows to hide stuff that we don't want to display in the minimap
    void rrMinimapRendering(bool postRender);

    Ogre::Light* addPointLightMenu(const std::string& name, const Ogre::Vector3& pos,
        const Ogre::ColourValue& diffuse, const Ogre::ColourValue& specular, Ogre::Real attenuationRange,
        Ogre::Real attenuationConstant, Ogre::Real attenuationLinear, Ogre::Real attenuationQuadratic);
    void removePointLightMenu(Ogre::Light* light);
    Ogre::Entity* addEntityMenu(const std::string& meshName, const std::string& entityName,
        const Ogre::Vector3& scale, const Ogre::Vector3& pos);
    void removeEntityMenu(Ogre::Entity* ent);
    Ogre::AnimationState* setMenuEntityAnimation(const std::string& entityName, const std::string& animation, bool loop);
    //! \brief Called to update the given animation with the given time. Returns true if animation ended and
    //! false otherwise
    bool updateMenuEntityAnimation(Ogre::AnimationState* animState, Ogre::Real timeSinceLastFrame);
    //! \brief Returns the scene node related to the given entity name. pos will be set to the current position
    //! of the node
    Ogre::SceneNode* getMenuEntityNode(const std::string& entityName, Ogre::Vector3& pos);
    void updateMenuEntityPosition(Ogre::SceneNode* node, const Ogre::Vector3& pos);
    void orientMenuEntityPosition(Ogre::SceneNode* node, const Ogre::Vector3& direction);
    Ogre::ParticleSystem* addEntityParticleEffectMenu(Ogre::SceneNode* node,
        const std::string& particleName, const std::string& particleScript);
    void removeEntityParticleEffectMenu(Ogre::SceneNode* node,
        Ogre::ParticleSystem* particleSystem);
    Ogre::ParticleSystem* addEntityParticleEffectBoneMenu(const std::string& entityName,
        const std::string& boneName, const std::string& particleName, const std::string& particleScript);
    void removeEntityParticleEffectBoneMenu(const std::string& entityName,
        Ogre::ParticleSystem* particleSystem);

private:
    //! \brief Correctly places entities in hand next to the keeper hand
    void changeRenderQueueRecursive(Ogre::SceneNode* node, uint8_t renderQueueId);

    //! \brief Correctly places entities in hand next to the keeper hand
    void rrOrderHand(Player* localPlayer);

    //! \brief Colorize the material with the corresponding team id color.
    //! \note If the material (wall tiles only) is marked for digging, a yellow color is added
    //! to the given color.
    //! \returns The new material name according to the current colorization.
    std::string colourizeMaterial(const std::string& materialName, const Seat* seat, bool markedForDigging, bool playerHasVision);

    //! \brief Colorize an entity with the team corresponding color.
    //! \Note: if the entity is marked for digging (wall tiles only), then a yellow color
    //! is added to the current colorization.
    void colourizeEntity(Ogre::Entity* ent, const Seat* seat, bool markedForDigging, bool playerHasVision);

    //! \brief Makes the material be transparent with the given opacity (0.0f - 1.0f)
    //! \returns The new material name according to the current opacity.
    std::string setMaterialOpacity(const std::string& materialName, float opacity);

    //! \brief Disables all animations of the given entity and starts the given one
    Ogre::AnimationState* setEntityAnimation(Ogre::Entity* ent, const std::string& animation, bool loop);

    //! \brief The main scene manager reference. Don't delete it.
    Ogre::SceneManager* mSceneManager;

    //! \brief Reference to the Ogre sub scene nodes. Don't delete them.
    Ogre::SceneNode* mTileSceneNode;
    Ogre::SceneNode* mRoomSceneNode;
    Ogre::SceneNode* mCreatureSceneNode;
    Ogre::SceneNode* mLightSceneNode;
    Ogre::SceneNode* mMainMenuSceneNode;

    Ogre::AnimationState* mHandAnimationState;

    Ogre::Viewport* mViewport;
    Ogre::RTShader::ShaderGenerator* mShaderGenerator;

    //! For the keeper hand
    Ogre::SceneNode* mHandKeeperNode;
    Ogre::Light* mHandLight;
    Ogre::Radian mCurrentFOVy;
    Ogre::Real mFactorWidth;
    Ogre::Real mFactorHeight;

    // As a workaround for some issues, we create dummy entities too small to be seen
    // and attach them to the keeper hand. This vector allows to keep a track and delete
    // them/recreate when loading a new game
    std::vector<Ogre::SceneNode*> mDummyEntities;

    //! \brief True if the creatures are currently displaying their text overlay
    bool mCreatureTextOverlayDisplayed;

    //! Bit array to allow to display tile hand (= 0) or not (!= 0)
    uint32_t mHandKeeperHandVisibility;
};

#endif // RENDERMANAGER_H
