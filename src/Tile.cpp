#include "Globals.h"
#include "Functions.h"
#include "Creature.h"
#include "GameMap.h"
#include "ServerNotification.h"
#include "RenderRequest.h"
#include "MapLight.h"
#include "Seat.h"
#include "SoundEffectsHelper.h"
#include "RenderManager.h"

#include "Tile.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf _snprintf
#endif

//FIXME:  this extern is probably not needed once the rendering code is all in one thread.
//FIXME: This should not be here. Use parameters for whatever needs the scenemanager instead.
extern Ogre::SceneManager* mSceneMgr;

void Tile::initialize()
{
    sem_init(&creaturesInCellLockSemaphore, 0, 1);
    sem_init(&fullnessLockSemaphore, 0, 1);
    sem_init(&coveringRoomLockSemaphore, 0, 1);
    sem_init(&neighborsLockSemaphore, 0, 1);
    sem_init(&claimLightLockSemaphore, 0, 1);

    selected = false;
    markedForDigging = false;
    //location = Ogre::Vector3(0.0, 0.0, 0.0);
    type = dirt;
    setFullness(100.0);
    rotation = 0.0;
    color = 0;
    colorDouble = 0.0;
    floodFillColor = -1;
    sem_wait(&coveringRoomLockSemaphore);
    coveringRoom = NULL;
    sem_post(&coveringRoomLockSemaphore);
    coveringTrap = false;

    sem_wait(&claimLightLockSemaphore);
    claimLight = NULL;
    sem_post(&claimLightLockSemaphore);

    meshesExist = false;
}

Tile::Tile()
{
    initialize();
}

Tile::Tile(int nX, int nY, TileType nType, double nFullness)
{
    initialize();

    x = nX;
    y = nY;
    setType(nType);
    setFullness(nFullness);
}

/*! \brief A mutator to set the type (rock, claimed, etc.) of the tile.
 *
 * In addition to setting the tile type this function also reloads the new mesh
 * for the tile.
 */
void Tile::setType(TileType t)
{
    // If the type has changed from its previous value we need to see if
    // the mesh should be updated
    if (t != type)
    {
        type = t;
        refreshMesh();
    }
}

/*! \brief An accessor which returns the tile type (rock, claimed, etc.).
 *
 */
Tile::TileType Tile::getType()
{
    return type;
}

/*! \brief A mutator to change how "filled in" the tile is.
 *
 * Additionally this function reloads the proper mesh to display to the user
 * how full the tile is.  It also determines the orientation of the
 * tile to make corners display correctly.  Both of these tasks are
 * accomplished by setting the fullnessMeshNumber variable which is
 * concatenated to the tile's type to determine the mesh to load, e.g.
 * Rock104.mesh for a rocky tile which has all 4 sides shown because it is an
 * "island" with all four sides visible.  Claimed102.mesh would be a fully
 * filled in tile but only two sides are drawn because it borders full tiles on
 * 2 sides.
 */
