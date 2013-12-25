void RenderManager::rrCreateTrap ( const RenderRequest& renderRequest )
{
    Trap* curTrap = static_cast<Trap*>( renderRequest.p);
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );

    std::stringstream tempSS;
    tempSS << "Trap_" << curTrap->getName() + "_tile_"
    << curTile->x << "_" << curTile->y;
    std::string tempString = tempSS.str();
    Ogre::Entity* ent = sceneManager->createEntity(tempString,
                        curTrap->getMeshName() + ".mesh");
    Ogre::SceneNode* node = roomSceneNode->createChildSceneNode(tempString
                            + "_node");
    node->setPosition(static_cast<Ogre::Real>(curTile->x),
                      static_cast<Ogre::Real>(curTile->y),
                      0.0f);
    node->attachObject(ent);
}