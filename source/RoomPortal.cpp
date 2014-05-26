/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "RoomPortal.h"

#include "ODServer.h"
#include "Seat.h"
#include "Player.h"
#include "Creature.h"
#include "Tile.h"
#include "RoomObject.h"
#include "GameMap.h"
#include "Weapon.h"
#include "CreatureAction.h"
#include "Random.h"

#include <cmath>

RoomPortal::RoomPortal() :
        mSpawnCreatureCountdown(0),
        mXCenter(0),
        mYCenter(0),
        mPortalObject(NULL)
{
    mType = portal;
}

void RoomPortal::createMesh()
{
    Room::createMesh();

    mPortalObject = loadRoomObject("PortalObject");
    createRoomObjectMeshes();

    mPortalObject->setAnimationState("Idle");
}

void RoomPortal::addCoveredTile(Tile* t, double nHP)
{
    Room::addCoveredTile(t, nHP);
    recomputeCenterPosition();
}

void RoomPortal::removeCoveredTile(Tile* t)
{
    Room::removeCoveredTile(t);
    recomputeCenterPosition();
}

bool RoomPortal::doUpkeep()
{
    // Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
    Room::doUpkeep();

    if (mSpawnCreatureCountdown > 0)
    {
        --mSpawnCreatureCountdown;
        return true;
    }

    // Randomly choose to spawn a creature.
    //TODO:  Improve this probability calculation.
    // Count how many creatures are controlled by this color, count both the ones on
    // the gameMap and the ones in all the players of that colors' hands'.
    double numCreatures = getGameMap()->getCreaturesByColor(getColor()).size();
    Seat *controllingSeat = getGameMap()->getSeatByColor(getColor());
    for(unsigned int i = 0, numPlayers = getGameMap()->numPlayers();
    		i < numPlayers; ++i)
    {
        Player *tempPlayer = getGameMap()->getPlayer(i);
        if (tempPlayer->getSeat() == controllingSeat)
            numCreatures += tempPlayer->numCreaturesInHand();
    }
    const double maxCreatures = 15;
    double targetProbability = powl((maxCreatures - numCreatures) / maxCreatures, 1.5);
    if (Random::Double(0.0, 1.0) <= targetProbability)
        spawnCreature();

    return true;
}

void RoomPortal::spawnCreature()
{
    std::cout << "Portal: " << getName() << "  spawn creature..." << std::endl;
    CreatureDefinition *classToSpawn = NULL;

    if (mPortalObject != NULL)
        mPortalObject->setAnimationState("Spawn", false);

    // If the room has been destroyed, or has not yet been assigned any tiles, then we
    // cannot determine where to place the new creature and we should just give up.
    if (mCoveredTiles.empty())
        return;

    // Compute and normalize the probabilities based on the current composition of creatures in the dungeon.
    recomputeClassProbabilities();

    // Determine which class the creature we spawn will be.
    double randomValue = Random::Double(0.0, 1.0);
    for (unsigned int i = 0; i < mClassProbabilities.size(); ++i)
    {
        randomValue -= mClassProbabilities[i].second;

        // When the randomValue drops below 0 it is because the cumulative probability values so far have
        // exceeded it and the one that finally made it negative is the one we choose.  This makes it more
        // likely that classes with large probability will be chosen since they are likely to be the
        // one that pushed it into the negatives.
        if (randomValue <= 0.0)
        {
            classToSpawn = mClassProbabilities[i].first;
            break;
        }
    }

    std::cout << "Spawning a creature of class " << classToSpawn->getClassName() << std::endl;

    // Create a new creature and copy over the class-based creature parameters.
    Creature *newCreature = new Creature(getGameMap());
    newCreature->setCreatureDefinition(classToSpawn);

    // Set the creature specific parameters.
    //NOTE:  This needs to be modified manually when the level file creature format changes.
    newCreature->setName(newCreature->getUniqueCreatureName());
    newCreature->setPosition(Ogre::Vector3((Ogre::Real)mXCenter, (Ogre::Real)mYCenter, (Ogre::Real)0.0));
    newCreature->setColor(getColor());

    //NOTE:  This needs to be modified manually when the level file weapon format changes.
    newCreature->setWeaponL(new Weapon("none", 0, 1.0, 0, "L", newCreature));
    newCreature->setWeaponR(new Weapon("none", 0, 1.0, 0, "R", newCreature));

    // Add the creature to the gameMap and create meshes so it is visible.
    getGameMap()->addCreature(newCreature);
    newCreature->createMesh();
    newCreature->getWeaponL()->createMesh();
    newCreature->getWeaponR()->createMesh();

    mSpawnCreatureCountdown = Random::Uint(15, 30);

    //TODO: Inform the clients that this creature has been created by placing a newCreature message in the serverNotificationQueue.
}

