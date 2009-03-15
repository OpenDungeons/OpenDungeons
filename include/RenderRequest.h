#ifndef RENDERREQUEST_H
#define RENDERREQUEST_H

#include <iostream>
using namespace std;

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

