void RenderManager::rrDestroyTile ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );

        if ( sceneManager->hasEntity ( curTile->getName() ) )
        {
            Ogre::Entity* ent = sceneManager->getEntity ( curTile->getName() );
            Ogre::SceneNode* node = sceneManager->getSceneNode ( curTile->getName() + "_node" );
            node->detachAllObjects();
            sceneManager->destroySceneNode ( curTile->getName() + "_node" );
            sceneManager->destroyEntity ( ent );
        }    
    
    
    // if (curTile->getType() == Tile::rock) {}
    // else {

    // }
}

