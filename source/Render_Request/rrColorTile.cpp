
void RenderManager::rrColorTile ( const RenderRequest& renderRequest ){
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    Player* pp    = static_cast<Player*> ( renderRequest.p2 );
    Ogre::Entity *ent = NULL;
    char tempString[255];
    char tempString2[255];
    
    if  (renderRequest.b){
        curTile->setColor(pp->getSeat()->color);
    }
    else{
	curTile->setColor(0);
    }  
        curTile->refreshMesh();
}
