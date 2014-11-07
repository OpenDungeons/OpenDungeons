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

#include "rooms/RoomTrainingHall.h"

#include "entities/Creature.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

const Ogre::Real RoomTrainingHall::OFFSET_CREATURE = 0.3;
const Ogre::Real RoomTrainingHall::OFFSET_DUMMY = 0.3;

RoomTrainingHall::RoomTrainingHall(GameMap* gameMap) :
    Room(gameMap),
    nbTurnsNoChangeDummies(0)
{
    setMeshName("Dojo");
}

void RoomTrainingHall::absorbRoom(Room *r)
{
    RoomTrainingHall* rd = static_cast<RoomTrainingHall*>(r);
    mUnusedDummies.insert(mUnusedDummies.end(), rd->mUnusedDummies.begin(), rd->mUnusedDummies.end());
    rd->mUnusedDummies.clear();

    Room::absorbRoom(r);
}

RenderedMovableEntity* RoomTrainingHall::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            y += OFFSET_DUMMY;
            mUnusedDummies.push_back(tile);
            int result = Random::Int(0, 5);
            if(result < 2)
                return loadBuildingObject(getGameMap(), "TrainingDummy2", tile, x, y, 0.0);
            else if (result < 4)
                return loadBuildingObject(getGameMap(), "TrainingDummy3", tile, x, y, 0.0);
            else
                return loadBuildingObject(getGameMap(), "TrainingDummy4", tile, x, y, 0.0);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            x -= OFFSET_DUMMY;
            mUnusedDummies.push_back(tile);
            return loadBuildingObject(getGameMap(), "TrainingDummy1", tile, x, y, 90.0);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            x += OFFSET_DUMMY;
            mUnusedDummies.push_back(tile);
            return loadBuildingObject(getGameMap(), "TrainingDummy1", tile, x, y, 270.0);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            y += OFFSET_DUMMY;
            mUnusedDummies.push_back(tile);
            return loadBuildingObject(getGameMap(), "TrainingDummy1", tile, x, y, 0.0);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            y -= OFFSET_DUMMY;
            mUnusedDummies.push_back(tile);
            return loadBuildingObject(getGameMap(), "TrainingDummy1", tile, x, y, 180.0);
        }
    }
    return NULL;
}

void RoomTrainingHall::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    Room::notifyActiveSpotRemoved(place, tile);

    for(std::map<Creature*,Tile*>::iterator it = mCreaturesDummies.begin(); it != mCreaturesDummies.end(); ++it)
    {
        Tile* tmpTile = it->second;
        if(tmpTile == tile)
        {
            Creature* creature = it->first;
            creature->changeJobRoom(NULL);
            // changeJobRoom should have released mCreaturesDummies[creature]. Now, we just need to release the unused dummy
            break;
        }
    }

    std::vector<Tile*>::iterator itEr = std::find(mUnusedDummies.begin(), mUnusedDummies.end(), tile);
    OD_ASSERT_TRUE_MSG(itEr != mUnusedDummies.end(), "name=" + getName() + ", tile=" + Tile::displayAsString(tile));
    if(itEr != mUnusedDummies.end())
        mUnusedDummies.erase(itEr);
}

void RoomTrainingHall::refreshCreaturesDummies()
{
    mCreaturesDummies.clear();
    mUnusedDummies.clear();
    nbTurnsNoChangeDummies = 0;

    mUnusedDummies.insert(mUnusedDummies.end(), mCentralActiveSpotTiles.begin(), mCentralActiveSpotTiles.end());
    mUnusedDummies.insert(mUnusedDummies.end(), mLeftWallsActiveSpotTiles.begin(), mLeftWallsActiveSpotTiles.end());
    mUnusedDummies.insert(mUnusedDummies.end(), mRightWallsActiveSpotTiles.begin(), mRightWallsActiveSpotTiles.end());
    mUnusedDummies.insert(mUnusedDummies.end(), mTopWallsActiveSpotTiles.begin(), mTopWallsActiveSpotTiles.end());
    mUnusedDummies.insert(mUnusedDummies.end(), mBottomWallsActiveSpotTiles.begin(), mBottomWallsActiveSpotTiles.end());

    if(mUnusedDummies.size() == 0 || mCreaturesUsingRoom.size() == 0)
        return;

    OD_ASSERT_TRUE_MSG(mUnusedDummies.size() >= mCreaturesUsingRoom.size(),
        "mUnusedDummies.size()=" + Ogre::StringConverter::toString(static_cast<int>(mUnusedDummies.size()))
        + "mCreaturesUsingRoom.size()=" + Ogre::StringConverter::toString(static_cast<int>(mCreaturesUsingRoom.size())));

    for(std::vector<Creature*>::iterator it = mCreaturesUsingRoom.begin(); it != mCreaturesUsingRoom.end(); ++it)
    {
        Creature* creature = *it;
        int index = Random::Int(0, mUnusedDummies.size() - 1);
        Tile* tileDummy = mUnusedDummies[index];
        mUnusedDummies.erase(mUnusedDummies.begin() + index);
        mCreaturesDummies[creature] = tileDummy;

        // Set destination to the newly affected dummies if there was a change
        Ogre::Vector3 creaturePosition = creature->getPosition();
        Ogre::Real wantedX, wantedY;
        getCreatureWantedPos(creature, tileDummy, wantedX, wantedY);
        if(creaturePosition.x != wantedX ||
           creaturePosition.y != wantedY)
        {
            // We move to the good tile
            std::list<Tile*> pathToDummy = getGameMap()->path(creature, tileDummy);
            OD_ASSERT_TRUE(!pathToDummy.empty());
            if(pathToDummy.empty())
                continue;

            creature->setWalkPath(pathToDummy, 0, false);
            // We add the last step to take account of the offset
            creature->addDestination(wantedX, wantedY);
            creature->setAnimationState("Walk");
        }
    }
}

