#include "Player.h"
#include "ODServer.h"
#include "ServerNotification.h"
#include "Creature.h"
#include "GameMap.h"
#include "Seat.h"
#include "Weapon.h"
#include "ClientNotification.h"
#include "RenderRequest.h"
#include "RenderManager.h"
#include "Socket.h"

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

    if (Socket::serverSocket == NULL && Socket::clientSocket == NULL)
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
    if (c->getHasVisualDebuggingEntities())
        c->destroyVisualDebugEntities();

    // Inform the clients
    if (Socket::serverSocket != NULL)
    {
        // Place a message in the queue to inform the clients that we picked up this creature
        ServerNotification *serverNotification = new ServerNotification;
        serverNotification->type = ServerNotification::creaturePickUp;
        serverNotification->cre = c;
        serverNotification->player = this;

        ODServer::queueServerNotification(serverNotification);
    }

    // If it is actually the user picking up a creature we move the scene node and inform
    // the server, otherwise we just hide the creature from the map.
    if (this == mGameMap->getLocalPlayer())
    {
        // Send a render request to move the crature into the "hand"
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::pickUpCreature;
        request->p = c;

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);

        if (Socket::clientSocket != NULL)
        {
            // Send a message to the server telling it we picked up this creature
            ClientNotification *clientNotification = new ClientNotification;
            clientNotification->mType = ClientNotification::creaturePickUp;
            clientNotification->mP = c;
            clientNotification->mP2 = this;

            ClientNotification::mClientNotificationQueue.push_back(clientNotification);
        }
    }
    else // it is just a message indicating another player has picked up a creature
    {
        // Hide the creature
        c->destroyMesh();
    }
}

void Player::removeCreatureFromHand(int i)
{
    mCreaturesInHand.erase(mCreaturesInHand.begin() + i);
}

bool Player::dropCreature(Tile* t, unsigned int index)
{
    // if we have a creature to drop
    if (mCreaturesInHand.empty())
        return false;

    Creature* tempCreature = mCreaturesInHand[index];

    // if the tile is a valid place to drop a creature
    //FIXME: Those could be race conditions, if the tile state changes on the server before the client knows about it.

    // check whether the tile is a ground tile ...
    if (t->getFullness() >= 1.0)
        return false;

    // ... and that the creature can dig, or we're putting it on a claimed tile of the team color.
    if ((tempCreature->getDigRate() < 0.1 || t->getType() != Tile::dirt)
            && (t->getType() != Tile::claimed || t->getColor() != getSeat()->getColor()))
        return false;

    // Add the creature to the map
    Creature *c = mCreaturesInHand[index];
    mCreaturesInHand.erase(mCreaturesInHand.begin() + index);
    mGameMap->addCreature(c);

    // If this is the result of another player dropping the creature it is currently not visible so we need to create a mesh for it
    //cout << "\nthis:  " << this << "\nme:  " << gameMap->getLocalPlayer() << endl;
    //cout.flush();
    if (this != mGameMap->getLocalPlayer())
    {
        c->createMesh();
        c->getWeaponL()->createMesh();
        c->getWeaponR()->createMesh();
    }
    else // This is the result of the player on the local computer dropping the creature
    {
        // Send a render request to rearrange the creatures in the hand to move them all forward 1 place
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::dropCreature;
        request->p = c;
        request->p2 = this;

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    }

    c->setPosition(Ogre::Vector3(static_cast<Ogre::Real>(t->x),
        static_cast<Ogre::Real>(t->y), 0.0));

    if (this == mGameMap->getLocalPlayer() || mHasAI)
    {
        if (Socket::serverSocket != NULL)
        {
            // Place a message in the queue to inform the clients that we dropped this creature
            ServerNotification *serverNotification = new ServerNotification;
            serverNotification->type = ServerNotification::creatureDrop;
            serverNotification->player = this;
            serverNotification->tile = t;

            ODServer::queueServerNotification(serverNotification);
        }
        else if (Socket::clientSocket != NULL && this == mGameMap->getLocalPlayer())
        {
            // Send a message to the server telling it we dropped this creature
            ClientNotification *clientNotification = new ClientNotification;
            clientNotification->mType = ClientNotification::creatureDrop;
            clientNotification->mP = this;
            clientNotification->mP2 = t;

            ClientNotification::mClientNotificationQueue.push_back(clientNotification);
        }
    }

    return true;
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
