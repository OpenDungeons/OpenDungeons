void RenderManager::rrCreateField ( const RenderRequest& renderRequest )
{
    Field* curField = static_cast<Field*> (renderRequest.p);
    //FIXME these vars should have proper names
    double* tempDoublePtr = static_cast<double*>(renderRequest.p2);
    Ogre::Real tempDouble = static_cast<Ogre::Real>(*tempDoublePtr);
    delete tempDoublePtr;

    FieldType::iterator fieldItr = curField->begin();
    while (fieldItr != curField->end())
    {
        int x = fieldItr->first.first;
        int y = fieldItr->first.second;
        Ogre::Real tempDouble2 = static_cast<Ogre::Real>(fieldItr->second);
        //cout << "\ncreating field tile:  " << tempX << "
        //"\t" << tempY << "\t" << tempDouble;
        std::stringstream tempSS;
        tempSS << "Field_" << curField->name << "_" << x << "_"
        << y;
        Ogre::Entity* fieldIndicatorEntity = sceneManager->createEntity(tempSS.str(),
                                             "Field_indicator.mesh");
        Ogre::SceneNode* fieldIndicatorNode = fieldSceneNode->createChildSceneNode(tempSS.str()
                                              + "_node");
        fieldIndicatorNode->setPosition(static_cast<Ogre::Real>(x)
                                        , static_cast<Ogre::Real>(y)
                                        , tempDouble + tempDouble2);
        fieldIndicatorNode->attachObject(fieldIndicatorEntity);

        ++fieldItr;
    }
}