void Tile::setFullness(double f)
{
    int oldFullnessMeshNumber = fullnessMeshNumber;
    TileClearType oldTilePassability = getTilePassability();

    sem_wait(&fullnessLockSemaphore);
    fullness = f;

    // If the tile was marked for digging and has been dug out, unmark it and set its fullness to 0.
    if (fullness < 1 && getMarkedForDigging(gameMap.me) == true)
    {
        setMarkedForDiggingForAllSeats(false);
        fullness = 0.0;

        //Play block destroy sound
        //TODO - play this if it's marked for any player.
        SoundEffectsHelper::getSingleton().playBlockDestroySound(x, y);
    }

    /*
     if(fullness < 1 && gameMap.)
     {
     SoundEffectsHelper::getSingleton().playBlockDestroySound(x, y);
     }
     */

    sem_post(&fullnessLockSemaphore);

    // If we are a sever, the clients need to be told about the change to the tile's fullness.
    if (serverSocket != NULL)
    {
        try
        {
            // Inform the clients that the fullness has changed.
            ServerNotification *serverNotification = new ServerNotification;
            serverNotification->type = ServerNotification::tileFullnessChange;
            serverNotification->tile = this;

            queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            std::cerr << "\n\nERROR:  bad alloc in Tile::setFullness\n\n";
            exit(1);
        }
    }

    // If the passability has changed we may have opened up new paths on the gameMap.
    if (oldTilePassability != getTilePassability())
    {
        // Do a flood fill to update the contiguous region touching the tile.
        gameMap.doFloodFill(x, y);
    }

    // 		4 0 7		    180
    // 		2 8 3		270  .  90
    // 		7 1 5		     0
    //
    bool fillStatus[9];
    Tile *tempTile = gameMap.getTile(x, y + 1);
    fillStatus[0] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
    tempTile = gameMap.getTile(x, y - 1);
    fillStatus[1] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
    tempTile = gameMap.getTile(x - 1, y);
    fillStatus[2] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
    tempTile = gameMap.getTile(x + 1, y);
    fillStatus[3] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;

    int fullNeighbors = 0;
    if (fillStatus[0])
        ++fullNeighbors;
    if (fillStatus[1])
        ++fullNeighbors;
    if (fillStatus[2])
        ++fullNeighbors;
    if (fillStatus[3])
        ++fullNeighbors;

    //FIXME:  This needs to be updated to reflect the allowable fill states for each tile type
    // This is also where the logic for checking neighboring fullness should go
    fullnessMeshNumber = 0;
    if (f > 0 && f <= 25)
        fullnessMeshNumber = 25;
    else if (f > 25 && f <= 50)
    {
        fullnessMeshNumber = 50;
        switch (fullNeighbors)
        {
            case 1:
                fullnessMeshNumber = 51;
                if (fillStatus[0])
                {
                    rotation = 270;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    rotation = 90;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    rotation = 180;
                    break;
                }//correct
                break;

            case 2:
                fullnessMeshNumber = 52;
                if (fillStatus[0] && fillStatus[2])
                {
                    rotation = 270;
                    break;
                }//correct
                if (fillStatus[0] && fillStatus[3])
                {
                    rotation = 180;
                    break;
                }//correct
                if (fillStatus[1] && fillStatus[2])
                {
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[1] && fillStatus[3])
                {
                    rotation = 90;
                    break;
                }//correct

                //TODO:  These next two options are for when the half full tile is in the middle of a wall, the need a separate mesh to be made.
                if (fillStatus[0] && fillStatus[1])
                {
                    fullnessMeshNumber = 51;
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[2] && fillStatus[3])
                {
                    fullnessMeshNumber = 51;
                    rotation = 90;
                    break;
                }//correct
                break;
        }
    }

    else if (f > 50 && f <= 75)
    {
        fullnessMeshNumber = 75;
        switch (fullNeighbors)
        {
            case 1:
                if (fillStatus[0])
                {
                    rotation = 270;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    rotation = 90;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    rotation = 180;
                    break;
                }//correct
                break;
        }
    }
    else if (f > 75)
    {
        switch (fullNeighbors)
        {
            //TODO:  Determine the rotation for each of these case statements
            int tempInt;
            case 0:
                fullnessMeshNumber = 104;
                rotation = 0;
                break;

            case 1:
                fullnessMeshNumber = 103;
                if (fillStatus[0])
                {
                    rotation = 180;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    rotation = 270;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    rotation = 90;
                    break;
                }//correct
                break;

            case 2:
                tempInt = 0;
                if (fillStatus[0])
                    tempInt += 1;
                if (fillStatus[1])
                    tempInt += 2;
                if (fillStatus[2])
                    tempInt += 4;
                if (fillStatus[3])
                    tempInt += 8;

                switch (tempInt)
                {
                    case 5:
                        fullnessMeshNumber = 52;
                        rotation = 270;
                        break;

                    case 6:
                        fullnessMeshNumber = 52;
                        rotation = 0;
                        break;

                    case 9:
                        fullnessMeshNumber = 52;
                        rotation = 180;
                        break;

                    case 10:
                        fullnessMeshNumber = 52;
                        rotation = 90;
                        break;

                    case 3:
                        fullnessMeshNumber = 102;
                        rotation = 0.0;
                        break;

                    case 12:
                        fullnessMeshNumber = 102;
                        rotation = 90.0;
                        break;

                    default:
                        std::cerr
                                << "\n\nERROR:  Unhandled case statement in Tile::setFullness(), exiting.  tempInt = "
                                << tempInt << "\n\n";
                        exit(1);
                        break;
                }
                break;

            case 3:
                fullnessMeshNumber = 101; //this is wrong for now it should be 101
                if (!fillStatus[0])
                {
                    rotation = 90;
                    break;
                }//correct
                if (!fillStatus[1])
                {
                    rotation = 270;
                    break;
                }
                if (!fillStatus[2])
                {
                    rotation = 180;
                    break;
                }
                if (!fillStatus[3])
                {
                    rotation = 0;
                    break;
                }
                break;

            case 4:
                fullnessMeshNumber = 100;
                rotation = 0;
                break;

            default:
                std::cerr
                        << "\n\nERROR:  fullNeighbors != 0 or 1 or 2 or 3 or 4.  This is impossible, exiting program.\n\n";
                exit(1);
                break;
        }

    }

    // If the mesh has changed it means that a new path may have opened up.
    if (oldFullnessMeshNumber != fullnessMeshNumber)
    {
        refreshMesh();
    }
}

