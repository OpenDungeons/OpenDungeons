#include "Globals.h"
#include "Trap.h"
#include "TrapCannon.h"
#include "RenderRequest.h"
#include "Seat.h"
#include "GameMap.h"
#include "RenderManager.h"

const double Trap::defaultTileHP = 10.0;

Trap::Trap()
{
    controllingSeat = NULL;
    meshExists = false;
}

Trap* Trap::createTrap(TrapType nType, const std::vector<Tile*> &nCoveredTiles,
        Seat *nControllingSeat)
{
    Trap *tempTrap = NULL;

    switch (nType)
    {
        case nullTrapType:
            tempTrap = NULL;
            break;
        case cannon:
            tempTrap = new TrapCannon();
            break;
    }

    if (tempTrap == NULL)
    {
        std::cerr
                << "\n\n\nERROR: Trying to create a trap of unknown type, bailing out.\n";
        std::cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
                << "\n\n\n";
        exit(1);
    }

    tempTrap->controllingSeat = nControllingSeat;

    tempTrap->meshName = getMeshNameFromTrapType(nType);
    tempTrap->type = nType;

    static int uniqueNumber = -1;
    std::stringstream tempSS;

    tempSS.str("");
    tempSS << tempTrap->meshName << "_" << uniqueNumber;
    --uniqueNumber;
    tempTrap->name = tempSS.str();

    for (unsigned int i = 0; i < nCoveredTiles.size(); ++i)
        tempTrap->addCoveredTile(nCoveredTiles[i]);

    return tempTrap;
}

Trap* Trap::createTrapFromStream(std::istream &is)
{
    Trap tempTrap;
    is >> &tempTrap;

    Trap *returnTrap = createTrap(tempTrap.type, tempTrap.coveredTiles,
            tempTrap.controllingSeat);
    return returnTrap;
}

void Trap::createMeshes()
{
    if (meshExists)
        return;

    meshExists = true;

    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::createTrap;
        request->p = this;
        request->p2 = coveredTiles[i];

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    }
}

void Trap::destroyMeshes()
{
    if (!meshExists)
        return;

    meshExists = false;

    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::destroyTrap;
        request->p = static_cast<void*>(this);
        request->p2 = coveredTiles[i];

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    }
}

void Trap::deleteYourself()
{
    if (meshExists)
        destroyMeshes();

    // Create a render request asking the render queue to actually do the deletion of this creature.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::deleteTrap;
    request->p = static_cast<void*>(this);

    // Add the requests to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

std::string Trap::getMeshNameFromTrapType(TrapType t)
{
    switch (t)
    {
        case nullTrapType:
            return "NullTrapType";
            break;
        case cannon:
            return "Cannon";
            break;
    }

    return "UnknownTrapType";
}

Trap::TrapType Trap::getTrapTypeFromMeshName(std::string s)
{
    if (s.compare("Cannon") == 0)
        return cannon;
    else
    {
        std::cerr
                << "\n\n\nERROR:  Trying to get trap type from unknown mesh name, bailing out.\n";
        std::cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
                << "\n\n\n";
        exit(1);
    }
}

int Trap::costPerTile(TrapType t)
{
    switch (t)
    {
        case nullTrapType:
            return 0;
            break;
        case cannon:
            return 500;
            break;
    }

    return 100;
}

bool Trap::doUpkeep()
{
    return doUpkeep(this);
}

bool Trap::doUpkeep(Trap *t)
{
    return true;
}

void Trap::addCoveredTile(Tile* t, double nHP)
{
    coveredTiles.push_back(t);
    tileHP[t] = nHP;
}

void Trap::removeCoveredTile(Tile* t)
{
    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        if (t == coveredTiles[i])
        {
            coveredTiles.erase(coveredTiles.begin() + i);
            tileHP.erase(t);
            break;
        }
    }

    /*
     // Destroy the mesh for this tile.
     RenderRequest *request = new RenderRequest;
     request->type = RenderRequest::destroyTrap;
     request->p = static_cast<void*>(this);
     request->p2 = t;

     // Add the request to the queue of rendering operations to be performed before the next frame.
     RenderManager::queueRenderRequest(request);
     */
}

Tile* Trap::getCoveredTile(int index)
{
    return coveredTiles[index];
}

std::vector<Tile*> Trap::getCoveredTiles()
{
    return coveredTiles;
}

unsigned int Trap::numCoveredTiles()
{
    return coveredTiles.size();
}

void Trap::clearCoveredTiles()
{
    coveredTiles.clear();
}

double Trap::getHP(Tile *tile)
{
    //NOTE: This function is the same as Room::getHP(), consider making a base class to inherit this from.
    if (tile != NULL)
    {
        return tileHP[tile];
    }
    else
    {
        // If the tile give was NULL, we add the total HP of all the tiles in the room and return that.
        double total = 0.0;
        std::map<Tile*, double>::iterator itr = tileHP.begin();
        while (itr != tileHP.end())
        {
            total += itr->second;
            ++itr;
        }

        return total;
    }
}

double Trap::getDefense() const
{
    return 0;
}

void Trap::takeDamage(double damage, Tile *tileTakingDamage)
{
    tileHP[tileTakingDamage] -= damage;
}

void Trap::recieveExp(double experience)
{
    exp += experience;
}

bool Trap::isMobile() const
{
    return false;
}

int Trap::getLevel() const
{
    return 1;
}

int Trap::getColor() const
{
    if (controllingSeat != NULL)
        return controllingSeat->color;
    else
        return 0;
}

AttackableObject::AttackableObjectType Trap::getAttackableObjectType() const
{
    return trap;
}

std::string Trap::getFormat()
{
    return "meshName\tcolor\t\tNextLine: numTiles\t\tSubsequent Lines: tileX\ttileY";
}

std::istream& operator>>(std::istream& is, Trap *t)
{
    static int uniqueNumber = 1;
    int tilesToLoad, tempX, tempY, tempInt;
    std::string tempString;
    std::stringstream tempSS;

    is >> t->meshName >> tempInt;
    t->controllingSeat = gameMap.getSeatByColor(tempInt);

    tempSS.str("");
    tempSS << t->meshName << "_" << uniqueNumber;
    ++uniqueNumber;
    t->name = tempSS.str();

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile *tempTile = gameMap.getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            t->addCoveredTile(tempTile);
            //FIXME: This next line will not be necessary when the the tile color is properly set by the tile load routine.
            tempTile->setColor(t->controllingSeat->color);
            tempTile->colorDouble = 1.0;
        }
    }

    t->type = Trap::getTrapTypeFromMeshName(t->meshName);
    return is;
}

std::ostream& operator<<(std::ostream& os, Trap *t)
{
    os << t->meshName << "\t" << t->controllingSeat->color << "\n";
    os << t->coveredTiles.size() << "\n";
    for (unsigned int i = 0; i < t->coveredTiles.size(); ++i)
    {
        Tile *tempTile = t->coveredTiles[i];
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }

    return os;
}

