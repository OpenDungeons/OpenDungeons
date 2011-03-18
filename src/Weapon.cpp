#include "Globals.h"
#include "Functions.h"
#include "Weapon.h"
#include "RenderRequest.h"
#include "Creature.h"

Weapon::Weapon()
{
    meshExists = false;
}

void Weapon::createMesh()
{
    if (meshExists)
        return;

    meshExists = true;

    if (name.compare("none") == 0)
    {
        return;
    }

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::createWeapon;
    request->p = this;
    request->p2 = parentCreature;
    request->p3 = &handString;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

void Weapon::destroyMesh()
{
    if (!meshExists)
        return;

    meshExists = false;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyWeapon;
    request->p = this;
    request->p2 = parentCreature;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

void Weapon::deleteYourself()
{
    if (meshExists)
        destroyMesh();

    // Create a render request asking the render queue to actually do the deletion of this creature.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::deleteWeapon;
    request->p = this;

    // Add the requests to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

std::string Weapon::getFormat()
{
    //NOTE:  When this format changes changes to RoomPortal::spawnCreature() may be necessary.
    return "name\tdamage\trange\tdefense";
}

std::ostream& operator<<(std::ostream& os, Weapon *w)
{
    os << w->name << "\t" << w->damage << "\t" << w->range << "\t"
            << w->defense;

    return os;
}

std::istream& operator>>(std::istream& is, Weapon *w)
{
    is >> w->name >> w->damage >> w->range >> w->defense;
    w->meshName = w->name + ".mesh";

    return is;
}

