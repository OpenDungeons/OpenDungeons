void RenderManager::rrDestroyCreature ( const RenderRequest& renderRequest )
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    if (sceneManager->hasEntity("Creature_" + curCreature->getName()))
    {
        Ogre::Entity* ent = sceneManager->getEntity("Creature_" + curCreature->getName());
        Ogre::SceneNode* node = sceneManager->getSceneNode(curCreature->getName() + "_node");
        node->detachObject(ent);
        creatureSceneNode->removeChild(node);
        sceneManager->destroyEntity(ent);
        sceneManager->destroySceneNode(curCreature->getName() + "_node");
    }
    curCreature->sceneNode = NULL;
}