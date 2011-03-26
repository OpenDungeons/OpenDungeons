#include <sstream>

#include "Globals.h"
#include "Functions.h"
#include "Room.h"
#include "Player.h"
#include "Tile.h"
#include "Creature.h"
#include "RenderRequest.h"
#include "GameMap.h"
#include "RoomObject.h"

const double Room::defaultTileHP = 10.0;

Room::Room()
{
    color = 0;
    controllingPlayer = NULL;
    meshExists = false;
}

/*! \brief Creates a type specific subclass of room (quarters, treasury, etc) and returns a pointer to it.  This function
 *  also initializes a unique default name for the room and sets up some of the room's properties.
 */
Room* Room::createRoom(RoomType nType, const std::vector<Tile*> &nCoveredTiles,
        int nColor)
{
    Room *tempRoom = NULL;

    switch (nType)
    {
        case nullRoomType:
            tempRoom = NULL;
            break;
        case quarters:
            tempRoom = new RoomQuarters();
            break;
        case treasury:
            tempRoom = new RoomTreasury();
            break;
        case portal:
            tempRoom = new RoomPortal();
            break;
        case dungeonTemple:
            tempRoom = new RoomDungeonTemple();
            break;
        case forge:
            tempRoom = new RoomForge();
            break;
        case dojo:
            tempRoom = new RoomDojo();
            break;
    }

    if (tempRoom == NULL)
    {
        std::cerr
                << "\n\n\nERROR: Trying to create a room of unknown type, bailing out.\n";
        std::cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
                << "\n\n\n";
        exit(1);
    }

    tempRoom->meshExists = false;
    tempRoom->color = nColor;

    //TODO: This should actually just call setType() but this will require a change to the >> operator.
    tempRoom->meshName = getMeshNameFromRoomType(nType);
    tempRoom->type = nType;

    static int uniqueNumber = -1;
    std::stringstream tempSS;

    tempSS.str("");
    tempSS << tempRoom->meshName << "_" << uniqueNumber;
    --uniqueNumber;
    tempRoom->name = tempSS.str();

    for (unsigned int i = 0; i < nCoveredTiles.size(); ++i)
        tempRoom->addCoveredTile(nCoveredTiles[i]);

    return tempRoom;
}

/*! \brief Moves all the covered tiles from room r into this one, the rooms should be of the same subtype.
 *  After this is called the other room should likely be removed from the game map and deleted.
 */
void Room::absorbRoom(Room *r)
{
    // Subclasses overriding this function can call this to do the generic stuff or they can reimplement it entirely.
    //TODO: This should probably just use an insert statement like the RoomOnjects below.
    while (r->numCoveredTiles() > 0)
    {
        Tile *tempTile = r->getCoveredTile(0);
        r->removeCoveredTile(tempTile);
        addCoveredTile(tempTile);
    }

    roomObjects.insert(r->roomObjects.begin(), r->roomObjects.end());
    r->roomObjects.clear();
}

Room* Room::createRoomFromStream(std::istream &is)
{
    Room tempRoom;
    is >> &tempRoom;

    Room *returnRoom = createRoom(tempRoom.type, tempRoom.coveredTiles,
            tempRoom.color);
    return returnRoom;
}

void Room::addCoveredTile(Tile* t, double nHP)
{
    coveredTiles.push_back(t);
    tileHP[t] = nHP;
    t->setCoveringRoom(this);
}

void Room::removeCoveredTile(Tile* t)
{
    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        if (t == coveredTiles[i])
        {
            coveredTiles.erase(coveredTiles.begin() + i);
            t->setCoveringRoom(NULL);
            tileHP.erase(t);
            break;
        }
    }

    // Destroy the mesh for this tile.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyRoom;
    request->p = this;
    request->p2 = t;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

Tile* Room::getCoveredTile(int index)
{
    return coveredTiles[index];
}

/** \brief Returns all of the tiles which are part of this room, this is to conform to the AttackableObject interface.
 *
 */
std::vector<Tile*> Room::getCoveredTiles()
{
    return coveredTiles;
}

unsigned int Room::numCoveredTiles()
{
    return coveredTiles.size();
}

void Room::clearCoveredTiles()
{
    coveredTiles.clear();
}

bool Room::tileIsPassable(Tile *t)
{
    return true;
}

