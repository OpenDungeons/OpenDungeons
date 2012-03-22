#include "Functions.h"
#include "Tile.h"
#include "RenderRequest.h"
#include "RenderManager.h"
#include "Seat.h"
#include "GameMap.h"
#include "TrapCannon.h"
#include "TrapBoulder.h"
#include "Random.h"
#include "SoundEffectsHelper.h"
#include "Player.h"

#include "Trap.h"

const double Trap::defaultTileHP = 10.0;

Trap::Trap() :
        reloadTime(0),
        reloadTimeCounter(0),
        minDamage(0.0),
        maxDamage(0.0),
        type(nullTrapType),
        exp(0.0)
{
}

Trap* Trap::createTrap(TrapType nType, const std::vector<Tile*> &nCoveredTiles,
                       Seat *nControllingSeat, void* params)
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
        case boulder:
            if(params != NULL) {
                int* p = (int*)params;
                tempTrap = new TrapBoulder(p[0], p[1]);
            }
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
    
    tempTrap->setControllingSeat(nControllingSeat);
    
    tempTrap->setMeshName(getMeshNameFromTrapType(nType));
    tempTrap->type = nType;
    
    static int uniqueNumber = -1;
    std::stringstream tempSS;
    
    tempSS.str("");
    tempSS << tempTrap->getMeshName() << "_" << uniqueNumber;
    --uniqueNumber;
    tempTrap->setName(tempSS.str());
    
    for (unsigned int i = 0; i < nCoveredTiles.size(); ++i)
        tempTrap->addCoveredTile(nCoveredTiles[i]);
    
    return tempTrap;
}

/** \brief Builds a trap for the current player.
 *  Builds a trap for the current player. Checks if the player has enough gold,
 *  if not, NULL is returned.
 *  \return The trap built, or NULL if the player does not have enough gold.
 */
Trap* Trap::buildTrap(GameMap* gameMap, Trap::TrapType nType, const std::vector< Tile* >& coveredTiles, Player* player, bool inEditor, void* params)
{
    //TODO: Use something better than a void pointer for this.
    int goldRequired = coveredTiles.size() * Trap::costPerTile(
                            nType);
    Trap* newTrap = NULL;
    if(player->getSeat()->getGold() > goldRequired || inEditor)
    {
        newTrap = createTrap(nType, coveredTiles, player->getSeat(), params);
        gameMap->addTrap(newTrap);
        if(!inEditor)
        {
            gameMap->withdrawFromTreasuries(goldRequired, player->getSeat()->getColor());
        }


        newTrap->createMesh();

        SoundEffectsHelper::getSingleton().playInterfaceSound(
                SoundEffectsHelper::BUILDTRAP, false);
    }
    return newTrap;
}

Trap* Trap::createTrapFromStream(std::istream &is, GameMap* gameMap)
{
    Trap tempTrap;
    tempTrap.setGameMap(gameMap);
    is >> &tempTrap;
    
    Trap *returnTrap = createTrap(tempTrap.type, tempTrap.coveredTiles,
                                  tempTrap.getControllingSeat());
    return returnTrap;
}

void Trap::createMesh()
{
    if (isMeshExisting())
        return;
    
    setMeshExisting(true);
    
    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::createTrap;
        request->p = static_cast<void*>(this);
        request->p2 = coveredTiles[i];
        
        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    }
}

void Trap::destroyMesh()
{
    if (!isMeshExisting())
        return;
    
    setMeshExisting(false);
    
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
    if (isMeshExisting())
        destroyMesh();
    
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

        case cannon:
            return "Cannon";

        case boulder:
            return "Boulder";

        default:
            return "UnknownTrapType";
    }
}

Trap::TrapType Trap::getTrapTypeFromMeshName(std::string s)
{
    if (s.compare("Cannon") == 0)
        return cannon;
    else if (s.compare("Boulder") == 0)
        return boulder;
    else if (s.compare("NullTrapType") == 0)
        return nullTrapType;
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

        case cannon:
            return 500;

        case boulder:
            return 500;

        default:
            return 100;
    }
}

bool Trap::doUpkeep()
{
    if(reloadTimeCounter > 0)
    {
        reloadTimeCounter--;
        return true;
    }
    
    std::vector<AttackableEntity*> enemyAttacked = aimEnemy();
    
    damage(enemyAttacked);
    
    if(!enemyAttacked.empty())
    {
        if(reloadTime >= 0)
        {
            // Begin the reload countdown.
            reloadTimeCounter = reloadTime;
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool Trap::doUpkeep(Trap* t)
{
    return t->doUpkeep();
}

void Trap::damage(std::vector<AttackableEntity*> enemyAttacked) 
{
    for(unsigned i=0;i<enemyAttacked.size();++i) 
    {
        enemyAttacked[i]->takeDamage(Random::Double(minDamage, maxDamage), enemyAttacked[i]->getCoveredTiles()[0]);
    }
}

std::vector<AttackableEntity*> Trap::aimEnemy()
{
    return std::vector<AttackableEntity*>();
}

void Trap::addCoveredTile(Tile* t, double nHP)
{
    coveredTiles.push_back(t);
    tileHP[t] = nHP;
    t->setCoveringTrap(true);
}

void Trap::removeCoveredTile(Tile* t)
{
    for (unsigned int i = 0; i < coveredTiles.size(); ++i)
    {
        if (t == coveredTiles[i])
        {
            coveredTiles.erase(coveredTiles.begin() + i);
            t->setCoveringTrap(false);
            tileHP.erase(t);
            break;
        }
    }
    /*
     *     // Destroy the mesh for this tile.
     *     RenderRequest *request = new RenderRequest;
     *     request->type = RenderRequest::destroyTrap;
     *     request->p = static_cast<void*>(this);
     *     request->p2 = t;
     * 
     *     // Add the request to the queue of rendering operations to be performed before the next frame.
     *     RenderManager::queueRenderRequest(request);
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

int Trap::getLevel() const
{
    return 1;
}

AttackableEntity::AttackableObjectType Trap::getAttackableObjectType() const
{
    return trap;
}

std::string Trap::getFormat()
{
    return "meshName\tcolor\t\tNextLine: numTiles\t\tSubsequent Lines: tileX\ttileY";
}

std::istream& operator>>(std::istream& is, Trap *t)
{
    static int uniqueNumber = 0;
    int tilesToLoad, tempX, tempY, tempInt;
    std::stringstream tempSS;
    
    std::string tempMeshName;
    is >> tempMeshName;
    t->setMeshName(tempMeshName);

    is >> tempInt;
    t->setControllingSeat(t->gameMap->getSeatByColor(tempInt));
    
    tempSS.str("");
    tempSS << t->getMeshName() << "_" << ++uniqueNumber;

    t->setName(tempSS.str());
    
    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile *tempTile = t->gameMap->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            t->addCoveredTile(tempTile);
            //FIXME: This next line will not be necessary when the the tile color is properly set by the tile load routine.
            tempTile->setColor(t->getControllingSeat()->color);
            tempTile->colorDouble = 1.0;
        }
    }
    
    t->type = Trap::getTrapTypeFromMeshName(t->getMeshName());
    return is;
}

std::ostream& operator<<(std::ostream& os, Trap *t)
{
    os << t->getMeshName() << "\t" << t->getControllingSeat()->color << "\n";
    os << t->coveredTiles.size() << "\n";
    for (unsigned int i = 0; i < t->coveredTiles.size(); ++i)
    {
        Tile *tempTile = t->coveredTiles[i];
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }
    
    return os;
}
