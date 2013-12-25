void RenderManager::rrRefreshField ( const RenderRequest& renderRequest )
{
    Field* curField = static_cast<Field*> (renderRequest.p);
    //FIXME these vars should have proper names
    double* tempDoublePtr = static_cast<double*>(renderRequest.p2);
    double tempDouble = *tempDoublePtr;
    delete tempDoublePtr;

    // Update existing meshes and create any new ones needed.
    FieldType::iterator fieldItr = curField->begin();
    while (fieldItr != curField->end())
    {


        int x = fieldItr->first.first;
        int y = fieldItr->first.second;
        double tempDouble2 = fieldItr->second;

        std::stringstream tempSS;
        tempSS << "Field_" << curField->name << "_" << x << "_"
        << y;

        Ogre::SceneNode* fieldIndicatorNode = NULL;

        if (sceneManager->hasEntity(tempSS.str()))
        {
            // The mesh alread exists, just get the existing one
            fieldIndicatorNode = sceneManager->getSceneNode(tempSS.str() + "_node");
        }
        else
        {
            // The mesh does not exist, create a new one
            Ogre::Entity* fieldIndicatorEntity = sceneManager->createEntity(tempSS.str(),
                                                 "Field_indicator.mesh");
            fieldIndicatorNode = fieldSceneNode->createChildSceneNode(
                                     tempSS.str() + "_node");
            fieldIndicatorNode->attachObject(fieldIndicatorEntity);
        }

        fieldIndicatorNode->setPosition(x, y, tempDouble + tempDouble2);
        ++fieldItr;
    }

    //TODO:  This is not done yet.
    // Delete any meshes not in the field currently
}