/*! \brief An accessor which returns the tile's fullness which should range from 0 to 100.
 *
 */
double Tile::getFullness()
{
    sem_wait(&fullnessLockSemaphore);
    double tempDouble = fullness;
    sem_post(&fullnessLockSemaphore);

    return tempDouble;
}

/*! \brief An accessor which returns the tile's fullness mesh number.
 *
 * The fullness mesh number is concatenated to the tile's type to determine the
 * mesh to load to display a given tile type.
 */
int Tile::getFullnessMeshNumber()
{
    return fullnessMeshNumber;
}

/*! \brief Returns the 'passability' state of a tile (impassableTile, walkableTile, etc.).
 *
 * The passability of a tile indicates what type of creatures may move into the
 * given tile.  As an example, no creatures may move into an 'impassableTile'
 * like Dirt100 and only flying creatures may move into a 'flyableTile' like
 * Lava0.
 */
Tile::TileClearType Tile::getTilePassability()
{
    // Check to see if the tile is filled in.
    if (getFullness() >= 1)
        return impassableTile;

    //Check to see if there is a room with objects covering this tile preventing creatures from walking through it.
    //FIXME: The second portion of this if statement throws a segfault.  Something is incorrectly setting the coveringRoom.
    //NOTE: If this code is turned back on the coveringRoom variable is protected by a LockSemaphore.
    //if(coveringRoom != NULL)// && !coveringRoom->tileIsPassable(this))
    //return impassableTile;

    switch (type)
    {
        case dirt:
        case gold:
        case rock:
        case claimed:
            return walkableTile;
            break;

        case water:
            return walkableTile;
            break;

        case lava:
            return flyableTile;
            break;

        default:
            std::cerr
                    << "\n\nERROR:  Unhandled tile type in Tile::getTilePassability()\n\n";
            exit(1);
            break;
    }

    // Return something to make the compiler happy.
    // Control should really never reach here because of the exit(1) call in the default switch case above
    std::cerr
            << "\n\nERROR:  Control reached the end of Tile::getTilePassability, this should never actually happen.\n\n";
    exit(1);
    return impassableTile;
}

bool Tile::permitsVision()
{
    //TODO: This call to getTilePassability() is far too much work, when the rules for vision are more well established this function should be replaced with specialized code which avoids this call.
    TileClearType clearType = getTilePassability();
    return (clearType == walkableTile || clearType == flyableTile)
            ? true : false;
}

/* Checks if the place is buildable at the moment */
bool Tile::isBuildableUpon()
{
    sem_wait(&coveringRoomLockSemaphore);
    Room *ret = coveringRoom;
    sem_post(&coveringRoomLockSemaphore);

    return (ret==NULL && coveringTrap==false);
}

bool Tile::getCoveringTrap()
{
    return coveringTrap;
}

void Tile::setCoveringTrap(bool t)
{
    coveringTrap = t;
}

