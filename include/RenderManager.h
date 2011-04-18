/*!
 *  \file   RenderManager.h
 *  \date   26 March 2001
 *  \author oln
 *  \brief  handles the render requests
 */

#ifndef RENDERMANAGER_H_
#define RENDERMANAGER_H_

#include <deque>

#include <OgreSingleton.h>
#include <semaphore.h>

class RenderRequest;
class GameMap;
namespace Ogre
{
class SceneManager;
class SceneNode;
}

class RenderManager: public Ogre::Singleton<RenderManager>
{
    public:
        RenderManager(GameMap* gameMap);
        ~RenderManager();
        static RenderManager& getSingleton();
        static RenderManager* getSingletonPtr();
        void setSceneNodes(Ogre::SceneNode* roomSceneNode,
                                    Ogre::SceneNode* creatureSceneNode, Ogre::SceneNode* lightSceneNode, Ogre::SceneNode* fieldSceneNode );

        void processRenderRequests();
        void updateAnimations();

        void waitOnRenderQueueFlush();
        /*! \brief Put a render request in the queue (helper function to avoid having to fetch the singleton)
        */
        static void queueRenderRequest(RenderRequest* renderRequest)
        {
            ms_Singleton->queueRenderRequest_priv(renderRequest);
        }
        Ogre::SceneManager* sceneManager;

    protected:
        void queueRenderRequest_priv(RenderRequest* renderRequest);
        //Render request functions

        //TODO - could some of these be merged?
        void rrRefreshTile(const RenderRequest& renderRequest);
        void rrCreateTile(const RenderRequest& renderRequest);
        void rrDestroyTile(const RenderRequest& renderRequest);
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
    private:
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
    
        bool initialized;

        GameMap* gameMap;
};

#endif /* RENDERMANAGER_H_ */
