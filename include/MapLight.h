#ifndef MAPLIGHT_H
#define MAPLIGHT_H

#include <iostream>
#include <string>
#include <Ogre.h>

#include "ActiveObject.h"

class MapLight
{
    public:
        void initialize();
        MapLight();
        MapLight(Ogre::Vector3 nPosition, Ogre::Real red, Ogre::Real green,
                Ogre::Real blue, Ogre::Real range, Ogre::Real constant, Ogre::Real linear,
                Ogre::Real quadratic);

        void setLocation(Ogre::Vector3 nPosition);
        void setDiffuseColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue);
        void setSpecularColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue);
        void setAttenuation(Ogre::Real range, Ogre::Real constant, Ogre::Real linear,
                Ogre::Real quadratic);

        void createOgreEntity();
        void destroyOgreEntity();
        void destroyOgreEntityVisualIndicator();
        void deleteYourself();

        std::string getName();
        void setPosition(Ogre::Real nX, Ogre::Real nY, Ogre::Real nZ);
        void setPosition(Ogre::Vector3 nPosition);
        Ogre::Vector3 getPosition();
        Ogre::ColourValue getDiffuseColor();
        Ogre::ColourValue getSpecularColor();
        Ogre::Real getAttenuationRange();
        Ogre::Real getAttenuationConstant();
        Ogre::Real getAttenuationLinear();
        Ogre::Real getAttenuationQuadratic();

        void advanceFlicker(Ogre::Real time);

        virtual bool isPermanent();

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, MapLight *m);
        friend std::istream& operator>>(std::istream& is, MapLight *m);

    private:
        Ogre::Vector3 position;
        Ogre::ColourValue diffuseColor;
        Ogre::ColourValue specularColor;

        Ogre::Real attenuationRange;
        Ogre::Real attenuationConstant;
        Ogre::Real attenuationLinear;
        Ogre::Real attenuationQuadratic;

        std::string name;
        bool ogreEntityExists;
        bool ogreEntityVisualIndicatorExists;

        Ogre::Vector3 flickerPosition;
        Ogre::Real thetaX;
        Ogre::Real thetaY;
        Ogre::Real thetaZ;
        int factorX;
        int factorY;
        int factorZ;
};

class TemporaryMapLight: public MapLight, public ActiveObject
{
    public:
        TemporaryMapLight(Ogre::Vector3 nPosition, Ogre::Real red, Ogre::Real green,
                Ogre::Real blue, Ogre::Real range, Ogre::Real constant, Ogre::Real linear,
                Ogre::Real quadratic);
        bool isPermanent();

        bool doUpkeep();

    protected:
        int turnsUntilDestroyed;
        int originalTurnsUntilDestroyed;
};

#endif

