void RenderManager::rrCreateRoom ( const RenderRequest& renderRequest )
{
    Room* curRoom = static_cast<Room*> ( renderRequest.p );
    Tile* curTile = static_cast<Tile*> ( renderRequest.p2 );

    std::stringstream tempSS;
    tempSS << curRoom->getName() << "_" << curTile->x << "_"
    << curTile->y;
    Ogre::Entity* ent = sceneManager->createEntity ( tempSS.str(), curRoom->getMeshName() + ".mesh" );
    Ogre::SceneNode* node = roomSceneNode->createChildSceneNode ( tempSS.str()
                            + "_node" );
    node->setPosition ( static_cast<Ogre::Real>(curTile->x),
                        static_cast<Ogre::Real>(curTile->y),
                        static_cast<Ogre::Real>(0.0f));
    node->setScale ( Ogre::Vector3 ( BLENDER_UNITS_PER_OGRE_UNIT,
                                     BLENDER_UNITS_PER_OGRE_UNIT,
                                     BLENDER_UNITS_PER_OGRE_UNIT ) );
    node->attachObject ( ent );
}