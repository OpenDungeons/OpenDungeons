/*!
 *  \file   RenderManager.h
 *  \date   26 March 2011
 *  \author oln
 *  \brief  handles the render requests
 */

#ifndef RENDERMANAGER_H_
#define RENDERMANAGER_H_

#include <deque>

#include <OgreSingleton.h>
#include <RTShaderSystem/OgreShaderGenerator.h>
#include <RTShaderSystem/OgreShaderExNormalMapLighting.h>
#include <semaphore.h>

template<typename T> class ProtectedObject;
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

        void setSceneNodes(Ogre::SceneNode* rockSceneNode, Ogre::SceneNode* roomSceneNode,
                                    Ogre::SceneNode* creatureSceneNode, Ogre::SceneNode* lightSceneNode, Ogre::SceneNode* fieldSceneNode );

        inline Ogre::Camera* getCamera()const {return mainCamera;}
        inline Ogre::SceneManager* getSceneManager()const {return sceneManager;}
        inline void setGameMap(GameMap* gameMap) {this->gameMap = gameMap;}

        void processRenderRequests();
        void updateAnimations();
        void createCamera();
        void createViewports();
        void createScene();

        void waitOnRenderQueueFlush();
        /*! \brief Put a render request in the queue (helper function to avoid having to fetch the singleton)
        */
        static void queueRenderRequest(RenderRequest* renderRequest)
        {
            msSingleton->queueRenderRequest_priv(renderRequest);
        }

        void rtssTest(); 
        void colourizeEntity(Ogre::Entity *ent, int colour);	
	static const Ogre::Real BLENDER_UNITS_PER_OGRE_UNIT;


    protected:
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
	void rrSetPickAxe( const RenderRequest& renderRequest ); 
	void rrTemporalMarkTile ( const RenderRequest& renderRequest );
	void rrShowSquareSelector  ( const RenderRequest& renderRequest );
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

        bool handleRenderRequest(const RenderRequest& renderRequest);
        bool generateRTSSShadersForMaterial(const std::string& materialName,
                                            const std::string& normalMapTextureName = "",
                                            Ogre::RTShader::NormalMapLighting::NormalMapSpace nmSpace = Ogre::RTShader::NormalMapLighting::NMS_TANGENT);
        Ogre::Entity* createEntity(const std::string& entityName, const std::string& meshName,
                          const std::string& normalMapTextureName = "");


        std::string colourizeMaterial(const std::string& materialName, int color);
    private:
        bool visibleCreatures;
        RenderManager(const RenderManager&);
        //TODO -should we maybe encapsulate the semaphores somewhere?
        sem_t renderQueueSemaphore;
        sem_t renderQueueEmptySemaphore;
        std::deque<RenderRequest*> renderQueue;
        //TODO - these should probably be defined in here instead of in the frame listener
        //FIXME - may want to rename these to make the functionality clearer
        Ogre::SceneNode* roomSceneNode;
        Ogre::SceneNode* creatureSceneNode;
        Ogre::SceneNode* lightSceneNode;
        Ogre::SceneNode* fieldSceneNode;
	Ogre::SceneNode* rockSceneNode;
        GameMap* gameMap;
        Ogre::Camera* mainCamera;
        Ogre::SceneManager* sceneManager;
        Ogre::Viewport* viewport;
        Ogre::RTShader::ShaderGenerator* shaderGenerator;
        bool initialized;

        static ProtectedObject<unsigned int> numThreadsWaitingOnRenderQueueEmpty;




};

#endif /* RENDERMANAGER_H_ */
