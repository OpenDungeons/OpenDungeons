#ifndef MAPLIGHT_H
#define MAPLIGHT_H

#include <iostream>
#include <string>
#include <Ogre.h>

class MapLight
{
    public:
        MapLight( const Ogre::Vector3&  nPosition   = Ogre::Vector3(0, 0, 0),
                  Ogre::Real            red         = 0,
                  Ogre::Real            green       = 0,
                  Ogre::Real            blue        = 0,
                  Ogre::Real            range       = 0,
                  Ogre::Real            constant    = 0,
                  Ogre::Real            linear      = 0,
                  Ogre::Real            quadratic   = 0
                ) :
            ogreEntityExists                (false),
            ogreEntityVisualIndicatorExists (false),
            thetaX                          (0.0),
            thetaY                          (0.0),
            thetaZ                          (0.0),
            factorX                         (0.1),
            factorY                         (0.1),
            factorZ                         (0.1)
        {
            static unsigned int lightNumber = 0;

            std::stringstream tempSS;
            sem_wait(&lightNumberLockSemaphore);
            tempSS << "Map_light_ " << ++lightNumber;
            sem_post(&lightNumberLockSemaphore);
            name = tempSS.str();

            setPosition(nPosition);
            setDiffuseColor(red, green, blue);
            setSpecularColor(red, green, blue);
            setAttenuation(range, constant, linear, quadratic);
        }

        virtual ~MapLight() {}

        void setLocation(const Ogre::Vector3& nPosition);
        void setDiffuseColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue);
        void setSpecularColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue);
        void setAttenuation(Ogre::Real range, Ogre::Real constant, Ogre::Real linear,
                Ogre::Real quadratic);

        void createOgreEntity();
        void destroyOgreEntity();
        void destroyOgreEntityVisualIndicator();
        void deleteYourself();

        const std::string& getName() const;
        void setPosition(Ogre::Real nX, Ogre::Real nY, Ogre::Real nZ);
        void setPosition(const Ogre::Vector3& nPosition);
        const Ogre::Vector3& getPosition() const;
        const Ogre::ColourValue& getDiffuseColor() const;
        const Ogre::ColourValue& getSpecularColor() const;
        Ogre::Real getAttenuationRange() const;
        Ogre::Real getAttenuationConstant() const;
        Ogre::Real getAttenuationLinear() const;
        Ogre::Real getAttenuationQuadratic() const;

        void advanceFlicker(Ogre::Real time);

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, MapLight *m);
        friend std::istream& operator>>(std::istream& is, MapLight *m);

        static sem_t lightNumberLockSemaphore;

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

#endif

