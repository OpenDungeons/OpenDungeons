#ifndef CREATURECLASS_H
#define CREATURECLASS_H

#include <string>
#include <iostream>
#include <OgreVector3.h>


#include "Tile.h"

class CreatureDefinition
{
    public:
        enum CreatureJob
        {
            nullCreatureJob = 0,
            basicWorker = 1,
            advancedWorker,
            scout,
            weakFighter,
            weakSpellcaster,
            weakBuilder,
            strongFighter,
            strongSpellcaster,
            strongBuilder,
            guard,
            specialCreature,
            summon,
            superCreature
        };

        CreatureDefinition(
                CreatureJob             job         = nullCreatureJob,
                const std::string&      className   = "",
                const std::string&      meshName    = "",
                const std::string&      bedMeshName = "",
                int                     bedDim1     = 0,
                int                     bedDim2     = 0,
                const Ogre::Vector3&    scale       = Ogre::Vector3(1, 1, 1),
                double                  sightRadius = 0.0,
                double                  digRate     = 10.0,
                double                  danceRate   = 0.35,
                double                  hpPerLevel  = 0.0,
                double                  manaPerLevel= 0.0,
                double                  maxHP       = 10.0,
                double                  maxMana     = 10.0,
                double                  moveSpeed   = 0.0,

                Tile::TileClearType     tilePassability = Tile::walkableTile,

                double coefficientHumans    = 0.0,
                double coefficientCorpars   = 0.0,
                double coefficientUndead    = 0.0,
                double coefficientConstructs= 0.0,
                double coefficientDenizens  = 0.0,
                double coefficientAltruism  = 0.0,
                double coefficientOrder     = 0.0,
                double coefficientPeace     = 0.0
        ) :
            creatureJob (job),
            className   (className),
            meshName    (meshName),
            bedMeshName (bedMeshName),
            bedDim1     (bedDim1),
            bedDim2     (bedDim2),
            scale       (scale),
            sightRadius (sightRadius),
            digRate     (digRate),
            danceRate   (danceRate),
            hpPerLevel  (hpPerLevel),
            manaPerLevel(manaPerLevel),
            maxHP       (maxHP),
            maxMana     (maxMana),
            moveSpeed   (moveSpeed),

            tilePassability         (tilePassability),

            coefficientHumans       (coefficientHumans),
            coefficientCorpars      (coefficientCorpars),
            coefficientUndead       (coefficientUndead),
            coefficientConstructs   (coefficientConstructs),
            coefficientDenizens     (coefficientDenizens),
            coefficientAltruism     (coefficientAltruism),
            coefficientOrder        (coefficientOrder),
            coefficientPeace        (coefficientPeace)
        {}

        static CreatureJob creatureJobFromString(std::string s);
        static std::string creatureJobToString(CreatureJob c);

        inline bool isWorker() const{return (creatureJob == basicWorker || creatureJob == advancedWorker);}

        inline static std::string getFormat()
        {
            return "# className\tcreatureJob\tmeshName\tbedMeshName\tbedDim1\tbedDim2\tscaleX\tscaleY\tscaleZ\thp/level\tmana/level\tsightRadius\tdigRate\tdanceRate\tmoveSpeed\tcHumans\tcCorpars\tcUndead\tcConstructs\tcDenizens\tcAltruism\tcOrder\tcPeace\n";
        }

        friend std::ostream & operator <<(std::ostream & os, CreatureDefinition *c);
        friend std::istream & operator >>(std::istream & is, CreatureDefinition *c);

        inline CreatureJob          getCreatureJob  () const    { return creatureJob; }
        inline int                  getBedDim1      () const    { return bedDim1; }
        inline int                  getBedDim2      () const    { return bedDim2; }
        inline const std::string&   getBedMeshName  () const    { return bedMeshName; }
        inline const std::string&   getClassName    () const    { return className; }
        inline double               getDanceRate    () const    { return danceRate; }
        inline double               getDigRate      () const    { return digRate; }
        inline double               getHpPerLevel   () const    { return hpPerLevel; }
        inline double               getManaPerLevel () const    { return manaPerLevel; }
        inline double               getMaxHp        () const    { return maxHP; }
        inline double               getMaxMana      () const    { return maxMana; }
        inline const std::string&   getMeshName     () const    { return meshName; }
        inline const Ogre::Vector3& getScale        () const    { return scale; }
        inline double               getSightRadius  () const    { return sightRadius; }

        inline Tile::TileClearType  getTilePassability () const { return tilePassability; }

        inline double getCoefficientAltruism    () const    { return coefficientAltruism; }
        inline double getCoefficientConstructs  () const    { return coefficientConstructs; }
        inline double getCoefficientCorpars     () const    { return coefficientCorpars; }
        inline double getCoefficientDenizens    () const    { return coefficientDenizens; }
        inline double getCoefficientHumans      () const    { return coefficientHumans; }
        inline double getCoefficientOrder       () const    { return coefficientOrder; }
        inline double getCoefficientPeace       () const    { return coefficientPeace; }
        inline double getCoefficientUndead      () const    { return coefficientUndead; }

    private:
        //NOTE: Anything added to this class must be included in the '=' operator for the Creature class.

        //! \brief The job of the creature (e.g. worker, fighter, ...)
        CreatureJob creatureJob;

        //! \brief The name of the creatures class
        std::string className;

        //! \brief The name of the model file
        std::string meshName;

        //! \brief The name of the bed model file
        std::string bedMeshName;

        //! \brief size of the bed (x)
        int bedDim1;

        //! \brief size of the bed (y)
        int bedDim2;

        Ogre::Vector3 scale;

        //! \brief The inner radius where the creature sees everything
        double sightRadius;

        //! \brief Fullness removed per turn of digging
        double digRate;

        //! \brief How much the danced upon tile's color changes per turn of dancing
        double danceRate;

        //! \brief How much HP the creature gets per level up
        double hpPerLevel;

        //! \brief How much mana the creature gets per level up
        double manaPerLevel;

        //! \brief The maximum HP the creature can ever have
        double maxHP;

        //! \brief The maximum mana the creature can ever have
        double maxMana;

        //! \brief How fast the creature moves
        double moveSpeed;

        //FIXME:  This is not set from file yet.
        Tile::TileClearType tilePassability;

        //! \brief Probability coefficients to determine how likely a creature is to come through the portal.
        double coefficientHumans;
        double coefficientCorpars;
        double coefficientUndead;
        double coefficientConstructs;
        double coefficientDenizens;
        double coefficientAltruism;
        double coefficientOrder;
        double coefficientPeace;
};

#endif

