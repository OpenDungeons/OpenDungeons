void RenderManager::rrCreateMapLight ( const RenderRequest& renderRequest )
{
    MapLight* curMapLight = static_cast<MapLight*> (renderRequest.p);

    // Create the light and attach it to the lightSceneNode.
    std::string mapLightName = "MapLight_" + curMapLight->getName();
    Ogre::Light* light = sceneManager->createLight(mapLightName);
    light->setDiffuseColour(curMapLight->getDiffuseColor());
    light->setSpecularColour(curMapLight->getSpecularColor());
    light->setAttenuation(curMapLight->getAttenuationRange(),
                          curMapLight->getAttenuationConstant(),
                          curMapLight->getAttenuationLinear(),
                          curMapLight->getAttenuationQuadratic());

    // Create the base node that the "flicker_node" and the mesh attach to.
    Ogre::SceneNode* mapLightNode = lightSceneNode->createChildSceneNode(mapLightName
                                    + "_node");
    mapLightNode->setPosition(curMapLight->getPosition());

    //TODO - put this in request so we don't have to include the globals here.
    if (renderRequest.b)
    {
        // Create the MapLightIndicator mesh so the light can be drug around in the map editor.
        Ogre::Entity* lightEntity = sceneManager->createEntity("MapLightIndicator_"
                                    + curMapLight->getName(), "Light.mesh");
        mapLightNode->attachObject(lightEntity);
    }

    // Create the "flicker_node" which moves around randomly relative to
    // the base node.  This node carries the light itself.
    Ogre::SceneNode* flickerNode
    = mapLightNode->createChildSceneNode(mapLightName
                                         + "_flicker_node");
    flickerNode->attachObject(light);
}