Room* Tile::getCoveringRoom()
{
    sem_wait(&coveringRoomLockSemaphore);
    Room *ret = coveringRoom;
    sem_post(&coveringRoomLockSemaphore);

    return ret;
}

void Tile::setCoveringRoom(Room *r)
{
    sem_wait(&coveringRoomLockSemaphore);
    coveringRoom = r;
    sem_post(&coveringRoomLockSemaphore);
}

/*! \brief Check if tile is diggable.
 *  Returns true if a tile is diggable.
 *  (Can be marked for digging and dug out by kobolds)
 */
bool Tile::isDiggable()
{
    return ((type == dirt || type == gold || type == claimed) && getFullness() > 1)
                ? true : false;
}

bool Tile::isClaimable()
{
    return ((type == dirt || type == claimed) && getFullness() < 1);
}

std::string Tile::getFormat()
{
    return "posX\tposY\ttype\tfullness";
}

/*! \brief The << operator is used for saving tiles to a file and sending them over the net.
 *
 * This operator is used in conjunction with the >> operator to standardize
 * tile format in the level files, as well as sending tiles over the network.
 */
std::ostream& operator<<(std::ostream& os, Tile *t)
{
    os << t->x << "\t" << t->y << "\t" << t->getType() << "\t"
            << t->getFullness();

    return os;
}

/*! \brief The >> operator is used for loading tiles from a file and for receiving them over the net.
 *
 * This operator is used in conjunction with the << operator to standardize
 * tile format in the level files, as well as sending tiles over the network.
 */
std::istream& operator>>(std::istream& is, Tile *t)
{
    int tempInt, xLocation, yLocation;
    double tempDouble;
    char tempCellName[255];

    is >> xLocation >> yLocation;
    //t->location = Ogre::Vector3(xLocation, yLocation, 0);
    snprintf(tempCellName, sizeof(tempCellName), "Level_%3i_%3i", xLocation,
            yLocation);
    t->name = tempCellName;
    t->x = xLocation;
    t->y = yLocation;

    is >> tempInt;
    t->setType((Tile::TileType) tempInt);

    is >> tempDouble;
    t->setFullness(tempDouble);

    return is;
}

/*! \brief This is a helper function which just converts the tile type enum into a string.
 *
 * This function is used primarily in forming the mesh names to load from disk
 * for the various tile types.  The name returned by this function is
 * concatenated with a fullnessMeshNumber to form the filename, e.g.
 * Dirt104.mesh is a 4 sided dirt mesh with 100% fullness.
 */
const char* Tile::tileTypeToString(TileType t)
{
    switch (t)
    {
        default:
        case dirt:
            return "Dirt";

        case rock:
            return "Rock";

        case gold:
            return "Gold";

        case water:
            return "Water";

        case lava:
            return "Lava";

        case claimed:
            return "Claimed";
    }
}

/*! \brief This is a helper function to scroll through the list of available tile types.
 *
 * This function is used in the map editor when the user presses the button to
 * select the next tile type to be active in the user interface.  The active
 * tile type is the one which is placed when the user clicks the mouse button.
 */
Tile::TileType Tile::nextTileType(TileType t)
{
    return static_cast<TileType>((static_cast<int>(t) + 1)
            % static_cast<int>(nullTileType));
}

/*! \brief This is a helper function to scroll through the list of available fullness levels.
 *
 * This function is used in the map editor when the user presses the button to
 * select the next tile fullness level to be active in the user interface.  The
 * active fullness level is the one which is placed when the user clicks the
 * mouse button.
 */
int Tile::nextTileFullness(int f)
{

    // Cycle the tile's fullness through the possible values
    switch (f)
    {
        case 0:
            return 25;

        case 25:
            return 50;

        case 50:
            return 75;

        case 75:
            return 100;

        case 100:
            return 0;

        default:
            return 0;
    }
}

/*! \brief This is a helper function that generates a mesh filename from a tile type and a fullness mesh number.
 *
 */
std::string Tile::meshNameFromFullness(TileType t, float fullnessMeshNumber)
{
    std::stringstream ss;
    //FIXME - define postfix somewhere
    ss << tileTypeToString(t) << fullnessMeshNumber << ".mesh";
    return ss.str();
}

