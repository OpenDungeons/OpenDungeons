void RenderManager::rrDestroyCreatureVisualDebug ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*>(renderRequest.p);
    Creature* curCreature = static_cast<Creature*>( renderRequest.p2);

    std::stringstream tempSS;
    tempSS << "Vision_indicator_" << curCreature->getName() << "_"
    << curTile->x << "_" << curTile->y;
    if (sceneManager->hasEntity(tempSS.str()))
    {
        Ogre::Entity* visIndicatorEntity = sceneManager->getEntity(tempSS.str());
        Ogre::SceneNode* visIndicatorNode = sceneManager->getSceneNode(tempSS.str() + "_node");

        visIndicatorNode->detachAllObjects();
        sceneManager->destroyEntity(visIndicatorEntity);
        sceneManager->destroySceneNode(visIndicatorNode);
    }
}