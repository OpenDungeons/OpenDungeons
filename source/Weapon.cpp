#include "Weapon.h"

std::string Weapon::getFormat()
{
    //NOTE:  When this format changes changes to RoomPortal::spawnCreature() may be necessary.
    return "name\tdamage\trange\tdefense";
}

std::ostream& operator<<(std::ostream& os, Weapon *w)
{
    os << w->getName() << "\t" << w->damage << "\t" << w->range << "\t" << w->defense;
    return os;
}

std::istream& operator>>(std::istream& is, Weapon *w)
{
    std::string name;
    is >> name >> w->damage >> w->range >> w->defense;
    w->setName(name);
    w->setMeshName(name + ".mesh");

    return is;
}
