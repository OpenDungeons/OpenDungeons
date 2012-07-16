


void RenderManager::rrDetachTile( const RenderRequest& renderRequest ){
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    Ogre::SceneNode* tileNode = sceneManager->getSceneNode( curTile->getName() + "_node" );   

    curTile->pSN=(tileNode->getParentSceneNode());
    curTile->pSN->removeChild(tileNode);

}
