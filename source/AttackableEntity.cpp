#include "AttackableEntity.h"

AttackableEntity::AttackableEntity() :
    gameMap(0)
{

}

std::vector<AttackableEntity*> AttackableEntity::removeDeadObjects(
        const std::vector<AttackableEntity*> &objects)
{
    std::vector<AttackableEntity*> ret;
    for(unsigned int i = 0, size = objects.size(); i < size; ++i)
    {
        if (objects[i]->getHP(NULL) > 0.0)
            ret.push_back(objects[i]);
    }

    return ret;
}

