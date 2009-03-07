#include "Creature.h"
#include "Defines.h"
#include "Globals.h"

Creature::Creature()
{
	position = Ogre::Vector3(0,0,0);
	scale = Ogre::Vector3(1,1,1);
}

Creature::Creature(string nClassName, string nMeshName, Ogre::Vector3 nScale)
{
	className = nClassName;
	meshName = nMeshName;
	scale = nScale;
}

ostream& operator<<(ostream& os, Creature *c)
{
	os << c->className << "\t" << c->name << "\t";
	os << c->position.x << "\t" << c->position.y << "\t" << c->position.z << "\n";

	return os;
}

istream& operator>>(istream& is, Creature *c)
{
	double xLocation = 0.0, yLocation = 0.0, zLocation = 0.0;
	is >> c->className >> c->name >> xLocation >> yLocation >> zLocation;
	c->position = Ogre::Vector3(xLocation, yLocation, zLocation);

	return is;
}

void Creature::createMesh()
{
	Entity *ent;
	SceneNode *node;

	ent = mSceneMgr->createEntity( ("Creature_" + name).c_str(), meshName.c_str());
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode( (name + "_node").c_str() );
	node->setPosition(position/BLENDER_UNITS_PER_OGRE_UNIT);
	//FIXME: Something needs to be done about the caling issue here.
	//node->setScale(1.0/BLENDER_UNITS_PER_OGRE_UNIT, 1.0/BLENDER_UNITS_PER_OGRE_UNIT, 1.0/BLENDER_UNITS_PER_OGRE_UNIT);
	node->setScale(scale);
	node->attachObject(ent);
}

void Creature::destroyMesh()
{
	Entity *ent;
	SceneNode *node;

	ent = mSceneMgr->getEntity( ("Creature_" + name).c_str() );
	node = mSceneMgr->getSceneNode( (name + "_node").c_str() );
	mSceneMgr->getRootSceneNode()->removeChild( node );
	node->detachObject( ent );
	mSceneMgr->destroyEntity( ent );
	mSceneMgr->destroySceneNode( (name + "_node") );
}

