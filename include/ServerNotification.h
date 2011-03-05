#ifndef SERVERNOTIFICATION_H
#define SERVERNOTIFICATION_H

#include <string>
using namespace std;
#include <Ogre.h>

#include "Tile.h"
#include "Player.h"
#include "AnimatedObject.h"

/*! \brief A data structure used to pass messages to the serverNotificationProcessor thread.
 *
 */
//TODO:  Make this class a base class and let specific messages be subclasses of this type with each having its own data structure so they don't need the unused fields
class ServerNotification
{
	public:
		enum ServerNotificationType
		{
			turnStarted,
			setTurnsPerSecond,

			tileFullnessChange,

			addMapLight,
			removeMapLight,

			animatedObjectAddDestination,
			animatedObjectClearDestinations,
			setObjectAnimationState,
			creaturePickUp,
			creatureDrop
		};

		//TODO:  Employ some void pointers on this to make this data structure smaller
		ServerNotificationType type;
		void *p;
		bool b;
		string str;
		Ogre::Vector3 vec;
		double doub;
		Tile *tile;
		Creature *cre;
		AnimatedObject *ani;
		Player *player;
		long int turnNumber;
};

#endif

