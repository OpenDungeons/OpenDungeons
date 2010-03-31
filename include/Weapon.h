#ifndef WEAPON_H
#define WEAPON_H

#include <string>
#include <iostream>
using namespace std;

class Weapon
{
	public:
		Weapon();

		string name, meshName;
		double damage, range, defense;
		Creature *parentCreature;
		string handString;

		void createMesh();
		void destroyMesh();
		void deleteYourself();

		static string getFormat();
		friend ostream& operator<<(ostream& os, Weapon *w);
		friend istream& operator>>(istream& is, Weapon *w);

	private:
		bool meshExists;
};

#endif

