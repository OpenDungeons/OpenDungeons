void RenderManager::rrRotateCreaturesInHand ( const RenderRequest& )
{
    // Loop over the creatures in our hand and redraw each of them in their new location.
    for (unsigned int i = 0; i < gameMap->me->numCreaturesInHand(); ++i)
    {
        Creature* curCreature = gameMap->me->getCreatureInHand(i);
        Ogre::SceneNode* creatureNode = sceneManager->getSceneNode(curCreature->getName() + "_node");
        creatureNode->setPosition(i % 6 + 1, (i / (int) 6), 0.0);
    }
}
