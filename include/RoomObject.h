#ifndef ROOMOBJECT_H
#define ROOMOBJECT_H

#include "Room.h"
#include "ActiveObject.h"
#include "AnimatedObject.h"

class RoomObject : public ActiveObject, public AnimatedObject
{
	public:
		RoomObject(Room *nParentRoom, string nMeshName);

		string getName();
		string getMeshName();

		Room* getParentRoom();

		void createMesh();
		void destroyMesh();
		void deleteYourself();

		std::string getOgreNamePrefix();

		static string getFormat();
		friend ostream& operator<<(ostream& os, RoomObject *o);
		friend istream& operator>>(istream& is, RoomObject *o);

		double x, y;
		double rotationAngle;

	private:
		Room *parentRoom;
		bool meshExists;
		string name, meshName;
};

#endif

