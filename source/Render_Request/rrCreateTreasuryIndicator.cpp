void RenderManager::rrCreateTreasuryIndicator ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    Room* curRoom = static_cast<Room*> ( renderRequest.p2 );
    std::stringstream tempSS;

    tempSS << curRoom->getName() << "_" << curTile->x << "_"
    << curTile->y;
    Ogre::Entity* ent = sceneManager->createEntity(tempSS.str()
                        + "_treasury_indicator", renderRequest.str + ".mesh");
    Ogre::SceneNode* node = sceneManager->getSceneNode(tempSS.str() + "_node");

    //FIXME: This second scene node is purely to cancel out the effects of BLENDER_UNITS_PER_OGRE_UNIT, it can be gotten rid of when that hack is fixed.
    node = node->createChildSceneNode(node->getName()
                                      + "_hack_node");
    node->setScale(Ogre::Vector3(1.0 / BLENDER_UNITS_PER_OGRE_UNIT,
                                 1.0 / BLENDER_UNITS_PER_OGRE_UNIT, 1.0
                                 / BLENDER_UNITS_PER_OGRE_UNIT));

    node->attachObject(ent);
}
