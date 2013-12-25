void RenderManager::rrCreateCreature ( const RenderRequest& renderRequest )
{
    Creature* curCreature = static_cast<Creature*>(renderRequest.p);
    std::string meshName = renderRequest.str;
    Ogre::Vector3 scale = renderRequest.vec;

    assert(curCreature != 0);
    //assert(curCreature->getDefinition() != 0);

    // Load the mesh for the creature
    Ogre::Entity* ent = sceneManager->createEntity("Creature_" + curCreature->getName(),
                        meshName);
    Ogre::MeshPtr meshPtr = ent->getMesh();

    unsigned short src, dest;
    if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
    {
        meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    //Disabled temporarily for normal-mapping
    //colourizeEntity(ent, curCreature->color);
    Ogre::SceneNode* node = creatureSceneNode->createChildSceneNode(
                                curCreature->getName() + "_node");
    curCreature->sceneNode = node;
    node->setPosition(curCreature->getPosition());
    node->setScale(scale);
    node->attachObject(ent);
}