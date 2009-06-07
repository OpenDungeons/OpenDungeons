#ifndef RENDERREQUEST_H
#define RENDERREQUEST_H

#include <iostream>
#include <semaphore.h>
using namespace std;

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
			      	noRequest};

		RenderRequest();

		RequestType type;
		void *p;
		string str;
};

#endif

