void RenderManager::rrTemporalMarkTile ( const RenderRequest& renderRequest ){
    

    Ogre::Entity* ent;
    char tempString[255];


    Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    Player* pp    = static_cast<Player*> ( renderRequest.p2 );	
    bool ss = renderRequest.b  ;

    snprintf(tempString, sizeof(tempString),
	     "Level_%i_%i_selection_indicator", curTile->x, curTile->y);
    if (sceneManager->hasEntity(tempString)){
            ent = sceneManager->getEntity(tempString);
        }
    else
        {
            char tempString2[255];
            ent = sceneManager->createEntity(tempString, "SquareSelector.mesh");
            snprintf(tempString2, sizeof(tempString2), "Level_%i_%i_node", curTile->x, curTile->y);

            sceneManager->getSceneNode(tempString2)->attachObject(ent);
        }


    /*! \brief Well this should go where the pick up axe shows up
     *
     */


    ent->setVisible(ss);


}