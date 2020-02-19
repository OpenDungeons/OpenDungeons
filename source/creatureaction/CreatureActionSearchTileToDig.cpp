/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "creatureaction/CreatureActionSearchTileToDig.h"

#include "creatureaction/CreatureActionDigTile.h"
#include "creatureaction/CreatureActionGrabEntity.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "entities/TreasuryObject.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "rooms/Room.h"
#include "utils/Helper.h"
#include "utils/MakeUnique.h"
#include "utils/LogManager.h"

CreatureActionSearchTileToDig::CreatureActionSearchTileToDig(Creature& creature, bool forced) :
    CreatureAction(creature),
    mForced(forced)
{
    mCreature.getSeat()->getPlayer()->notifyWorkerAction(mCreature, getType());
}

CreatureActionSearchTileToDig::~CreatureActionSearchTileToDig()
{
    mCreature.getSeat()->getPlayer()->notifyWorkerStopsAction(mCreature, getType());
}

std::function<bool()> CreatureActionSearchTileToDig::action()
{
    return std::bind(&CreatureActionSearchTileToDig::handleSearchTileToDig,
        std::ref(mCreature), getNbTurns(), mForced);
}

bool CreatureActionSearchTileToDig::handleSearchTileToDig(Creature& creature, int32_t nbTurns, bool forced)
{
    if(creature.getDigRate() <= 0.0)
        return false;

    Tile* myTile = creature.getPositionTile();
    if (myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    if(!forced)
    {
        // If we are digging tiles for too long, we stop to check if there is
        // something else to do
        if(nbTurns >= Creature::NB_TURNS_BEFORE_CHECKING_TASK)
        {
            creature.popAction();
            return true;
        }
    }

    // See if any of the tiles is one of our neighbors
    Player* tempPlayer = creature.getGameMap()->getPlayerBySeat(creature.getSeat());
    for (Tile* tempTile : myTile->getAllNeighbors())
    {
        if (tempPlayer == nullptr)
            break;

        if (!tempTile->getMarkedForDigging(tempPlayer))
            continue;

        // Check if there is still empty space for digging the tile
        std::vector<Tile*> tiles;
        tempTile->canWorkerDig(creature, tiles);
        if(tiles.empty())
            continue;

        // Check if the position tile is in the list
        bool isOk = false;
        for(Tile* tile : tiles)
        {
            if(tile != myTile)
                continue;

            isOk = true;
            break;
        }

        if(!isOk)
            continue;
        
        assert(tempTile!=nullptr);
        assert(myTile!=nullptr);
        // We found a tile marked by our controlling seat, dig out the tile.
        creature.pushAction(Utils::make_unique<CreatureActionDigTile>(creature, *tempTile, *myTile));
        return true;
    }

    // Find the closest tile to dig
    float distBest = -1;
    Tile* tileToDig = nullptr;
    Tile* tilePos = nullptr;
    for (Tile* tile : creature.getTilesWithinSightRadius())
    {
        // Check to see whether the tile is marked for digging
        if(!tile->getMarkedForDigging(tempPlayer))
            continue;

        // and there is still room to work on it
        std::vector<Tile*> tiles;
        tile->canWorkerDig(creature, tiles);
        if(tiles.empty())
            continue;

        // We search for the closest neighbor tile
        for (Tile* neighborTile : tiles)
        {
            if (!creature.getGameMap()->pathExists(&creature, myTile, neighborTile))
                continue;

            float dist = Pathfinding::squaredDistanceTile(*myTile, *neighborTile);
            if((distBest != -1) && (distBest <= dist))
                continue;

            distBest = dist;
            tileToDig = tile;
            tilePos = neighborTile;
        }
    }

    if((tileToDig != nullptr) && (tilePos != nullptr) && tileToDig->getPosition()!=Ogre::Vector3::ZERO && tilePos->getPosition()!=Ogre::Vector3::ZERO)
    {
        // We also push the dig action to lock the tile to make sure not every worker will try to go to the same tile
        creature.pushAction(Utils::make_unique<CreatureActionDigTile>(creature, *tileToDig, *tilePos));
        return true;
    }

    // If none of our neighbors are marked for digging we got here too late.
    // Finish digging
    creature.popAction();
    if(creature.getGoldCarried() > 0)
    {
        TreasuryObject* obj = new TreasuryObject(creature.getGameMap(), creature.getGoldCarried());
        creature.resetGoldCarried();
        Ogre::Vector3 pos(static_cast<Ogre::Real>(myTile->getX()), static_cast<Ogre::Real>(myTile->getY()), 0.0f);
        obj->addToGameMap();
        obj->createMesh();
        obj->setPosition(pos);

        bool isTreasuryAvailable = false;
        for(Room* room : creature.getGameMap()->getRooms())
        {
            if(room->getSeat() != creature.getSeat())
                continue;

            if(room->getTotalGoldStorage() <= 0)
                continue;

            if(room->getTotalGoldStored() >= room->getTotalGoldStorage())
                continue;

            if(room->numCoveredTiles() <= 0)
                continue;

            Tile* tile = room->getCoveredTile(0);
            if(!creature.getGameMap()->pathExists(&creature, myTile, tile))
                continue;

            isTreasuryAvailable = true;
            break;
        }
        if(isTreasuryAvailable)
        {
            // We do not push CreatureActionType::searchEntityToCarry because we want
            // this worker to be count as digging, not as carrying stuff
            creature.pushAction(Utils::make_unique<CreatureActionGrabEntity>(creature, *obj));
            return true;
        }
        else if(creature.getSeat()->getPlayer()->getIsHuman() &&
                !creature.getSeat()->getPlayer()->getHasLost())
        {
            creature.getSeat()->getPlayer()->notifyNoTreasuryAvailable();
        }
    }

    return true;
}
