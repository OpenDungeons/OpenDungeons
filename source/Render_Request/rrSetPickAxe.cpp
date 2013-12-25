void RenderManager::rrSetPickAxe( const RenderRequest& renderRequest ) {
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    bool ss =   renderRequest.b ;
    Ogre::Entity *ent = NULL;

    //FIXME:  This code should be moved over to the rendering thread and called via a RenderRequest

    if ( sceneManager->hasSceneNode ( curTile->getName() + "_node" ) ){
	snprintf(tempString, sizeof(tempString),
		 "Level_%i_%i_digging_indicator", curTile->x, curTile->y);
	if (sceneManager->hasEntity(tempString))
	    {
		ent = sceneManager->getEntity(tempString);
	    }
	else
	    {
		ent = sceneManager->createEntity(tempString, "DigSelector.mesh");
		snprintf(tempString2, sizeof(tempString2),
			 "Level_%i_%i_node", curTile->x, curTile->y);

		Ogre::SceneNode *tempNode =		    sceneManager->getSceneNode(tempString2);
		tempNode->attachObject(ent);
	    }



	if ((ent!=NULL))
	    ent->setVisible(ss);

    }

}