bool RoomTrainingHall::hasOpenCreatureSpot(Creature* c)
{
    // Creatures can only train to level 10 at a trainingHall.
    //TODO: Check to see if the trainingHall has been upgraded to allow training to a higher level.
    if (c->getLevel() > 10)
        return false;

    // We accept all creatures as soon as there are free dummies
    return mUnusedDummies.size() > 0;
}

bool RoomTrainingHall::addCreatureUsingRoom(Creature* creature)
{
    if(!Room::addCreatureUsingRoom(creature))
        return false;

    int index = Random::Int(0, mUnusedDummies.size() - 1);
    Tile* tileDummy = mUnusedDummies[index];
    mUnusedDummies.erase(mUnusedDummies.begin() + index);
    mCreaturesDummies[creature] = tileDummy;
    Ogre::Vector3 creaturePosition = creature->getPosition();
    Ogre::Real wantedX, wantedY;
    getCreatureWantedPos(creature, tileDummy, wantedX, wantedY);
    if(creaturePosition.x != wantedX ||
       creaturePosition.y != wantedY)
    {
        // We move to the good tile
        std::list<Tile*> pathToDummy = getGameMap()->path(creature, tileDummy);
        OD_ASSERT_TRUE(!pathToDummy.empty());
        if(pathToDummy.empty())
            return true;

        creature->setWalkPath(pathToDummy, 0, false);
        // We add the last step to take account of the offset
        creature->addDestination(wantedX, wantedY);
        creature->setAnimationState("Walk");
    }

    return true;
}

void RoomTrainingHall::removeCreatureUsingRoom(Creature* c)
{
    Room::removeCreatureUsingRoom(c);
    if(mCreaturesDummies.count(c) > 0)
    {
        Tile* tileDummy = mCreaturesDummies[c];
        OD_ASSERT_TRUE(tileDummy != NULL);
        if(tileDummy == NULL)
            return;
        mUnusedDummies.push_back(tileDummy);
        mCreaturesDummies.erase(c);
    }
}

void RoomTrainingHall::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    // We add a probability to change dummies so that creatures do not use the same during too much time
    if(mCreaturesDummies.size() > 0 && Random::Int(5,50) < ++nbTurnsNoChangeDummies)
        refreshCreaturesDummies();

    for(std::map<Creature*,Tile*>::iterator it = mCreaturesDummies.begin(); it != mCreaturesDummies.end(); ++it)
    {
        Creature* creature = it->first;
        Tile* tileDummy = it->second;
        Tile* tileCreature = creature->getPositionTile();
        if(tileCreature == NULL)
            continue;

        Ogre::Real wantedX, wantedY;
        getCreatureWantedPos(creature, tileDummy, wantedX, wantedY);

        RenderedMovableEntity* ro = getBuildingObjectFromTile(tileDummy);
        OD_ASSERT_TRUE(ro != NULL);
        if(ro == NULL)
            continue;
        Ogre::Vector3 creaturePosition = creature->getPosition();
        if(creaturePosition.x == wantedX &&
           creaturePosition.y == wantedY)
        {
            if (creature->getJobCooldown() > 0)
            {
                creature->setAnimationState("Idle");
                creature->setJobCooldown(creature->getJobCooldown() - 1);
            }
            else
            {
                Ogre::Vector3 walkDirection(ro->getPosition().x - creature->getPosition().x, ro->getPosition().y - creature->getPosition().y, 0);
                walkDirection.normalise();
                creature->setAnimationState("Attack1", false, &walkDirection);

                ro->setAnimationState("Triggered", false);
                creature->receiveExp(ConfigManager::getSingleton().getRoomConfigDouble("HatcheryHpRecoveredPerChicken"));
                creature->jobDone(ConfigManager::getSingleton().getRoomConfigDouble("TrainHallAwaknessPerAttack"));
                creature->setJobCooldown(Random::Uint(ConfigManager::getSingleton().getRoomConfigUInt32("TrainHallCooldownHitMin"),
                    ConfigManager::getSingleton().getRoomConfigUInt32("TrainHallCooldownHitMax")));
            }
        }
    }
}

void RoomTrainingHall::getCreatureWantedPos(Creature* creature, Tile* tileDummy,
    Ogre::Real& wantedX, Ogre::Real& wantedY)
{
    RenderedMovableEntity* ro = getBuildingObjectFromTile(tileDummy);
    OD_ASSERT_TRUE(ro != NULL);
    if(ro == NULL)
        return;

    wantedX = static_cast<Ogre::Real>(tileDummy->getX());
    wantedY = static_cast<Ogre::Real>(tileDummy->getY());

    if(ro->getRotationAngle() == 0.0)
    {
        wantedY -= OFFSET_CREATURE;
    }
    else if(ro->getRotationAngle() == 90.0)
    {
        wantedX += OFFSET_CREATURE;
    }
    else if(ro->getRotationAngle() == 180.0)
    {
        wantedY += OFFSET_CREATURE;
    }
    else if(ro->getRotationAngle() == 270.0)
    {
        wantedX -= OFFSET_CREATURE;
    }
}
