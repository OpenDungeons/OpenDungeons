void RenderManager::rrOrientSceneNodeToward ( const RenderRequest& renderRequest )
{
    Ogre::SceneNode* node = sceneManager->getSceneNode(renderRequest.str);
    Ogre::Vector3 tempVector = node->getOrientation()
                               * Ogre::Vector3::NEGATIVE_UNIT_Y;

    // Work around 180 degree quaternion rotation quirk
    if ((1.0f + tempVector.dotProduct(renderRequest.vec)) < 0.0001f)
    {
        node->roll(Ogre::Degree(180));
    }
    else
    {
        node->rotate(tempVector.getRotationTo(renderRequest.vec));
    }
}
