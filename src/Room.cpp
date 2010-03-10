#include "Globals.h"
#include "Functions.h"
#include "Room.h"

Room::Room()
{
	HP = 10;
	sem_init(&meshCreationFinishedSemaphore, 0, 0);
	sem_init(&meshDestructionFinishedSemaphore, 0, 0);
}

void Room::addCoveredTile(Tile* t)
{
	coveredTiles.push_back(t);
}

void Room::removeCoveredTile(Tile* t)
{
	for(unsigned int i = 0; i < coveredTiles.size(); i++)
	{
		if(t == coveredTiles[i])
		{
			coveredTiles.erase(coveredTiles.begin() + i);
			break;
		}
	}
}

Tile* Room::getCoveredTile(int index)
{
	return coveredTiles[index];
}

unsigned int Room::numCoveredTiles()
{
	return coveredTiles.size();
}

void Room::clearCoveredTiles()
{
	coveredTiles.clear();
}

void Room::createMeshes()
{
	for(unsigned int i = 0; i < coveredTiles.size(); i++)
	{
		Tile *tempTile = coveredTiles[i];
		RenderRequest *request = new RenderRequest;
		request->type = RenderRequest::createRoom;
		request->p = this;
		request->p2 = tempTile;

		// Add the request to the queue of rendering operations to be performed before the next frame.
		queueRenderRequest(request);
		//sem_wait(&meshCreationFinishedSemaphore);
	}
}

void Room::destroyMeshes()
{
	for(unsigned int i = 0; i < coveredTiles.size(); i++)
	{
		Tile *tempTile = coveredTiles[i];
		RenderRequest *request = new RenderRequest;
		request->type = RenderRequest::destroyRoom;
		request->p = this;
		request->p2 = tempTile;

		// Add the request to the queue of rendering operations to be performed before the next frame.
		queueRenderRequest(request);

		//FIXME:  This wait needs to happen however it currently causes the program to lock up because this function is called from the rendering thread which causes that thread to wait on itself
		//sem_wait(&meshDestructionFinishedSemaphore);
	}
}

void Room::deleteYourself()
{
	destroyMeshes();
	unsigned int numToWait = coveredTiles.size();
	for(unsigned int i = 0; i < numToWait; i++)
	{
		//sem_wait(&meshDestructionFinishedSemaphore);
	}

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::deleteRoom;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

string Room::getFormat()
{
        return "meshName\tcolor\t\tNextLine: numTiles\t\tSubsequent Lines: tileX\ttileY";
}

istream& operator>>(istream& is, Room *r)
{
	static int uniqueNumber = 1;
	int tilesToLoad, tempX, tempY;
	string tempString;
	stringstream tempSS;

	is >> r->meshName >> r->color;

	tempString = "";
	tempSS.str(tempString);
	tempSS << r->meshName << "_" << uniqueNumber;
	uniqueNumber++;
	r->name = tempSS.str();

	is >> tilesToLoad;
	for(int i = 0; i < tilesToLoad; i++)
	{
		is >> tempX >> tempY;
		Tile *tempTile = gameMap.getTile(tempX, tempY);
		if(tempTile != NULL)
		{
			r->addCoveredTile(tempTile);
			tempTile->color = r->color;
			tempTile->colorDouble = 1.0;
		}
	}
	
	return is;
}

ostream& operator<<(ostream& os, Room *r)
{
	os << r->meshName << "\t" << r->color << "\n";
	os << r->coveredTiles.size() << "\n";
	for(unsigned int i = 0; i < r->coveredTiles.size(); i++)
	{
		Tile *tempTile = r->coveredTiles[i];
		os << tempTile->x << "\t" << tempTile->y << "\n";
	}

	return os;
}

