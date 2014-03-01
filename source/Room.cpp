#include <sstream>

#include "Player.h"
#include "Tile.h"
#include "Creature.h"
#include "RenderRequest.h"
#include "GameMap.h"
#include "RoomObject.h"
#include "RenderManager.h"

#include "Room.h"
#include "Seat.h"

const double Room::defaultTileHP = 10.0;

Room::Room() :
        type    (nullRoomType)
{
    setObjectType(GameEntity::room);
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
                << "\n\n\nERROR: Trying to create a room of unknown type, bailing out.\n"
                << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
                << "\n\n\n";
        exit(1);
    }

    tempRoom->setMeshExisting(false);
    tempRoom->setColor(nColor);

    //TODO: This should actually just call setType() but this will require a change to the >> operator.
    tempRoom->setMeshName(getMeshNameFromRoomType(nType));
    tempRoom->type = nType;

    static int uniqueNumber = 0;
    std::stringstream tempSS;

    tempSS.str("");
    tempSS << tempRoom->getMeshName() << "_" << --uniqueNumber;
    tempRoom->setName(tempSS.str());

    for (unsigned int i = 0; i < nCoveredTiles.size(); ++i)
        tempRoom->addCoveredTile(nCoveredTiles[i]);

    return tempRoom;
}

/** \brief Builds a room for the current player.
 *  Builds a room for the current player. Checks if the player has enough gold,
 *  if not, NULL is returned.
 *  \return The room built, or NULL if the player does not have enough gold.
 */
Room* Room::buildRoom(GameMap* gameMap, Room::RoomType nType, const std::vector< Tile* >& coveredTiles, Player* player, bool inEditor)
{
    int goldRequired = coveredTiles.size() * Room::costPerTile(
                            nType);
    Room* newRoom = NULL;
    if(player->getSeat()->getGold() > goldRequired || inEditor)
    {
        newRoom = createRoom(nType, coveredTiles, player->getSeat()->getColor());
            gameMap->addRoom(newRoom);
        if(!inEditor)
        {
            gameMap->withdrawFromTreasuries(goldRequired, player->getSeat()->getColor());
        }

        // Check all the tiles that border the newly created room and see if they
        // contain rooms which can be absorbed into this newly created room.
        std::vector<Tile*> borderTiles =
                gameMap->tilesBorderedByRegion(coveredTiles);
        for (unsigned int i = 0; i < borderTiles.size(); ++i)
        {
            Room *borderingRoom = borderTiles[i]->getCoveringRoom();
            if (borderingRoom != NULL && borderingRoom->getType()
                    == newRoom->getType() && borderingRoom
                    != newRoom)
            {
                newRoom->absorbRoom(borderingRoom);
                gameMap->removeRoom(borderingRoom);
                //FIXME:  Need to delete the bordering room to avoid a memory leak, the deletion should be done in a safe way though as there will still be outstanding RenderRequests.
            }
        }

        newRoom->createMesh();

        SoundEffectsHelper::getSingleton().playInterfaceSound(
                SoundEffectsHelper::BUILDROOM, false);
    }
    return newRoom;
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

Room* Room::createRoomFromStream(std::istream &is, GameMap* gameMap)
{
    Room tempRoom;
    tempRoom.setGameMap(gameMap);
    is >> &tempRoom;

    return createRoom(tempRoom.type, tempRoom.coveredTiles, tempRoom.getColor());
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
    RenderManager::queueRenderRequest(request);
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
    if (coveredTiles.empty())
    {
        return NULL;
    }

    int minX, maxX, minY, maxY;
    minX = maxX = coveredTiles[0]->getX();
    minY = maxY = coveredTiles[0]->getY();

    for(unsigned int i = 0, size = coveredTiles.size(); i < size; ++i)
    {
        int tempX = coveredTiles[i]->getX();
        int tempY = coveredTiles[i]->getY();

        if (tempX < minX)
            minX = tempX;
        if (tempY < minY)
            minY = tempY;
        if (tempX > maxX)
            maxX = tempX;
        if (tempY > maxY)
            maxY = tempY;
    }

    //TODO: If this tile is NULL we should move outward until we find a valid one.
    return getGameMap()->getTile((minX + maxX) / 2, (minY + maxY) / 2);
}

const Tile* Room::getCentralTile() const
{
    if (coveredTiles.empty())
    {
        return NULL;
    }

    int minX, maxX, minY, maxY;
    minX = maxX = coveredTiles[0]->x;
    minY = maxY = coveredTiles[0]->y;

    for(unsigned int i = 0, size = coveredTiles.size(); i < size; ++i)
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

    //TODO: If this tile is NULL we should move outward until we find a valid one.
    return getGameMap()->getTile((minX + maxX) / 2, (minY + maxY) / 2);
}

/*! \brief Creates a child RoomObject mesh using the given mesh name and placing on the target tile, if the tile is NULL the object appears in the room's center, the rotation angle is given in degrees.
 *
 */
RoomObject* Room::loadRoomObject(std::string meshName, Tile *targetTile,
        double rotationAngle)
{
    // TODO - proper random distrubition of room objects
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
    tempRoomObject->x = (Ogre::Real)x;
    tempRoomObject->y = (Ogre::Real)y;
    tempRoomObject->rotationAngle = (Ogre::Real)rotationAngle;

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
    static int uniqueNumber = 0;
    int tilesToLoad, tempX, tempY;
    std::stringstream tempSS;

    std::string tempString;
    is >> tempString;
    r->setMeshName(tempString);

    int tempInt = 0;
    is >> tempInt;
    r->setColor(tempInt);

    tempSS.str("");
    tempSS << r->getMeshName() << "_" << ++uniqueNumber;
    r->setName(tempSS.str());

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
	tempX+= r->getGameMap()->getMapSizeX()/2 ;
	tempY+= r->getGameMap()->getMapSizeY()/2 ;
        Tile *tempTile = r->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            r->addCoveredTile(tempTile);

            //FIXME: This next line will not be necessary when the the tile color is properly set by the tile load routine.
            tempTile->setColor(r->getColor());
            tempTile->colorDouble = 1.0;
        }
    }

    r->type = Room::getRoomTypeFromMeshName(r->getMeshName());
    return is;
}

std::ostream& operator<<(std::ostream& os, Room *r)
{
    os << r->getMeshName() << "\t" << r->getColor() << "\n";
    os << r->coveredTiles.size() << "\n";
    for (unsigned int i = 0; i < r->coveredTiles.size(); ++i)
    {
        Tile *tempTile = r->coveredTiles[i];
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }

    return os;
}

const char* Room::getMeshNameFromRoomType(RoomType t)
{
    switch (t)
    {
        case nullRoomType:
            return "NullRoomType";

        case dungeonTemple:
            return "DungeonTemple";

        case quarters:
            return "Quarters";

        case treasury:
            return "Treasury";

        case portal:
            return "Portal";

        case forge:
            return "Forge";

        case dojo:
            return "Dojo";

        default:
            return "UnknownRoomType";
    }
}

Room::RoomType Room::getRoomTypeFromMeshName(const std::string& s)
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

        case dungeonTemple:
            return 0;

        case portal:
            return 0;

        case quarters:
            return 75;

        case treasury:
            return 25;

        case forge:
            return 150;

        case dojo:
            return 175;

        default:
            return 0;
    }
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

        for(std::map<Tile*, double>::iterator itr = tileHP.begin(), end = tileHP.end();
                itr != end; ++itr)
        {
            total += itr->second;
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
