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

#include "Player.h"

#include "ODServer.h"
#include "ServerNotification.h"
#include "ODClient.h"
#include "Creature.h"
#include "GameMap.h"
#include "Seat.h"
#include "Weapon.h"
#include "RenderRequest.h"
#include "RenderManager.h"

Player::Player() :
    mNewRoomType(Room::nullRoomType),
    mNewTrapType(Trap::nullTrapType),
    mGameMap(NULL),
    mSeat(NULL),
    mHasAI(false)
{
}

unsigned int Player::numCreaturesInHand() const
{
    return mCreaturesInHand.size();
}

Creature* Player::getCreatureInHand(int i)
{
    return mCreaturesInHand[i];
}

const Creature* Player::getCreatureInHand(int i) const
{
    return mCreaturesInHand[i];
}

void Player::addCreatureToHand(Creature *c)
{
    if (mCreaturesInHand.empty())
    {
        mCreaturesInHand.push_back(c);
        return;
    }

    // creaturesInHand.push_front(c);
    // Since vectors have no push_front method,
    // we need to move all of the elements in the vector back one
    // and then add this one to the beginning.
    mCreaturesInHand.push_back(NULL);
    for (unsigned int j = mCreaturesInHand.size() - 1; j > 0; --j)
        mCreaturesInHand[j] = mCreaturesInHand[j - 1];

    mCreaturesInHand[0] = c;
}

void Player::pickUpCreature(Creature *c)
{
    assert(mGameMap != NULL);

    if (!ODServer::getSingleton().isConnected() && !ODClient::getSingleton().isConnected())
        return;

    if (c->getHP() <= 0.0)
        return;

    // Stop the creature walking and take it off the gameMap to prevent the AI from running on it.
    mGameMap->removeCreature(c);
    c->clearDestinations();
    c->clearActionQueue();

    // Start tracking this creature as being in this player's hand
    addCreatureToHand(c);

    // Destroy the creature's visual debugging entities if it has them
    if (!mGameMap->isServerGameMap() && c->getHasVisualDebuggingEntities())
        c->destroyVisualDebugEntities();

    if (c->getGameMap()->isServerGameMap())
        return;

    // If it is actually the user picking up a creature we move the scene node.
    // Otherwise we just hide the creature from the map.
    if (this == mGameMap->getLocalPlayer())
    {
        // Send a render request to move the crature into the "hand"
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::pickUpCreature;
        request->p = c;

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    }
    else // it is just a message indicating another player has picked up a creature
    {
        // Hide the creature
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::detachCreature;
        request->p = c;
        RenderManager::queueRenderRequest(request);
    }
}

void Player::removeCreatureFromHand(int i)
{
    mCreaturesInHand.erase(mCreaturesInHand.begin() + i);
}

bool Player::isDropCreaturePossible(Tile *t, unsigned int index)
{
    // if we have a creature to drop
    if (mCreaturesInHand.empty())
        return false;

    Creature* tempCreature = mCreaturesInHand[index];

    // if the tile is a valid place to drop a creature

    // check whether the tile is a ground tile ...
    if (t->getFullness() >= 1.0)
        return false;

    // ... and that the creature can dig, or we're putting it on a claimed tile of the team color.
    if ((tempCreature->getDigRate() < 0.1 || (t->getType() != Tile::dirt && t->getType() != Tile::gold))
            && (t->getType() != Tile::claimed || t->getColor() != getSeat()->getColor()))
        return false;

    return true;
}

void Player::dropCreature(Tile* t, unsigned int index)
{
    // Add the creature to the map
    Creature *c = mCreaturesInHand[index];
    mCreaturesInHand.erase(mCreaturesInHand.begin() + index);
    mGameMap->addCreature(c);
    c->drop(Ogre::Vector3(static_cast<Ogre::Real>(t->x),
        static_cast<Ogre::Real>(t->y), 0.0));

    if(c->getGameMap()->isServerGameMap())
        return;

    // If this is the result of another player dropping the creature it is currently not visible so we need to create a mesh for it
    //cout << "\nthis:  " << this << "\nme:  " << gameMap->getLocalPlayer() << endl;
    //cout.flush();
    if (this != mGameMap->getLocalPlayer())
    {
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::attachCreature;
        request->p = c;
        RenderManager::queueRenderRequest(request);
    }
    else // This is the result of the player on the local computer dropping the creature
    {
        // Send a render request to rearrange the creatures in the hand to move them all forward 1 place
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::dropCreature;
        request->p = c;
        request->p2 = this;
        RenderManager::queueRenderRequest(request);
    }
}

void Player::rotateCreaturesInHand(int n)
{
    // If there are no creatures or only one creature in our hand, rotation doesn't change the order.
    if (mCreaturesInHand.size() <= 1)
        return;

    for (unsigned int i = 0; i < (unsigned int) fabs((double) n); ++i)
    {
        if (n > 0)
        {
            Creature* tempCreature = mCreaturesInHand.back();

            //creaturesInHand.push_front(tempCreature);
            // Since vectors have no push_front method
            // we need to move all of the elements in the vector back one and then add this one to the beginning.
            for (unsigned int j = mCreaturesInHand.size() - 1; j > 0; --j)
                mCreaturesInHand[j] = mCreaturesInHand[j - 1];

            mCreaturesInHand[0] = tempCreature;

        }
        else
        {
            Creature* tempCreature = mCreaturesInHand.front();
            mCreaturesInHand.erase(mCreaturesInHand.begin());
            mCreaturesInHand.push_back(tempCreature);
        }
    }

    // Send a render request to move the crature into the "hand"
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::rotateCreaturesInHand;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}
