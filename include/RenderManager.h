/*
 * RenderManager.h
 *
 *  Created on: 26. mars 2011
 *      Author: oln
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
    RenderManager();
    virtual ~RenderManager();
    static RenderManager& getSingleton();
    static RenderManager* getSingletonPtr();
    bool initialize(
        Ogre::SceneManager* sceneManager,
        GameMap* gameMap);
    void setSceneNodes(Ogre::SceneNode* roomSceneNode,
                                Ogre::SceneNode* creatureSceneNode, Ogre::SceneNode* lightSceneNode, Ogre::SceneNode* fieldSceneNode );

    void processRenderRequests();
    void updateAnimations();
    
    void waitOnRenderQueueFlush();
    static void queueRenderRequest(RenderRequest* renderRequest)
    {
        ms_Singleton->queueRenderRequest_priv(renderRequest);
    }
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
    //TODO -should we maybe encapsulate the semaphores somewhere?
    sem_t renderQueueSemaphore;
    sem_t renderQueueEmptySemaphore;
    std::deque<RenderRequest*> renderQueue;
    Ogre::SceneManager* sceneManager;
    //TODO - these should probably be defined in here instead of in the frame listener
    //NOTE - may want to rename these to make the functionality clearer
    Ogre::SceneNode* roomSceneNode;
    Ogre::SceneNode* creatureSceneNode;
    Ogre::SceneNode* lightSceneNode;
    Ogre::SceneNode* fieldSceneNode;
    GameMap* gameMap;

    bool initialized;
};

#endif /* RENDERMANAGER_H_ */
