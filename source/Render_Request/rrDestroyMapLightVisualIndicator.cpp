void RenderManager::rrDestroyMapLightVisualIndicator ( const RenderRequest& renderRequest )
{
    MapLight* curMapLight = static_cast<MapLight*> (renderRequest.p);
    std::string mapLightName = "MapLight_" + curMapLight->getName();
    if (sceneManager->hasLight(mapLightName))
    {
        Ogre::SceneNode* mapLightNode = sceneManager->getSceneNode(mapLightName + "_node");
        std::string mapLightIndicatorName = "MapLightIndicator_"
                                            + curMapLight->getName();
        if (sceneManager->hasEntity(mapLightIndicatorName))
        {
            Ogre::Entity* mapLightIndicatorEntity = sceneManager->getEntity(mapLightIndicatorName);
            mapLightNode->detachObject(mapLightIndicatorEntity);
            sceneManager->destroyEntity(mapLightIndicatorEntity);
            //NOTE: This line throws an error complaining 'scene node not found' that should not be happening.
            //sceneManager->destroySceneNode(node->getName());
        }
    }
}