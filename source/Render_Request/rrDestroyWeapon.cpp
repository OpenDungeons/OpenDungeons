void RenderManager::rrDestroyWeapon ( const RenderRequest& renderRequest )
{
    Weapon* curWeapon = static_cast<Weapon*>( renderRequest.p);
    Creature* curCreature = static_cast<Creature*>(renderRequest.p2);

    if (curWeapon->getName().compare("none") != 0)
    {
        Ogre::Entity* ent = sceneManager->getEntity("Weapon_"
                            + curWeapon->getHandString() + "_" + curCreature->getName());
        sceneManager->destroyEntity(ent);
    }
}
