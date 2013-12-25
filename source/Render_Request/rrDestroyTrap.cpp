void RenderManager::rrDestroyTrap ( const RenderRequest& renderRequest )
{
    Trap* curTrap = static_cast<Trap*>( renderRequest.p);
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );

    std::stringstream tempSS;
    tempSS << "Trap_" << curTrap->getName() + "_tile_" << curTile->x << "_"
    << curTile->y;
    std::string tempString = tempSS.str();
    Ogre::Entity* ent = sceneManager->getEntity(tempString);
    Ogre::SceneNode* node = sceneManager->getSceneNode(tempString + "_node");
    node->detachObject(ent);
    sceneManager->destroySceneNode(node->getName());
    sceneManager->destroyEntity(ent);
}
