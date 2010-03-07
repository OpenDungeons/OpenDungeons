#ifndef RENDERREQUEST_H
#define RENDERREQUEST_H

#include <iostream>
#include <semaphore.h>
using namespace std;

#include <Ogre.h>

/*! \brief A data structure to be used for requesting that the OGRE rendering thread perform certain tasks.
 *
 *  This data structure is used filled out with a request and then placed in
 *  the global renderQueue.  The requests are taken out of the queue and
 *  processed by the frameStarted event in the ExampleFrameListener class. 
 */
class RenderRequest
{
	public:
		enum RequestType {createTile, refreshTile, destroyTile, deleteTile,
				createCreature, destroyCreature, deleteCreature, setCreatureAnimationState,
				createCreatureVisualDebug, destroyCreatureVisualDebug,
				createWeapon, destroyWeapon,
				pickUpCreature, dropCreature,
				createRoom, destroyRoom, deleteRoom,
				createMapLight, updateMapLight, destroyMapLight, destroyMapLightVisualIndicator, deleteMapLight,
				createField, refreshField, destroyField,
				moveSceneNode,
			      	noRequest};

		RenderRequest();

		RequestType type;
		void *p;
		void *p2;
		void *p3;
		string str;
		Ogre::Vector3 vec;
		//TODO:  Add a pointer called destroyMe which is used to pass a void pointer which should be deleted after it is used, this can replace the need for str and vec.
};

#endif

