void RenderManager::rrCreateMissileObject ( const RenderRequest& renderRequest )
{
    MissileObject* curMissileObject = static_cast<MissileObject*>(renderRequest.p);
    Ogre::Entity* ent = sceneManager->createEntity(curMissileObject->getName(),
                        curMissileObject->getMeshName() + ".mesh");
    //TODO:  Make a new subroot scene node for these so lookups are faster since only a few missile objects should be onscreen at once.
    Ogre::SceneNode* node = creatureSceneNode->createChildSceneNode(
                                curMissileObject->getName() + "_node");
    node->setPosition(curMissileObject->getPosition());
    node->attachObject(ent);
}