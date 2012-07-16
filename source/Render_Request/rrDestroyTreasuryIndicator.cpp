void RenderManager::rrDestroyTreasuryIndicator ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*> ( renderRequest.p );
    Room* curRoom = static_cast<Room*> ( renderRequest.p2 );

    std::stringstream tempSS;
    tempSS << curRoom->getName() << "_" << curTile->x << "_"
    << curTile->y;
    if (sceneManager->hasEntity(tempSS.str() + "_treasury_indicator"))
    {
        Ogre::Entity* ent = sceneManager->getEntity(tempSS.str()
                            + "_treasury_indicator");

        //FIXME: This second scene node is purely to cancel out the effects of BLENDER_UNITS_PER_OGRE_UNIT, it can be gotten rid of when that hack is fixed.
        Ogre::SceneNode* node = sceneManager->getSceneNode(tempSS.str() + "_node"
                                + "_hack_node");

        /*  The proper code once the above hack is fixed.
        node = sceneManager->getSceneNode(tempSS.str() + "_node");
        */
        node->detachObject(ent);

        //FIXME: This line is not needed once the above hack is fixed.
        sceneManager->destroySceneNode(node->getName());

        sceneManager->destroyEntity(ent);
    }
}
