void RenderManager::rrDestroyRoom ( const RenderRequest& renderRequest )
{
    Room* curRoom = static_cast<Room*> ( renderRequest.p );
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );
    std::stringstream tempSS;
    tempSS << curRoom->getName() << "_" << curTile->x << "_"
    << curTile->y;
    if (sceneManager->hasEntity(tempSS.str()))
    {
        Ogre::Entity* ent = sceneManager->getEntity(tempSS.str());
        Ogre::SceneNode* node = sceneManager->getSceneNode(tempSS.str() + "_node");
        node->detachObject(ent);
        roomSceneNode->removeChild(node);
        sceneManager->destroyEntity(ent);
        sceneManager->destroySceneNode(tempSS.str() + "_node");
    }
}