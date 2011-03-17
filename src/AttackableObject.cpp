#include "AttackableObject.h"

std::vector<AttackableObject*> AttackableObject::removeDeadObjects(
        const std::vector<AttackableObject*> &objects)
{
    std::vector<AttackableObject*> ret;
    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        if (objects[i]->getHP(NULL) > 0.0)
            ret.push_back(objects[i]);
    }

    return ret;
}

