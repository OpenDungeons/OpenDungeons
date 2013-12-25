void RenderManager::rrDropCreature ( const RenderRequest& renderRequest )
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    Player* curPlayer = static_cast<Player*> (renderRequest.p2);
    // Detach the creature from the "hand" scene node
    Ogre::SceneNode* creatureNode = sceneManager->getSceneNode(curCreature->getName() + "_node");
    sceneManager->getSceneNode("Hand_node")->removeChild(creatureNode);

    // Attach the creature from the creature scene node
    creatureSceneNode->addChild(creatureNode);
    creatureNode->setPosition(curCreature->getPosition());
    creatureNode->scale(3.0, 3.0, 3.0);

    // Move the other creatures in the player's hand to replace the dropped one
    for (unsigned int i = 0; i < curPlayer->numCreaturesInHand(); ++i)
    {
        curCreature = curPlayer->getCreatureInHand(i);
        creatureNode = sceneManager->getSceneNode(curCreature->getName() + "_node");
        creatureNode->setPosition(i % 6 + 1, (i / (int) 6), 0.0);
    }
}
