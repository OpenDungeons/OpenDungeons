#include "Globals.h"
#include "Functions.h"
#include "Room.h"

Room::Room()
{
	color = 0;
	controllingPlayer = NULL;
	meshExists = false;
}

Room* Room::createRoom(RoomType nType, const vector<Tile*> &nCoveredTiles, int nColor)
{
	Room *tempRoom = NULL;

	switch(nType)
	{
		case quarters:
			tempRoom = new RoomQuarters();
			break;

		case treasury:
			tempRoom = new RoomTreasury();
			break;
	}

	if(tempRoom == NULL)
	{
		cerr << "\n\n\nERROR: Trying to create a room of unknown type, bailing out.\n";
		cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__ << "\n\n\n";
		exit(1);
	}

	tempRoom->meshExists = false;
	tempRoom->color = nColor;

	static int uniqueNumber = -1;
	stringstream tempSS;

	//TODO: This should actually just call setType() but this will require a change to the >> operator.
	tempRoom->meshName = getMeshNameFromRoomType(nType);
	tempRoom->type = nType;

	tempSS.str("");
	tempSS << tempRoom->meshName << "_" << uniqueNumber;
	uniqueNumber--;
	tempRoom->name = tempSS.str();

	for(unsigned int i = 0; i < nCoveredTiles.size(); i++)
		tempRoom->addCoveredTile(nCoveredTiles[i]);

	return tempRoom;
}

void Room::absorbRoom(Room *r)
{
	// Subclasses overriding this function can call this to do the generic stuff or they can reiplement it entirely.
	while(r->numCoveredTiles() > 0)
	{
		Tile *tempTile = r->getCoveredTile(0);
		r->removeCoveredTile(tempTile);
		addCoveredTile(tempTile);
	}
}

Room* Room::createRoomFromStream(istream &is)
{
	Room tempRoom;
	is >> &tempRoom;

	Room *returnRoom = createRoom(tempRoom.type, tempRoom.coveredTiles, tempRoom.color);
	return returnRoom;
}

void Room::addCoveredTile(Tile* t, double nHP)
{
	coveredTiles.push_back(t);
	tileHP[t] = nHP;
	t->setCoveringRoom(this);
}

void Room::removeCoveredTile(Tile* t)
{
	for(unsigned int i = 0; i < coveredTiles.size(); i++)
	{
		if(t == coveredTiles[i])
		{
			coveredTiles.erase(coveredTiles.begin() + i);
			t->setCoveringRoom(NULL);
			tileHP.erase(t);
			break;
		}
	}

	// Destroy the mesh for this tile.
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyRoom;
	request->p = this;
	request->p2 = t;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

Tile* Room::getCoveredTile(int index)
{
	return coveredTiles[index];
}

/** \brief Returns all of the tiles which are part of this room, this is to conform to the AttackableObject interface.
  *
*/
vector<Tile*> Room::getCoveredTiles()
{
	return coveredTiles;
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
	if(meshExists)
		return;

	meshExists = true;

	for(unsigned int i = 0; i < coveredTiles.size(); i++)
	{
		Tile *tempTile = coveredTiles[i];
		RenderRequest *request = new RenderRequest;
		request->type = RenderRequest::createRoom;
		request->p = this;
		request->p2 = tempTile;

		// Add the request to the queue of rendering operations to be performed before the next frame.
		queueRenderRequest(request);
	}
}

void Room::destroyMeshes()
{
	if(!meshExists)
		return;

	meshExists = false;

	for(unsigned int i = 0; i < coveredTiles.size(); i++)
	{
		Tile *tempTile = coveredTiles[i];
		RenderRequest *request = new RenderRequest;
		request->type = RenderRequest::destroyRoom;
		request->p = this;
		request->p2 = tempTile;

		// Add the request to the queue of rendering operations to be performed before the next frame.
		queueRenderRequest(request);
	}
}

void Room::deleteYourself()
{
	destroyMeshes();

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::deleteRoom;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

Room::RoomType Room::getType()
{
	return type;
}

string Room::getFormat()
{
        return "meshName\tcolor\t\tNextLine: numTiles\t\tSubsequent Lines: tileX\ttileY";
}

/** \brief Carry out per turn upkeep on the room, the parameter r should be set to 'this' if called from a subclass to determine the room type.
  *
*/
void Room::doUpkeep(Room *r)
{
	// Do any generic upkeep here (i.e. any upkeep that all room types should do).  All base classes of room should call this function during their doUpkeep() routine.

	// If r is non-null we use it to determine the type of the room (quarters, treasury, etc) of the room so we can call the room specific functions.
	if(r != NULL)
	{
		// Loop over the tiles in Room r and remove any whose HP has dropped to zero.
		unsigned int i = 0;
		while(i < r->coveredTiles.size())
		{
			if(r->tileHP[r->coveredTiles[i]] <= 0.0)
				r->removeCoveredTile(r->coveredTiles[i]);
			else
				i++;
		}
	}
}

istream& operator>>(istream& is, Room *r)
{
	static int uniqueNumber = 1;
	int tilesToLoad, tempX, tempY;
	string tempString;
	stringstream tempSS;

	is >> r->meshName >> r->color;

	tempSS.str("");
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
	
	r->type = Room::getRoomTypeFromMeshName(r->meshName);
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

string Room::getMeshNameFromRoomType(RoomType t)
{
	switch(t)
	{
		case nullRoomType:      return "NullRoomType";      break;
		case dungeonTemple:     return "DungeonTemple";     break; 
		case vein:              return "Vein";              break; 
		case quarters:          return "Quarters";          break; 
		case treasury:          return "Treasury";          break;
	}

	return "UnknownRoomType";
}

Room::RoomType Room::getRoomTypeFromMeshName(string s)
{
	if(s.compare("DungeonTemple") == 0)
		return dungeonTemple;
	else if(s.compare("Vein") == 0)
		return vein;
	else if(s.compare("Quarters") == 0)
		return quarters;
	else if(s.compare("Treasury") == 0)
		return treasury;
	else
	{
		cerr << "\n\n\nERROR:  Trying to get room type from unknown mesh name, bailing out.\n";
		cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__ << "\n\n\n";
		exit(1);
	}
}

int Room::costPerTile(RoomType t)
{
	switch(t)
	{
		case nullRoomType:      return 0;         break;
		case dungeonTemple:     return 10000;     break; 
		case vein:              return 25;        break; 
		case quarters:          return 250;       break; 
		case treasury:          return 125;       break;
	}

	return 0;
}

double Room::getHP(Tile *tile)
{
	return tileHP[tile];
}

double Room::getDefense()
{
	return 0.0;
}

void Room::takeDamage(double damage, Tile *tileTakingDamage)
{
	tileHP[tileTakingDamage] -= damage;
}

void Room::recieveExp(double experience)
{
	// Do nothing since Rooms do not have exp.
}

bool Room::isMobile()
{
	return false;
}

int Room::getLevel()
{
	// Since rooms do not have exp or level we just consider them level 1 for compatibility with the AttackableObject interface.
	return 1;
}

int Room::getColor()
{
	return color;
}

string Room::getName()
{
	return name;
}

AttackableObject::AttackableObjectType Room::getAttackableObjectType()
{
	return AttackableObject::room;
}

