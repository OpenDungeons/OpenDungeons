#ifndef MAPLIGHT_H
#define MAPLIGHT_H

#include <iostream>
#include <string>
using namespace std;

#include <Ogre.h>

class MapLight
{
	public:
		MapLight();

		void setLocation(Ogre::Vector3 nPosition);
		void setDiffuseColor(double red, double green, double blue);
		void setSpecularColor(double red, double green, double blue);
		void setAttenuation (double range, double constant, double linear, double quadratic);

		void createOgreEntity();
		void destroyOgreEntity();
		void destroyOgreEntityVisualIndicator();
		void deleteYourself();

		string getName();
		void setPosition(double nX, double nY, double nZ);
		void setPosition(Ogre::Vector3 nPosition);
		Ogre::Vector3 getPosition();
		Ogre::ColourValue getDiffuseColor();
		Ogre::ColourValue getSpecularColor();
		double getAttenuationRange();
		double getAttenuationConstant();
		double getAttenuationLinear();
		double getAttenuationQuadratic();

		void advanceFlicker(double time);

		friend ostream& operator<<(ostream& os, MapLight *m);
		friend istream& operator>>(istream& is, MapLight *m);

	private:
		Ogre::Vector3 position;
		Ogre::ColourValue diffuseColor;
		Ogre::ColourValue specularColor;

		double attenuationRange;
		double attenuationConstant;
		double attenuationLinear;
		double attenuationQuadratic;

		string name;
		bool ogreEntityExists;
		bool ogreEntityVisualIndicatorExists;

		Ogre::Vector3 flickerPosition;
		double thetaX;
		double thetaY;
		double thetaZ;
		int factorX;
		int factorY;
		int factorZ;
};

#endif

