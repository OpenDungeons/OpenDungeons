void RenderManager::rrDestroyMissileObject ( const RenderRequest& renderRequest )
{
    MissileObject* curMissileObject = static_cast<MissileObject*>(renderRequest.p);
    if (sceneManager->hasEntity(curMissileObject->getName()))
    {
        Ogre::Entity* ent = sceneManager->getEntity(curMissileObject->getName());
        Ogre::SceneNode* node = sceneManager->getSceneNode(curMissileObject->getName()  + "_node");
        node->detachObject(ent);
        creatureSceneNode->removeChild(node);
        sceneManager->destroyEntity(ent);
        sceneManager->destroySceneNode(curMissileObject->getName() + "_node");
    }
}
