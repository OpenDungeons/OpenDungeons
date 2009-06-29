#include "Globals.h"
#include "Room.h"

Room::Room()
{
	HP = 10;
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

		sem_wait(&renderQueueSemaphore);
		renderQueue.push_back(request);
		sem_post(&renderQueueSemaphore);
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

		sem_wait(&renderQueueSemaphore);
		renderQueue.push_back(request);
		sem_post(&renderQueueSemaphore);
	}
}

void Room::deleteYourself()
{
	destroyMeshes();

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::deleteRoom;
	request->p = this;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
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

