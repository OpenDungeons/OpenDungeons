void RenderManager::rrScaleSceneNode( const RenderRequest& renderRequest )
{
    Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(renderRequest.p);

    if (node != NULL)
    {
        node->scale(renderRequest.vec);
    }
}