void Room::addCreatureUsingRoom(Creature *c)
{
    //FIXME: When the room is destroyed, any creatures holding pointers to this room should be notified so they can purge them.  This is a somewhat non-trivial task.
    creaturesUsingRoom.push_back(c);
}

void Room::removeCreatureUsingRoom(Creature *c)
{
    for (unsigned int i = 0; i < creaturesUsingRoom.size(); ++i)
    {
        if (creaturesUsingRoom[i] == c)
        {
            creaturesUsingRoom.erase(creaturesUsingRoom.begin() + i);
            break;
        }
    }
}

Creature* Room::getCreatureUsingRoom(int index)
{
    return creaturesUsingRoom[index];
}

unsigned int Room::numCreaturesUsingRoom()
{
    return creaturesUsingRoom.size();
}

/** \brief Returns how many creatures could use this room for its intended purpose: negative numbers indicate there is no limit to the number of creatures.
 *
 */
int Room::numOpenCreatureSlots()
{
    return -1;
}

Tile* Room::getCentralTile()
{
    if (coveredTiles.size() == 0)
        return NULL;

    int minX, maxX, minY, maxY;
    minX = maxX = coveredTiles[0]->x;
    minY = maxY = coveredTiles[0]->y;

    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        int tempX = coveredTiles[i]->x;
        int tempY = coveredTiles[i]->y;

        if (tempX < minX)
            minX = tempX;
        if (tempY < minY)
            minY = tempY;
        if (tempX > maxX)
            maxX = tempX;
        if (tempY > maxY)
            maxY = tempY;
    }

    int centralX = (minX + maxX) / 2;
    int centralY = (minY + maxY) / 2;
    //TODO: If this central tile is NULL we should move outward until we find a valid one.
    Tile *centralTile = gameMap.getTile(centralX, centralY);

    return centralTile;
}

void Room::createMeshes()
{
    if (meshExists)
        return;

    meshExists = true;

    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        Tile *tempTile = coveredTiles[i];
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::createRoom;
        request->p = this;
        request->p2 = tempTile;

        // Add the request to the queue of rendering operations to be performed before the next frame.
        queueRenderRequest(request);
    }
}

void Room::destroyMeshes()
{
    if (!meshExists)
        return;

    meshExists = false;

    destroyRoomObjectMeshes();

    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        Tile *tempTile = coveredTiles[i];
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::destroyRoom;
        request->p = this;
        request->p2 = tempTile;

        // Add the request to the queue of rendering operations to be performed before the next frame.
        queueRenderRequest(request);
    }
}

/*! \brief Creates a child RoomObject mesh using the given mesh name and placing on the target tile, if the tile is NULL the object appears in the room's center, the rotation angle is given in degrees.
 *
 */
RoomObject* Room::loadRoomObject(std::string meshName, Tile *targetTile,
        double rotationAngle)
{
    if (targetTile == NULL)
        targetTile = getCentralTile();

    return loadRoomObject(meshName, targetTile, targetTile->x, targetTile->y,
            rotationAngle);
}

RoomObject* Room::loadRoomObject(std::string meshName, Tile *targetTile, double x,
        double y, double rotationAngle)
{
    RoomObject *tempRoomObject = new RoomObject(this, meshName);
    roomObjects[targetTile] = tempRoomObject;
    tempRoomObject->x = x;
    tempRoomObject->y = y;
    tempRoomObject->rotationAngle = rotationAngle;

    return tempRoomObject;
}

void Room::createRoomObjectMeshes()
{
    // Loop over all the RoomObjects that are children of this room and create each mesh individually.
    std::map<Tile*, RoomObject*>::iterator itr = roomObjects.begin();
    while (itr != roomObjects.end())
    {
        itr->second->createMesh();
        ++itr;
    }
}

void Room::destroyRoomObjectMeshes()
{
    // Loop over all the RoomObjects that are children of this room and destroy each mesh individually.
    std::map<Tile*, RoomObject*>::iterator itr = roomObjects.begin();
    while (itr != roomObjects.end())
    {
        itr->second->destroyMesh();
        ++itr;
    }
}

