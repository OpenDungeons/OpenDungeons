void RenderManager::rrCreateRoomObject ( const RenderRequest& renderRequest )
{
    RoomObject* curRoomObject = static_cast<RoomObject*> (renderRequest.p);
    std::string name = renderRequest.str;
    boost::scoped_ptr<std::string> meshName(static_cast<std::string*>(renderRequest.p3));
    //TODO - find out why this was here
    //Room* curRoom = static_cast<Room*> ( renderRequest.p2 );

    std::string tempString = curRoomObject->getOgreNamePrefix()
                             + name;
    Ogre::Entity* ent = sceneManager->createEntity(tempString,
                        *meshName.get() + ".mesh");
    Ogre::SceneNode* node = roomSceneNode->createChildSceneNode(tempString
                            + "_node");
    node->setPosition(Ogre::Vector3(curRoomObject->x, curRoomObject->y, 0.0));
    node->roll(Ogre::Degree(curRoomObject->rotationAngle));
    node->attachObject(ent);
}
