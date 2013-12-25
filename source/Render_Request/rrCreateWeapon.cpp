void RenderManager::rrCreateWeapon ( const RenderRequest& renderRequest )
{
    Weapon* curWeapon = static_cast<Weapon*>( renderRequest.p);
    Creature* curCreature = static_cast<Creature*>(renderRequest.p2);

    Ogre::Entity* ent = sceneManager->getEntity("Creature_" + curCreature->getName());
    //colourizeEntity(ent, curCreature->color);
    Ogre::Entity* weaponEntity = sceneManager->createEntity("Weapon_"
                                 + curWeapon->getHandString() + "_" + curCreature->getName(),
                                 curWeapon->getMeshName());
    Ogre::Bone* weaponBone = ent->getSkeleton()->getBone(
                                 "Weapon_" + curWeapon->getHandString());

    // Rotate by -90 degrees around the x-axis from the bone's rotation.
    Ogre::Quaternion rotationQuaternion;
    rotationQuaternion.FromAngleAxis(Ogre::Degree(-90.0), Ogre::Vector3(1.0,
                                     0.0, 0.0));

    ent->attachObjectToBone(weaponBone->getName(), weaponEntity,
                            rotationQuaternion);
}