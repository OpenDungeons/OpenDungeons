#ifndef ROOMOBJECT_H
#define ROOMOBJECT_H

#include "Room.h"
#include "ActiveObject.h"

class RoomObject : public ActiveObject
{
	public:
		RoomObject(Room *nParentRoom, string nMeshName);

		string getName();
		string getMeshName();

		void createMesh();
		void destroyMesh();
		void deleteYourself();

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

