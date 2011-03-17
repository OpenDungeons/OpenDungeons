#ifndef MAPLIGHT_H
#define MAPLIGHT_H

#include <iostream>
#include <string>
using namespace std;

#include <Ogre.h>

class MapLight
{
    public:
        void initialize();
        MapLight();
        MapLight(Ogre::Vector3 nPosition, double red, double green,
                double blue, double range, double constant, double linear,
                double quadratic);

        void setLocation(Ogre::Vector3 nPosition);
        void setDiffuseColor(double red, double green, double blue);
        void setSpecularColor(double red, double green, double blue);
        void setAttenuation(double range, double constant, double linear,
                double quadratic);

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

        virtual bool isPermanent();

        static string getFormat();
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

class TemporaryMapLight: public MapLight, public ActiveObject
{
    public:
        TemporaryMapLight(Ogre::Vector3 nPosition, double red, double green,
                double blue, double range, double constant, double linear,
                double quadratic);
        bool isPermanent();

        bool doUpkeep();

    protected:
        int turnsUntilDestroyed;
        int originalTurnsUntilDestroyed;
};

#endif

