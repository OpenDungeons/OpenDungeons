void RenderManager::rrCreateCreatureVisualDebug ( const RenderRequest& renderRequest )
{
    Tile* curTile = static_cast<Tile*>(renderRequest.p);
    Creature* curCreature = static_cast<Creature*>( renderRequest.p2);

    if (curTile != NULL && curCreature != NULL)
    {
        std::stringstream tempSS;
        tempSS << "Vision_indicator_" << curCreature->getName() << "_"
        << curTile->x << "_" << curTile->y;

        Ogre::Entity* visIndicatorEntity = sceneManager->createEntity(tempSS.str(),
                                           "Cre_vision_indicator.mesh");
        Ogre::SceneNode* visIndicatorNode = creatureSceneNode->createChildSceneNode(tempSS.str()
                                            + "_node");
        visIndicatorNode->attachObject(visIndicatorEntity);
        visIndicatorNode->setPosition(Ogre::Vector3(curTile->x, curTile->y, 0));
        visIndicatorNode->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT,
                                   BLENDER_UNITS_PER_OGRE_UNIT));
    }
}
