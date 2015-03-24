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

#ifndef RENDERMANAGER_H_
#define RENDERMANAGER_H_

#include <deque>
#include <string>
#include <OgreSingleton.h>
#include <OgreMath.h>

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
class SceneManager;
class SceneNode;
class OverlaySystem;
class AnimationState;

namespace RTShader {
    class ShaderGenerator;
}
} //End namespace Ogre

class RenderManager: public Ogre::Singleton<RenderManager>
{
public:
    RenderManager(Ogre::OverlaySystem* overlaySystem);
    ~RenderManager();

    inline Ogre::SceneManager* getSceneManager() const
    { return mSceneManager; }

    //! \brief Loop through the render requests in the queue and process them
    void updateRenderAnimations(Ogre::Real timeSinceLastFrame);

    //! \brief Resets the renderer
    void clearRenderer();

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
    void setHoveredTile(int tileX, int tileY);
    void entitySlapped();

    static const Ogre::Real BLENDER_UNITS_PER_OGRE_UNIT;
    static const Ogre::Real KEEPER_HAND_WORLD_Z;

    //! Debug function to be used for dev only. Beware, it should not be called from the server thread
    static std::string consoleListAnimationsForMesh(const std::string& meshName);

    //Render request functions
    void rrRefreshTile(const Tile* curTile, const Player* localPlayer);
    void rrCreateTile(Tile* curTile, Player* localPlayer);
    void rrDestroyTile(Tile* curTile);
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

    //! \brief Toggles the creatures text overlay
    void rrSetCreaturesTextOverlay(GameMap& gameMap, bool value);

    //! \brief Toggles the creatures text overlay
    void rrTemporaryDisplayCreaturesTextOverlay(Creature* creature, Ogre::Real timeToDisplay);

    std::string rrBuildSkullFlagMaterial(const std::string& materialNameBase,
        const Ogre::ColourValue& color);

private:

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

    //! \brief The main scene manager reference. Don't delete it.
    Ogre::SceneManager* mSceneManager;

    //! \brief Reference to the Ogre sub scene nodes. Don't delete them.
    Ogre::SceneNode* mRoomSceneNode;
    Ogre::SceneNode* mCreatureSceneNode;
    Ogre::SceneNode* mLightSceneNode;

    Ogre::AnimationState* mHandAnimationState;

    Ogre::Viewport* mViewport;
    Ogre::RTShader::ShaderGenerator* mShaderGenerator;

    //! For the keeper hand
    Ogre::Light* mHandLight;
    Ogre::SceneNode* mHandSquareSelectorNode;
    Ogre::SceneNode* mHandKeeperMesh;
    Ogre::Radian mCurrentFOVy;
    Ogre::Real mFactorWidth;
    Ogre::Real mFactorHeight;

    //! \brief True if the creatures are currently displaying their text overlay
    bool mCreatureTextOverlayDisplayed;
};

#endif // RENDERMANAGER_H_
