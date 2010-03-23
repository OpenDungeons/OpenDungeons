#include "CreatureClass.h"

CreatureClass::CreatureClass()
{
	sightRadius = 10.0;
	digRate = 0.0;
	danceRate = 0.0;
	hpPerLevel = 0.0;
	manaPerLevel = 0.0;
	moveSpeed = 1.0;
	maxHP = 10.0;
	maxMana = 10.0;
}

CreatureClass::CreatureClass(string nClassName, string nMeshName, Ogre::Vector3 nScale, double nHPPerLevel, double nManaPerLevel,\
				double nSightRadius, double nDigRate, double nMoveSpeed)
{
	className = nClassName;
	meshName = nMeshName;
	scale = nScale;

	hpPerLevel = nHPPerLevel;
	manaPerLevel = nManaPerLevel;
	sightRadius = nSightRadius;
	digRate = nDigRate;
	moveSpeed = nMoveSpeed;
}

string CreatureClass::getFormat()
{
	return "# className\tmeshName\tscaleX\tscaleY\tscaleZ\thp/level\tmana/level\tsightRadius\tdigRate\tdanceRate\tmoveSpeed\n";
}

ostream& operator<<(ostream& os, CreatureClass *c)
{
	os << c->className << "\t" << c->meshName << "\t" << c->scale.x << "\t" << c->scale.y << "\t" << c->scale.z << "\t";
	os << c->hpPerLevel << "\t" << c->manaPerLevel << "\t";
	os << c->sightRadius << "\t" << c->digRate << "\t" << c->danceRate << "\t" << c->moveSpeed;
	return os;
}

istream& operator>>(istream& is, CreatureClass *c)
{
	is >> c->className >> c->meshName >> c->scale.x >> c->scale.y >> c->scale.z;
	is >> c->hpPerLevel >> c->manaPerLevel;
	is >> c->sightRadius >> c->digRate >> c->danceRate >> c->moveSpeed;

	return is;
}

