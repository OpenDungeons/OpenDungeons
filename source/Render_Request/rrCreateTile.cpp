void RenderManager::rrCreateTile ( const RenderRequest& renderRequest )
{
    Ogre::SceneNode* node;
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );

    Ogre::Entity* ent = sceneManager->createEntity( curTile->getName(),
                        Tile::meshNameFromFullness(curTile->getType(),
                                                   curTile->getFullnessMeshNumber()));
    if (curTile->getType() == Tile::claimed)
    {
        colourizeEntity ( ent, curTile->getColor() );
    }

    node = sceneManager->getRootSceneNode()->createChildSceneNode (
								   curTile->getName() + "_node" );


    // if (curTile->getType() == Tile::rock) {
    //     node = rockSceneNode;


    // }
    // else {

    // }
    Ogre::MeshPtr meshPtr = ent->getMesh();
    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    //node->setPosition(Ogre::Vector3(x/BLENDER_UNITS_PER_OGRE_UNIT, y/BLENDER_UNITS_PER_OGRE_UNIT, 0));
    node->setPosition ( static_cast<Ogre::Real>(curTile->x)
                        , static_cast<Ogre::Real>(curTile->y),
                        0);
    node->attachObject ( ent );

    node->setScale ( Ogre::Vector3 ( BLENDER_UNITS_PER_OGRE_UNIT,
                                     BLENDER_UNITS_PER_OGRE_UNIT,
                                     BLENDER_UNITS_PER_OGRE_UNIT ) );
    node->resetOrientation();
    node->roll ( Ogre::Degree ( curTile->rotation ) );

}