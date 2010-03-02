#include <sstream>
using namespace std;

#include "MapLight.h"

MapLight::MapLight()
{
	static unsigned int lightNumber = 1;

	ogreEntityExists = false;

	stringstream tempSS;
	tempSS << "Map_light_ " << lightNumber;
	properties.name = tempSS.str();

	lightNumber++;
}

void MapLight::setLocation(Ogre::Vector3 nPosition)
{
	//TODO: This needs to make a renderRequest to actually move the light.
	properties.position = nPosition;
}

void MapLight::setDiffuseColor(double red, double green, double blue)
{
	properties.diffuseColor = Ogre::ColourValue(red, green, blue);
	//TODO: Call refresh of the OGRE entity.
}

void MapLight::setSpecularColor(double red, double green, double blue)
{
	properties.specularColor = Ogre::ColourValue(red, green, blue);
	//TODO: Call refresh of the OGRE entity.
}

void MapLight::setAttenuation (double range, double constant, double linear, double quadratic)
{
	properties.attenuationRange = range;
	properties.attenuationConstant = constant;
	properties.attenuationLinear = linear;
	properties.attenuationQuadratic = quadratic;
	//TODO: Call refresh of the OGRE entity.
}

ostream& operator<<(ostream& os, MapLight *m)
{
	os << m->properties.position.x << "\t" << m->properties.position.y << "\t" << m->properties.position.z << "\t";
	os << m->properties.diffuseColor.r << "\t" << m->properties.diffuseColor.g << "\t" << m->properties.diffuseColor.b << "\t";
	os << m->properties.specularColor.r << "\t" << m->properties.specularColor.g << "\t" << m->properties.specularColor.b << "\t";
	os << m->properties.attenuationRange << "\t" << m->properties.attenuationConstant << "\t";
	os << m->properties.attenuationLinear << "\t" << m->properties.attenuationQuadratic;

	return os;
}

istream& operator>>(istream& is, MapLight *m)
{
	is >> m->properties.position.x >> m->properties.position.y >> m->properties.position.z;
	is >> m->properties.diffuseColor.r >> m->properties.diffuseColor.g >> m->properties.diffuseColor.b;
	is >> m->properties.specularColor.r >> m->properties.specularColor.g >> m->properties.specularColor.b;
	is >> m->properties.attenuationRange >> m->properties.attenuationConstant;
	is >> m->properties.attenuationLinear >> m->properties.attenuationQuadratic;

	return is;
}

