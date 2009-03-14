#ifndef RENDERREQUEST_H
#define RENDERREQUEST_H

#include <iostream>
using namespace std;

class RenderRequest
{
	public:
		enum RequestType {createTile, refreshTile, createCreature, noRequest};

		RenderRequest();

		RequestType type;
		void *p;
};

#endif

