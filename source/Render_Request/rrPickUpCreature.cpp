void RenderManager::rrPickUpCreature ( const RenderRequest& renderRequest )
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    // Detach the creature from the creature scene node
    Ogre::SceneNode* creatureNode = sceneManager->getSceneNode(curCreature->getName() + "_node");
    //FIXME this variable name is a bit misleading
    creatureSceneNode->removeChild(creatureNode);

    // Attatch the creature to the hand scene node
    sceneManager->getSceneNode("Hand_node")->addChild(creatureNode);
    //FIXME we should probably use setscale for this, because of rounding.
    creatureNode->scale(0.333, 0.333, 0.333);

    // Move the other creatures in the player's hand to make room for the one just picked up.
    for (unsigned int i = 0; i < gameMap->me->numCreaturesInHand(); ++i)
    {
        curCreature = gameMap->me->getCreatureInHand(i);
        creatureNode = sceneManager->getSceneNode(curCreature->getName() + "_node");
        creatureNode->setPosition(i % 6 + 1, (i / (int) 6), 0.0);
    }
}