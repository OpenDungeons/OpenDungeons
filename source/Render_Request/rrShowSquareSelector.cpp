void RenderManager::rrShowSquareSelector( const RenderRequest& renderRequest ) {

    int* xPos = static_cast<int*> ( renderRequest.p );
    int* yPos = static_cast<int*> ( renderRequest.p2 );

    sceneManager->getEntity("SquareSelector")->setVisible(true);
    sceneManager->getSceneNode("SquareSelectorNode")->setPosition(
								  *xPos, *yPos, 0);




}
