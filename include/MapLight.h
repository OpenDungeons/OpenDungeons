#ifndef MAPLIGHT_H
#define MAPLIGHT_H

#include <iostream>
#include <string>
using namespace std;

#include <Ogre.h>

struct MapLightProperties
{
	Ogre::Vector3 position;
	Ogre::ColourValue diffuseColor;
	Ogre::ColourValue specularColor;

	double attenuationRange;
	double attenuationConstant;
	double attenuationLinear;
	double attenuationQuadratic;

	string name;
};

class MapLight
{
	public:
		MapLight();

		void setLocation(Ogre::Vector3 nPosition);
		void setDiffuseColor(double red, double green, double blue);
		void setSpecularColor(double red, double green, double blue);
		void setAttenuation (double range, double constant, double linear, double quadratic);
		//void createOgreEntity();
		//void destroyOgreEntity();

		friend ostream& operator<<(ostream& os, MapLight *m);
		friend istream& operator>>(istream& is, MapLight *m);

	private:
		MapLightProperties properties;
		bool ogreEntityExists;
};

#endif

