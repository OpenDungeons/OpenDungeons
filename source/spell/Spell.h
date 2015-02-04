/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPELL_H
#define SPELL_H

#include "entities/RenderedMovableEntity.h"

class GameMap;
class ODPacket;

enum class SpellType
{
    nullSpellType = 0,
    summonWorker,
    callToWar
};

std::istream& operator>>(std::istream& is, SpellType& tt);
std::ostream& operator<<(std::ostream& os, const SpellType& tt);
ODPacket& operator>>(ODPacket& is, SpellType& tt);
ODPacket& operator<<(ODPacket& os, const SpellType& tt);

/*! \class Spell
 *  \brief Defines a spell
 */
class Spell : public RenderedMovableEntity
{
public:
    Spell(GameMap* gameMap, const std::string& baseName, const std::string& meshName, Ogre::Real rotationAngle,
        int32_t nbTurns, const std::string& initialAnimationState = "", bool initialAnimationLoop = true);
    virtual ~Spell()
    {}

    virtual ObjectType getObjectType() const
    { return ObjectType::spell; }

    static const std::string SPELL_OGRE_PREFIX;

    virtual std::string getOgreNamePrefix() const
    { return SPELL_OGRE_PREFIX; }

    RenderedMovableEntityType getRenderedMovableEntityType()
    { return RenderedMovableEntityType::spellEntity; }

    static Spell* getSpellFromStream(GameMap* gameMap, std::istream &is);
    static Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is);

    virtual SpellType getSpellType() const = 0;

    virtual void addToGameMap();
    virtual void removeFromGameMap();

    static std::string getSpellNameFromSpellType(SpellType t);

    static int getSpellCost(GameMap* gameMap, SpellType type, const std::vector<Tile*>& tiles, Player* player);

    static void castSpell(GameMap* gameMap, SpellType type, const std::vector<Tile*>& tiles, Player* player);

    // Functions which can be overridden by child classes.
    virtual void doUpkeep();

    /*! \brief Exports the headers needed to recreate the Spell. It allows to extend Spells as much as wanted.
     * The content of the Spell will be exported by exportToPacket.
     */
    virtual void exportHeadersToStream(std::ostream& os);
    virtual void exportHeadersToPacket(ODPacket& os);
    //! \brief Exports the data of the Spell
    virtual void exportToStream(std::ostream& os) const;
    virtual void importFromStream(std::istream& is);
    virtual void exportToPacket(ODPacket& os) const;
    virtual void importFromPacket(ODPacket& is);

    static std::string getFormat();

private:
    //! \brief Number of turns the spell should be displayed before automatic deletion.
    //! If < 0, the Spell will not be removed automatically
    int32_t mNbTurns;
};

#endif // SPELL_H