void Room::deleteYourself()
{
    destroyMeshes();

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::deleteRoom;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

std::string Room::getFormat()
{
    return "meshName\tcolor\t\tNextLine: numTiles\t\tSubsequent Lines: tileX\ttileY";
}

bool Room::doUpkeep()
{
    return doUpkeep(this);
}

/** \brief Carry out per turn upkeep on the room, the parameter r should be set to 'this' if called from a subclass to determine the room type.
 *
 */
bool Room::doUpkeep(Room *r)
{
    // Do any generic upkeep here (i.e. any upkeep that all room types should do).  All base classes of room should call this function during their doUpkeep() routine.

    // If r is non-null we use it to determine the type of the room (quarters, treasury, etc) of the room so we can call the room specific functions.
    if (r != NULL)
    {
        // Loop over the tiles in Room r and remove any whose HP has dropped to zero.
        unsigned int i = 0;
        while (i < r->coveredTiles.size())
        {
            if (r->tileHP[r->coveredTiles[i]] <= 0.0)
                r->removeCoveredTile(r->coveredTiles[i]);
            else
                ++i;
        }
    }

    //TODO: This could return false if the whole room has been destroyed and then the activeObject processor should destroy it.
    return true;
}

std::istream& operator>>(std::istream& is, Room *r)
{
    static int uniqueNumber = 1;
    int tilesToLoad, tempX, tempY;
    std::string tempString;
    std::stringstream tempSS;

    is >> r->meshName >> r->color;

    tempSS.str("");
    tempSS << r->meshName << "_" << uniqueNumber;
    uniqueNumber++;
    r->name = tempSS.str();

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile *tempTile = gameMap.getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            r->addCoveredTile(tempTile);

            //FIXME: This next line will not be necessary when the the tile color is properly set by the tile load routine.
            tempTile->setColor(r->color);
            tempTile->colorDouble = 1.0;
        }
    }

    r->type = Room::getRoomTypeFromMeshName(r->meshName);
    return is;
}

std::ostream& operator<<(std::ostream& os, Room *r)
{
    os << r->meshName << "\t" << r->color << "\n";
    os << r->coveredTiles.size() << "\n";
    for (unsigned int i = 0; i < r->coveredTiles.size(); ++i)
    {
        Tile *tempTile = r->coveredTiles[i];
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }

    return os;
}

std::string Room::getMeshNameFromRoomType(RoomType t)
{
    switch (t)
    {
        case nullRoomType:
            return "NullRoomType";
            break;
        case dungeonTemple:
            return "DungeonTemple";
            break;
        case quarters:
            return "Quarters";
            break;
        case treasury:
            return "Treasury";
            break;
        case portal:
            return "Portal";
            break;
        case forge:
            return "Forge";
            break;
        case dojo:
            return "Dojo";
            break;
    }

    return "UnknownRoomType";
}

Room::RoomType Room::getRoomTypeFromMeshName(std::string s)
{
    if (s.compare("DungeonTemple") == 0)
        return dungeonTemple;
    else if (s.compare("Quarters") == 0)
        return quarters;
    else if (s.compare("Treasury") == 0)
        return treasury;
    else if (s.compare("Portal") == 0)
        return portal;
    else if (s.compare("Forge") == 0)
        return forge;
    else if (s.compare("Dojo") == 0)
        return dojo;
    else
    {
        std::cerr
                << "\n\n\nERROR:  Trying to get room type from unknown mesh name, bailing out.\n";
        std::cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
                << "\n\n\n";
        exit(1);
    }
}

int Room::costPerTile(RoomType t)
{
    switch (t)
    {
        case nullRoomType:
            return 0;
            break;
        case dungeonTemple:
            return 0;
            break;
        case portal:
            return 0;
            break;
        case quarters:
            return 75;
            break;
        case treasury:
            return 25;
            break;
        case forge:
            return 150;
            break;
        case dojo:
            return 175;
            break;
    }

    return 0;
}

double Room::getHP(Tile *tile)
{
    //NOTE: This function is the same as Trap::getHP(), consider making a base class to inherit this from.
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

double Room::getDefense() const
{
    return 0.0;
}

void Room::takeDamage(double damage, Tile *tileTakingDamage)
{
    tileHP[tileTakingDamage] -= damage;
}

void Room::recieveExp(double experience)
{
    // Do nothing since Rooms do not have exp.
}

bool Room::isMobile() const
{
    return false;
}

int Room::getLevel() const
{
    // Since rooms do not have exp or level we just consider them level 1 for compatibility with the AttackableObject interface.
    return 1;
}

int Room::getColor() const
{
    return color;
}

AttackableObject::AttackableObjectType Room::getAttackableObjectType() const
{
    return AttackableObject::room;
}

