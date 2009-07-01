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
	c->clearActionQueue();
	gameMap.removeCreature(c);

	// Start tracking this creature as being in this player's hand
	addCreatureToHand(c);

	// Destroy the creature's visual debugging entities if it has them
	if(c->getHasVisualDebuggingEntities())
	{
		c->destroyVisualDebugEntities();
	}

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

/*! \brief Check to see the first creatureInHand can be dropped on Tile t and do so if possible.
 *
 */
bool Player::dropCreature(Tile *t)
{

	// if we have a creature to drop
	if(creaturesInHand.size() > 0)
	{
		// if the tile is a valid place to drop a creature
		//FIXME:  This could be a race condition, if the tile state changes on the server before the client knows about it.
		if(t->getFullness() == 0)
		{
			// Pause the creature AI thread
			if(serverSocket != NULL)
			{
				sem_wait(&creatureAISemaphore);
			}

			// Add the creature to the map
			Creature *c = creaturesInHand[0];
			creaturesInHand.erase(creaturesInHand.begin());
			gameMap.addCreature(c);

			// If this is the result of another player dropping the creature it is currently not visible so we need to create a mesh for it
			//cout << "\nthis:  " << this << "\nme:  " << gameMap.me << endl;
			//cout.flush();
			if(this != gameMap.me)
			{
				c->createMesh();
				sem_wait(&c->meshCreationFinishedSemaphore);
			}
			else // This is the reult of the player on the local computer dropping the creature
			{
				// Send a render request to rearrange the creatures in the hand to move them all forward 1 place
				RenderRequest *request = new RenderRequest;
				request->type = RenderRequest::dropCreature;
				request->p = c;
				request->p2 = this;

				sem_wait(&renderQueueSemaphore);
				renderQueue.push_back(request);
				sem_post(&renderQueueSemaphore);
			}

			c->setPosition(t->x, t->y, 0.0);

			if(this == gameMap.me)
			{
				if(serverSocket != NULL)
				{
					// Place a message in the queue to inform the clients that we dropped this creature
					ServerNotification *serverNotification = new ServerNotification;
					serverNotification->type = ServerNotification::creatureDrop;
					serverNotification->player = this;
					serverNotification->tile = t;

					sem_wait(&serverNotificationQueueLockSemaphore);
					serverNotificationQueue.push_back(serverNotification);
					sem_post(&serverNotificationQueueLockSemaphore);

					sem_post(&serverNotificationQueueSemaphore);
				}
				else
				{
					if(clientSocket != NULL && this == gameMap.me)
					{
						// Send a message to the server telling it we dropped this creature
						ClientNotification *clientNotification = new ClientNotification;
						clientNotification->type = ClientNotification::creatureDrop;
						clientNotification->p = this;
						clientNotification->p2 = t;

						sem_wait(&clientNotificationQueueLockSemaphore);
						clientNotificationQueue.push_back(clientNotification);
						sem_post(&clientNotificationQueueLockSemaphore);

						sem_post(&clientNotificationQueueSemaphore);
					}
				}
			}

			// Resume the creature AI thread
			if(serverSocket != NULL)
			{
				sem_post(&creatureAISemaphore);
			}

			return true;
		}
	}

	return false;
}

