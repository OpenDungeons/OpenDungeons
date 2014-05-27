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

using  std::string;

class RenderRequest;
class GameMap;

namespace Ogre
{
class SceneManager;
class SceneNode;


/*namespace RTShader {
    class ShaderGenerator;
}*/
} //End namespace Ogre

class RenderManager: public Ogre::Singleton<RenderManager>
{
public:
    RenderManager();
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
    void colourizeEntity(Ogre::Entity *ent, int colour);

    static const Ogre::Real BLENDER_UNITS_PER_OGRE_UNIT;

protected:
    //! \brief Put a render request in the queue (implementation)
    void queueRenderRequest_priv(RenderRequest* renderRequest);

    //Render request functions

    //TODO - could some of these be merged?
    void rrRefreshTile(const RenderRequest& renderRequest);
    void rrCreateTile(const RenderRequest& renderRequest);
    void rrDestroyTile(const RenderRequest& renderRequest);
    void rrDetachCreature(const RenderRequest& renderRequest);
    void rrAttachCreature(const RenderRequest& renderRequest);
    void rrDetachTile(const RenderRequest& renderRequest);
    void rrAttachTile(const RenderRequest& renderRequest);
    void rrToggleCreaturesVisibility();
    void rrColorTile(const RenderRequest& renderRequest);
    void rrSetPickAxe(const RenderRequest& renderRequest);
    void rrTemporalMarkTile(const RenderRequest& renderRequest);
    void rrShowSquareSelector(const RenderRequest& renderRequest);
    void rrCreateRoom(const RenderRequest& renderRequest);
    void rrDestroyRoom(const RenderRequest& renderRequest);
    void rrCreateRoomObject(const RenderRequest& renderRequest);
    void rrDestroyRoomObject(const RenderRequest& renderRequest);
    void rrCreateTrap(const RenderRequest& renderRequest);
    void rrDestroyTrap(const RenderRequest& renderRequest);
    void rrCreateTreasuryIndicator(const RenderRequest& renderRequest);
    void rrDestroyTreasuryIndicator(const RenderRequest& renderRequest);
    void rrCreateCreature(const RenderRequest& renderRequest);
    void rrDestroyCreature(const RenderRequest& renderRequest);
    void rrOrientSceneNodeToward(const RenderRequest& renderRequest);
    void rrReorientSceneNode(const RenderRequest& renderRequest);
    void rrScaleSceneNode(const RenderRequest& renderRequest);
    void rrCreateWeapon(const RenderRequest& renderRequest);
    void rrDestroyWeapon(const RenderRequest& renderRequest);
    void rrCreateMissileObject(const RenderRequest& renderRequest);
    void rrDestroyMissileObject(const RenderRequest& renderRequest);
    void rrCreateMapLight(const RenderRequest& renderRequest);
    void rrDestroyMapLight(const RenderRequest& renderRequest);
    void rrDestroyMapLightVisualIndicator(const RenderRequest& renderRequest);
    void rrCreateField(const RenderRequest& renderRequest);
    void rrRefreshField(const RenderRequest& renderRequest);
    void rrPickUpCreature(const RenderRequest& renderRequest);
    void rrDropCreature(const RenderRequest& renderRequest);
    void rrRotateCreaturesInHand(const RenderRequest& renderRequest);
    void rrCreateCreatureVisualDebug(const RenderRequest& renderRequest);
    void rrDestroyCreatureVisualDebug(const RenderRequest& renderRequest);
    void rrSetObjectAnimationState(const RenderRequest& renderRequest);
    void rrMoveSceneNode(const RenderRequest& renderRequest);

    //! \brief Handle the renderRequest requested
    bool handleRenderRequest(const RenderRequest& renderRequest);

    bool generateRTSSShadersForMaterial(const std::string& materialName,
                                        const std::string& normalMapTextureName = "",
                                        Ogre::RTShader::NormalMapLighting::NormalMapSpace nmSpace = Ogre::RTShader::NormalMapLighting::NMS_TANGENT);

    Ogre::Entity* createEntity(const std::string& entityName, const std::string& meshName,
                               const std::string& normalMapTextureName = "");


    std::string colourizeMaterial(const std::string& materialName, int color);
private:
    bool mVisibleCreatures;

    std::deque<RenderRequest*> mRenderQueue;

    //! \brief The main scene manager reference. Don't delete it.
    Ogre::SceneManager* mSceneManager;

    //! \brief Reference to the Ogre sub scene nodes. Don't delete them.
    Ogre::SceneNode* mRoomSceneNode;
    Ogre::SceneNode* mCreatureSceneNode;
    Ogre::SceneNode* mLightSceneNode;
    Ogre::SceneNode* mFieldSceneNode;
    Ogre::SceneNode* mRockSceneNode;

    //! \brief The game map reference. Don't delete it.
    GameMap* mGameMap;

    Ogre::Viewport* mViewport;
    Ogre::RTShader::ShaderGenerator* mShaderGenerator;
    bool mInitialized;
};

#endif // RENDERMANAGER_H_