void RoomPortal::recomputeClassProbabilities()
{
    double probability, totalProbability = 0.0, tempDouble;
    Seat* controllingSeat = getGameMap()->getSeatByColor(getColor());

    // Normalize the faction and alignment coefficients.
    tempDouble = controllingSeat->factionHumans
                 + controllingSeat->factionCorpars + controllingSeat->factionUndead
                 + controllingSeat->factionConstructs
                 + controllingSeat->factionDenizens;
    if (std::fabs(tempDouble) > 0.000001)
    {
        controllingSeat->factionHumans /= tempDouble;
        controllingSeat->factionCorpars /= tempDouble;
        controllingSeat->factionUndead /= tempDouble;
        controllingSeat->factionConstructs /= tempDouble;
        controllingSeat->factionDenizens /= tempDouble;
    }

    tempDouble = controllingSeat->alignmentAltruism
                 + controllingSeat->alignmentOrder + controllingSeat->alignmentPeace;
    if (std::fabs(tempDouble) > 0.000001)
    {
        controllingSeat->alignmentAltruism /= tempDouble;
        controllingSeat->alignmentOrder /= tempDouble;
        controllingSeat->alignmentPeace /= tempDouble;
    }

    // Loop over the CreatureClasses in the gameMap and for each one, compute
    // the probability that a creature of that type will be selected.
    mClassProbabilities.clear();
    for (unsigned int i = 0; i < getGameMap()->numClassDescriptions(); ++i)
    {
        CreatureDefinition *tempClass = getGameMap()->getClassDescription(i);

        // Compute the probability that a creature of the current class will be chosen.
        //TODO:  Actually implement this probability calculation.
        probability = 1.0 / getGameMap()->numClassDescriptions();

        probability += controllingSeat->factionHumans
                * tempClass->getCoefficientHumans();
        probability += controllingSeat->factionCorpars
                * tempClass->getCoefficientCorpars();
        probability += controllingSeat->factionUndead
                * tempClass->getCoefficientUndead();
        probability += controllingSeat->factionConstructs
                * tempClass->getCoefficientConstructs();
        probability += controllingSeat->factionDenizens
                * tempClass->getCoefficientDenizens();
        probability += controllingSeat->alignmentAltruism
                * tempClass->getCoefficientAltruism();
        probability += controllingSeat->alignmentOrder
                * tempClass->getCoefficientOrder();
        probability += controllingSeat->alignmentPeace
                * tempClass->getCoefficientPeace();

        // Store the computed probability and compute the sum of the probabilities to be used for renormalization.
        mClassProbabilities.push_back(std::pair<CreatureDefinition*, double> (tempClass, probability));
        totalProbability += probability;
    }

    // Loop over the stored probabilities and renormalise them (i.e. divide each by the total so the sum is 1.0).
    if (std::fabs(totalProbability) > 0.000001)
    {
        for (unsigned int i = 0; i < mClassProbabilities.size(); ++i)
        {
            probability = mClassProbabilities[i].second / totalProbability;
            mClassProbabilities[i].second = probability;
        }
    }
}

void RoomPortal::recomputeCenterPosition()
{
    mXCenter = mYCenter = 0.0;

    if (mCoveredTiles.empty())
        return;

    // Loop over the covered tiles and compute the average location (i.e. the center) of the portal.
    for (unsigned int i = 0; i < mCoveredTiles.size(); ++i)
    {
        Tile *tempTile = mCoveredTiles[i];
        mXCenter += tempTile->x;
        mYCenter += tempTile->y;
    }

    mXCenter /= static_cast<double>(mCoveredTiles.size());
    mYCenter /= static_cast<double>(mCoveredTiles.size());
}
