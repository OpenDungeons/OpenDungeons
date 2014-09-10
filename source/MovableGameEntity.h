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

#ifndef MOVABLEGAMEENTITY_H
#define MOVABLEGAMEENTITY_H

#include "GameEntity.h"

#include <OgreVector3.h>
#include <OgreAnimationState.h>
#include <OgreSceneNode.h>

#include <deque>
#include <list>
#include <vector>

class Tile;

class MovableGameEntity : public GameEntity
{
public:
    MovableGameEntity(GameMap* gameMap);

    virtual ~MovableGameEntity()
    {}

    /*! \brief Adds a position in 3d space to an animated object's walk queue and, if necessary, starts it walking.
     *
     * This function also notify the server so that relevant clients are informed about the change.
     */
    void addDestination(Ogre::Real x, Ogre::Real y, Ogre::Real z = 0.0);

    //! \brief Checks if the destination queue is empty
    bool isMoving();


    /*! \brief Replaces a object's current walk queue with a new path.
     *
     * This replacement is done if, and only if, the new path is at least minDestinations
     * long; if addFirstStop is false the new path will start with the second entry in path.
     */
    bool setWalkPath(std::list<Tile*> path, unsigned int minDestinations, bool addFirstStop);

    //! \brief Clears all future destinations from the walk queue, stops the object where it is, and sets its animation state.
    void clearDestinations();

    //! \brief Stops the object where it is, and sets its animation state.
    virtual void stopWalking();

    virtual double getMoveSpeed() const
    { return mMoveSpeed; }

    virtual void setMoveSpeed(double s);

    virtual void setAnimationState(const std::string& state, bool loop = true, Ogre::Vector3* direction = NULL);

    virtual double getAnimationSpeedFactor();
    virtual void setAnimationSpeedFactor(double f);

    //! \brief Updates the entity path, movement, and direction
    //! \param timeSinceLastFrame the elapsed time since last displayed frame in seconds.
    virtual void update(Ogre::Real timeSinceLastFrame);

    void setWalkDirection(Ogre::Vector3& direction);

    virtual void setPosition(const Ogre::Vector3& v);

    Ogre::AnimationState* mAnimationState;
    Ogre::SceneNode* mSceneNode;

protected:
    std::deque<Ogre::Vector3> mWalkQueue;

private:
    double mMoveSpeed;
    std::string mPrevAnimationState;
    bool mPrevAnimationStateLoop;
    double mAnimationSpeedFactor;
    std::string mDestinationAnimationState;
};


#endif // MOVABLEGAMEENTITY_H