/*! \brief This function puts a message in the renderQueue to change the mesh for this tile.
 *
 */
void Tile::refreshMesh()
{
    if (!meshesExist)
        return;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::refreshTile;
    request->p = static_cast<void*>(this);

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

/*! \brief This function puts a message in the renderQueue to load the mesh for this tile.
 *
 */
void Tile::createMesh()
{
    if (meshesExist)
        return;

    meshesExist = true;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::createTile;
    request->p = static_cast<void*>(this);

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

/*! \brief This function puts a message in the renderQueue to unload the mesh for this tile.
 *
 */
void Tile::destroyMesh()
{
    if (!meshesExist)
        return;

    meshesExist = false;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyTile;
    request->p = static_cast<void*>(this);

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

/*! \brief This function marks the tile as being selected through a mouse click or drag.
 *
 */
void Tile::setSelected(bool s)
{
    if (selected != s)
    {
        Ogre::Entity* ent;
        char tempString[255];

        snprintf(tempString, sizeof(tempString),
                "Level_%3i_%3i_selection_indicator", x, y);
        if (mSceneMgr->hasEntity(tempString))
        {
            ent = mSceneMgr->getEntity(tempString);
        }
        else
        {
            char tempString2[255];
            snprintf(tempString2, sizeof(tempString2), "Level_%3i_%3i_node", x, y);
            ent = mSceneMgr->createEntity(tempString, "SquareSelector.mesh");
            mSceneMgr->getSceneNode(tempString2)->attachObject(ent);
        }

        selected = s;
        ent->setVisible(selected);
    }
}

/*! \brief This accessor function returns whether or not the tile has been selected.
 *
 */
bool Tile::getSelected()
{
    return selected;
}

/*! \brief This function marks the tile to be dug out by workers, and displays the dig indicator on it.
 *
 */
void Tile::setMarkedForDigging(bool s, Player *p)
{
    /* If we are trying to mark a tile that is not dirt or gold
     * or is already dug out, ignore the request.
     */
    if (s && (!isDiggable() || (getFullness() < 1)))
        return;

    Ogre::Entity *ent;
    char tempString[255];
    char tempString2[255];

    if (getMarkedForDigging(p) != s)
    {
        bool thisRequestIsForMe = (p == gameMap.me);
        if (thisRequestIsForMe)
        {
            //FIXME:  This code should be moved over to the rendering thread and called via a RenderRequest
            snprintf(tempString, sizeof(tempString),
                    "Level_%i_%i_selection_indicator", x, y);
            if (mSceneMgr->hasEntity(tempString))
            {
                ent = mSceneMgr->getEntity(tempString);
            }
            else
            {
                snprintf(tempString2, sizeof(tempString2),
                        "Level_%3i_%3i_node", x, y);
                Ogre::SceneNode *tempNode =
                        mSceneMgr->getSceneNode(tempString2);

                ent = mSceneMgr->createEntity(tempString, "DigSelector.mesh");
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
                ent->setNormaliseNormals(true);
#endif
                tempNode->attachObject(ent);
            }
        }

        if (s)
        {
            //FIXME:  This code should be moved over to the rendering thread and called via a RenderRequest
            if (thisRequestIsForMe)
            {
                ent->setVisible(true);
            }
            addPlayerMarkingTile(p);
        }
        else
        {
            //FIXME:  This code should be moved over to the rendering thread and called via a RenderRequest
            if (thisRequestIsForMe)
            {
                ent->setVisible(false);
            }
            removePlayerMarkingTile(p);
        }
    }
}

/*! \brief This is a simple helper function which just calls setMarkedForDigging() for everyone in the game (including me).
 *
 */
void Tile::setMarkedForDiggingForAllSeats(bool s)
{
    setMarkedForDigging(s, gameMap.me);

    for (unsigned int i = 0, num = gameMap.numPlayers(); i < num; ++i)
        setMarkedForDigging(s, gameMap.getPlayer(i));
}

/*! \brief This accessor function returns whether or not the tile has been marked to be dug out by a given Player p.
 *
 */
bool Tile::getMarkedForDigging(Player *p)
{
    // Loop over any players who have marked this tile and see if 'p' is one of them
    for (unsigned int i = 0, size = playersMarkingTile.size(); i < size; ++i)
    {
        if (playersMarkingTile[i] == p)
        {
            return true;
        }
    }

    return false;
}

/*! \brief This function places a message in the render queue to unload the mesh and delete the tile structure.
 *
 */
void Tile::deleteYourself()
{
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyTile;
    request->p = static_cast<void*>(this);

    RenderRequest *request2 = new RenderRequest;
    request2->type = RenderRequest::deleteTile;
    request2->p = static_cast<void*>(this);

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
    RenderManager::queueRenderRequest(request2);
}

/*! \brief This function adds a creature to the list of creatures in this tile.
 *
 */
void Tile::addCreature(Creature *c)
{
    sem_wait(&creaturesInCellLockSemaphore);
    creaturesInCell.push_back(c);
    sem_post(&creaturesInCellLockSemaphore);
}

/*! \brief This function removes a creature to the list of creatures in this tile.
 *
 */
void Tile::removeCreature(Creature *c)
{
    sem_wait(&creaturesInCellLockSemaphore);

    // Check to see if the given crature is actually in this tile
    std::vector<Creature*>::iterator itr;
    for (itr = creaturesInCell.begin(); itr != creaturesInCell.end(); ++itr)
    {
        if ((*itr) == c)
        {
            // Remove the creature from the list
            creaturesInCell.erase(itr);
            break;
        }
    }

    sem_post(&creaturesInCellLockSemaphore);
}

/*! \brief This function returns the count of the number of creatures in the tile.
 *
 */
unsigned int Tile::numCreaturesInCell()
{
    sem_wait(&creaturesInCellLockSemaphore);
    unsigned int tempUnsigned = creaturesInCell.size();
    sem_post(&creaturesInCellLockSemaphore);

    return tempUnsigned;
}

/*! \brief This function returns the i'th creature in the tile.
 *
 */
Creature* Tile::getCreature(int index)
{
    sem_wait(&creaturesInCellLockSemaphore);
    Creature *tempCreature = creaturesInCell[index];
    sem_post(&creaturesInCellLockSemaphore);

    return tempCreature;
}

/*! \brief Add a player to the vector of players who have marked this tile for digging.
 *
 */
void Tile::addPlayerMarkingTile(Player *p)
{
    playersMarkingTile.push_back(p);
}

void Tile::removePlayerMarkingTile(Player *p)
{
    for (unsigned int i = 0; i < playersMarkingTile.size(); ++i)
    {
        if (p == playersMarkingTile[i])
        {
            playersMarkingTile.erase(playersMarkingTile.begin() + i);
            return;
        }
    }
}

unsigned int Tile::numPlayersMarkingTile()
{
    return playersMarkingTile.size();
}

Player* Tile::getPlayerMarkingTile(int index)
{
    return playersMarkingTile[index];
}

void Tile::addNeighbor(Tile *n)
{
    sem_wait(&neighborsLockSemaphore);
    neighbors.push_back(n);
    sem_post(&neighborsLockSemaphore);
}

double Tile::claimForColor(int nColor, double nDanceRate)
{
    Tile *tempTile;
    double amountClaimed;

    if (!isClaimable())
        return 0.0;

    // If the color is the same as ours we add to it, if it is an enemy color we subtract from it.
    if (nColor == color)
    {
        amountClaimed = std::min(nDanceRate, 1.0 - colorDouble);
        //cout << "\t\tmyTile is My color.";
        colorDouble += nDanceRate;
        if (colorDouble >= 1.0)
        {
            // Claim the tile.
            colorDouble = 1.0;
            setType(Tile::claimed);
            refreshMesh();
        }
    }
    else
    {
        amountClaimed = std::min(nDanceRate, 1.0 + colorDouble);
        colorDouble -= nDanceRate;
        if (colorDouble <= 0.0)
        {
            // The tile is not yet claimed, but it is now our color.
            colorDouble *= -1.0;
            color = nColor;

            if (colorDouble >= 1.0)
            {
                colorDouble = 1.0;
                refreshMesh();
            }
        }
    }

    // If this is the first time this tile has been claimed, emit a flash of light indicating that the tile was claimed.
    sem_wait(&claimLightLockSemaphore);
    if (amountClaimed > 0.0 && claimLight == NULL)
    {
        Ogre::ColourValue tempColour =
                gameMap.getSeatByColor(nColor)->colourValue;
        claimLight = new TemporaryMapLight(Ogre::Vector3(x, y, 0.5),
                tempColour.r, tempColour.g, tempColour.b, 1.0, 0.1, 0.5, 0.5);
        gameMap.addMapLight(claimLight);
        claimLight->createOgreEntity();
        SoundEffectsHelper::getSingleton().playInterfaceSound(
                SoundEffectsHelper::CLAIM, this->x, this->y);
    }
    sem_post(&claimLightLockSemaphore);

    // If there is still some left to claim.
    if (amountClaimed > 0.0 && amountClaimed < nDanceRate)
    {
        double amountToClaim = nDanceRate - amountClaimed;
        if (amountToClaim < 0.05)
            return amountClaimed;

        // Distribute the remaining amount left to claim out amongst the neighbor tiles.
        sem_wait(&neighborsLockSemaphore);
        amountToClaim /= (double) neighbors.size();
        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            if (neighbors[j]->getType() == dirt || neighbors[j]->getType()
                    == claimed)
            {
                tempTile = neighbors[j];
                // Release and relock the semaphore since the claimForColor() routine will eventually need to lock it.
                sem_post(&neighborsLockSemaphore);
                amountClaimed += tempTile->claimForColor(color, amountToClaim);
                sem_wait(&neighborsLockSemaphore);
            }
        }
        sem_post(&neighborsLockSemaphore);
    }

    return amountClaimed;
}

double Tile::digOut(double digRate, bool doScaleDigRate)
{
    Tile *tempTile;

    if (doScaleDigRate)
        digRate = scaleDigRate(digRate);

    double amountDug = 0.0;

    if (!isDiggable())
        return 0.0;

    sem_wait(&fullnessLockSemaphore);
    if (digRate >= fullness)
    {
        amountDug = fullness;
        sem_post(&fullnessLockSemaphore);
        setFullness(0.0);
        setType(dirt);
    }
    else
    {
        sem_post(&fullnessLockSemaphore);
        amountDug = digRate;
        setFullness(fullness - digRate);
    }

    // Force all the neighbors to recheck their meshes as we may have exposed
    // a new side that was not visible before.
    sem_wait(&neighborsLockSemaphore);
    for (unsigned int j = 0; j < neighbors.size(); ++j)
    {
        neighbors[j]->setFullness(neighbors[j]->getFullness());
    }
    sem_post(&neighborsLockSemaphore);

    if (amountDug > 0.0 && amountDug < digRate)
    {
        double amountToDig = digRate - amountDug;
        if (amountToDig < 0.05)
            return amountDug;

        sem_wait(&neighborsLockSemaphore);
        amountToDig /= (double) neighbors.size();
        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            if (neighbors[j]->getType() == dirt)
            {
                tempTile = neighbors[j];
                // Release and relock the semaphore since the digOut() routine will eventually need to lock it.
                sem_post(&neighborsLockSemaphore);
                amountDug += tempTile->digOut(amountToDig);
                sem_wait(&neighborsLockSemaphore);
            }
        }
        sem_post(&neighborsLockSemaphore);
    }

    return amountDug;
}

double Tile::scaleDigRate(double digRate)
{
    switch (type)
    {
        case claimed:
            return 0.2 * digRate;

        default:
            return digRate;
    }
}

Tile* Tile::getNeighbor(unsigned int index)
{
    sem_wait(&neighborsLockSemaphore);
    Tile *ret = neighbors[index];
    sem_post(&neighborsLockSemaphore);

    return ret;
}

std::vector<Tile*> Tile::getAllNeighbors()
{
    sem_wait(&neighborsLockSemaphore);
    std::vector<Tile*> ret = neighbors;
    sem_post(&neighborsLockSemaphore);

    return ret;
}

int Tile::getColor()
{
    return color;
}

void Tile::setColor(int nColor)
{
    color = nColor;

    refreshMesh();
}

