void RenderManager::rrRefreshTile ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );

    if ( sceneManager->hasSceneNode ( curTile->getName() + "_node" ) )
    {

        // Unlink and delete the old mesh
        sceneManager->getSceneNode ( curTile->getName() + "_node" )->detachObject (
            curTile->getName() );
        sceneManager->destroyEntity ( curTile->getName() );

        Ogre::Entity* ent = sceneManager->createEntity ( curTile->getName(),
                            Tile::meshNameFromFullness(curTile->getType(),
                                                       curTile->getFullnessMeshNumber()) );
        /*        Ogre::Entity* ent = createEntity(curTile->name,
                                    Tile::meshNameFromFullness(curTile->getType(),
                                                               curTile->getFullnessMeshNumber()), "Claimedwall2_nor3.png");*/

        colourizeEntity ( ent, curTile->getColor() );

        // Link the tile mesh back to the relevant scene node so OGRE will render it
        Ogre::SceneNode* node = sceneManager->getSceneNode ( curTile->getName() + "_node" );
        node->attachObject ( ent );
        node->resetOrientation();
        node->roll ( Ogre::Degree ( curTile->rotation ) );
    }
}
