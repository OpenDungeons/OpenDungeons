void RenderManager::rrDestroyMapLight ( const RenderRequest& renderRequest )
{
    MapLight* curMapLight = static_cast<MapLight*> (renderRequest.p);
    std::string mapLightName = "MapLight_" + curMapLight->getName();
    if (sceneManager->hasLight(mapLightName))
    {
        Ogre::Light* light = sceneManager->getLight(mapLightName);
        Ogre::SceneNode* lightNode = sceneManager->getSceneNode(mapLightName + "_node");
        Ogre::SceneNode* lightFlickerNode = sceneManager->getSceneNode(mapLightName
                                            + "_flicker_node");
        lightFlickerNode->detachObject(light);
        lightSceneNode->removeChild(lightNode);
        sceneManager->destroyLight(light);

        if (sceneManager->hasEntity(mapLightName))
        {
            Ogre::Entity* mapLightIndicatorEntity = sceneManager->getEntity("MapLightIndicator_"
                                                    + curMapLight->getName());
            lightNode->detachObject(mapLightIndicatorEntity);
        }
        sceneManager->destroySceneNode(lightFlickerNode->getName());
        sceneManager->destroySceneNode(lightNode->getName());
    }
}
