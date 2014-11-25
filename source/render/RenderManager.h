/*!
 *  \file   RenderManager.h
 *  \date   26 March 2011
 *  \author oln
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

#ifndef RENDERMANAGER_H_
#define RENDERMANAGER_H_

#include <deque>
#include <string>
#include <OgreSingleton.h>
#include <RTShaderSystem/OgreShaderGenerator.h>
#include <RTShaderSystem/OgreShaderExNormalMapLighting.h>

class RenderRequest;
class GameMap;
class Building;
class Seat;
class Tile;
class GameEntity;
class MovableGameEntity;
class MapLight;
class Creature;
class RenderedMovableEntity;
class Weapon;

namespace Ogre
{
class SceneManager;
class SceneNode;
class OverlaySystem;

/*namespace RTShader {
    class ShaderGenerator;
}*/
} //End namespace Ogre

class RenderManager: public Ogre::Singleton<RenderManager>
{
    friend class RenderRequest;
public:
    RenderManager(Ogre::OverlaySystem* overlaySystem);
    ~RenderManager();

    inline Ogre::SceneManager* getSceneManager() const
    { return mSceneManager; }

    inline void setGameMap(GameMap* gameMap)
    { mGameMap = gameMap; }

    //! \brief Loop through the render requests in the queue and process them
    void processRenderRequests();
    void updateAnimations();

    //! \brief starts the compositor compositorName.
    void triggerCompositor(const std::string& compositorName);

    //! \brief setup the scene
    void createScene(Ogre::Viewport*);

    //! \brief Put a render request in the queue (helper function to avoid having to fetch the singleton)
    static void queueRenderRequest(RenderRequest* renderRequest)
    {
        msSingleton->queueRenderRequest_priv(renderRequest);
    }

    Ogre::SceneNode* getCreatureSceneNode()
    {
        return mCreatureSceneNode;
    }

    void rtssTest();

    //! \brief Colorize an entity with the team corresponding color.
    //! \Note: if the entity is marked for digging (wall tiles only), then a yellow color
    //! is added to the current colorization.
    void colourizeEntity(Ogre::Entity* ent, Seat* seat, bool markedForDigging = false);

    //! \brief Set the entity's opacity
    void setEntityOpacity(Ogre::Entity* ent, float opacity);

    static const Ogre::Real BLENDER_UNITS_PER_OGRE_UNIT;

    //! Debug function to be used for dev only. Beware, it should not be called from the server thread
    static std::string consoleListAnimationsForMesh(const std::string& meshName);

private:
    //! \brief Put a render request in the queue (implementation)
    void queueRenderRequest_priv(RenderRequest* renderRequest);

    //Render request functions
    void rrRefreshTile(Tile* curTile);
    void rrCreateTile(Tile* curTile);
    void rrDestroyTile(Tile* curTile);
    void rrDetachEntity(GameEntity* curEntity);
    void rrAttachEntity(GameEntity* curEntity);
    void rrTemporalMarkTile(Tile* curTile);
    void rrShowSquareSelector(const Ogre::Real& xPos, const Ogre::Real& yPos);
    void rrCreateBuilding(Building* curBuilding, Tile* curTile);
    void rrDestroyBuilding(Building* curBuilding, Tile* curTile);
    void rrCreateRenderedMovableEntity(RenderedMovableEntity* curRenderedMovableEntity);
    void rrDestroyRenderedMovableEntity(RenderedMovableEntity* curRenderedMovableEntity);
    void rrUpdateEntityOpacity(GameEntity* entity);
    void rrCreateCreature(Creature* curCreature);
    void rrDestroyCreature(Creature* curCreature);
    void rrOrientSceneNodeToward(MovableGameEntity* gameEntity, const Ogre::Vector3& direction);
    void rrScaleSceneNode(Ogre::SceneNode* node, const Ogre::Vector3& scale);
    void rrCreateWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand);
    void rrDestroyWeapon(Creature* curCreature, const Weapon* curWeapon, const std::string& hand);
    void rrCreateMapLight(MapLight* curMapLight, bool displayVisual);
    void rrDestroyMapLight(MapLight* curMapLight);
    void rrDestroyMapLightVisualIndicator(MapLight* curMapLight);
    void rrPickUpEntity(GameEntity* curEntity);
    void rrDropHand(GameEntity* curEntity);
    void rrRotateHand();
    void rrCreateCreatureVisualDebug(Creature* curCreature, Tile* curTile);
    void rrDestroyCreatureVisualDebug(Creature* curCreature, Tile* curTile);
    void rrSetObjectAnimationState(MovableGameEntity* curAnimatedObject, const std::string& animation, bool loop);
    void rrMoveSceneNode(const std::string& sceneNodeName, const Ogre::Vector3& position);
    void rrCarryEntity(Creature* carrier, GameEntity* carried);
    void rrReleaseCarriedEntity(Creature* carrier, GameEntity* carried);

    bool generateRTSSShadersForMaterial(const std::string& materialName,
                                        const std::string& normalMapTextureName = "",
                                        Ogre::RTShader::NormalMapLighting::NormalMapSpace nmSpace = Ogre::RTShader::NormalMapLighting::NMS_TANGENT);

    Ogre::Entity* createEntity(const std::string& entityName, const std::string& meshName,
                               const std::string& normalMapTextureName = "");

    //! \brief Colorize the material with the corresponding team id color.
    //! \note If the material (wall tiles only) is marked for digging, a yellow color is added
    //! to the given color.
    //! \returns The new material name according to the current colorization.
    std::string colourizeMaterial(const std::string& materialName, Seat* seat, bool markedForDigging = false);

    //! \brief Makes the material be transparent with the given opacity (0.0f - 1.0f)
    //! \returns The new material name according to the current opacity.
    std::string setMaterialOpacity(const std::string& materialName, float opacity);

    std::deque<RenderRequest*> mRenderQueue;

    //! \brief The main scene manager reference. Don't delete it.
    Ogre::SceneManager* mSceneManager;

    //! \brief Reference to the Ogre sub scene nodes. Don't delete them.
    Ogre::SceneNode* mRoomSceneNode;
    Ogre::SceneNode* mCreatureSceneNode;
    Ogre::SceneNode* mLightSceneNode;
    Ogre::SceneNode* mRockSceneNode;

    //! \brief The game map reference. Don't delete it.
    GameMap* mGameMap;

    Ogre::Viewport* mViewport;
    Ogre::RTShader::ShaderGenerator* mShaderGenerator;
    bool mInitialized;
};

#endif // RENDERMANAGER_H_
