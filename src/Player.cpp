#include "Globals.h"
#include "Player.h"

Player::Player()
{
	nick =  "";
	color = 1;
	mana = 1000;
	humanPlayer = true;
}

unsigned int Player::numCreaturesInHand()
{
	return creaturesInHand.size();
}

Creature* Player::getCreatureInHand(int i)
{
	return creaturesInHand[i];
}

void Player::addCreatureToHand(Creature *c)
{
	creaturesInHand.push_back(c);
}

/*! \brief Check to see if it is the user or another player picking up the creature and act accordingly.
 *
 * This function takes care of all of the operations required for a player to
 * pick up a creature.  If the player is the user we need to move the creature
 * oncreen to the "hand" as well as add the creature to the list of creatures
 * in our own hand, this is done by setting moveToHand to true.  If move to
 * hand is false we just hide the creature (and stop its AI, etc.), rather than
 * making it follow the cursor.
 */
void Player::pickUpCreature(Creature *c)
{
	// Stop the creature walking and take it off the gameMap to prevent the AI from running on it.
	c->clearDestinations();
	gameMap.removeCreature(c);

	// Start tracking this creature as being in this player's hand
	addCreatureToHand(c);

	if(serverSocket != NULL || clientSocket != NULL)
	{
		// Inform the clients
		if(serverSocket != NULL)
		{
			// Place a message in the queue to inform the clients that we picked up this creature
			ServerNotification *serverNotification = new ServerNotification;
			serverNotification->type = ServerNotification::creaturePickUp;
			serverNotification->cre = c;
			serverNotification->player = this;

			sem_wait(&serverNotificationQueueLockSemaphore);
			serverNotificationQueue.push_back(serverNotification);
			sem_post(&serverNotificationQueueLockSemaphore);

			sem_post(&serverNotificationQueueSemaphore);
		}

		// If it is actually the user picking up a creature we move the scene node and inform
		// the server, otherwise we just hide the creature from the map.
		if(this == gameMap.me)
		{
			// Send a render request to move the crature into the "hand"
			RenderRequest *request = new RenderRequest;
			request->type = RenderRequest::pickUpCreature;
			request->p = c;

			sem_wait(&renderQueueSemaphore);
			renderQueue.push_back(request);
			sem_post(&renderQueueSemaphore);

			if(clientSocket != NULL)
			{
				// Send a message to the server telling it we picked up this creature
				ClientNotification *clientNotification = new ClientNotification;
				clientNotification->type = ClientNotification::creaturePickUp;
				clientNotification->p = c;
				clientNotification->p2 = this;

				sem_wait(&clientNotificationQueueLockSemaphore);
				clientNotificationQueue.push_back(clientNotification);
				sem_post(&clientNotificationQueueLockSemaphore);

				sem_post(&clientNotificationQueueSemaphore);
			}
		}
		else // it is just a message indicating another player has picked up a creature
		{
			// Hide the creature
			c->destroyMesh();
		}

	}
}

void Player::removeCreatureFromHand(int i)
{
	vector<Creature*>::iterator curCreature = creaturesInHand.begin();
	while(i > 0 && curCreature != creaturesInHand.end())
	{
		i--;
		curCreature++;
	}

	creaturesInHand.erase(curCreature);
}

