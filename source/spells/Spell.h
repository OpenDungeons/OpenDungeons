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

enum class SpellType;

/*! \class Spell
 *  \brief Defines a spell. To be usable in game, a spell must register by creating a global class SpellManagerRegister.
 *         A Spell have to have the following static functions (otherwise, it won't be registrable):
 *         - getSpellCost
 *         - castSpell
 *         - getSpellFromStream
 *         - getSpellFromPacket
 */
class Spell : public RenderedMovableEntity
{
public:
    Spell(GameMap* gameMap, const std::string& baseName, const std::string& meshName, Ogre::Real rotationAngle,
        int32_t nbTurns);
    virtual ~Spell()
    {}

    virtual GameEntityType getObjectType() const
    { return GameEntityType::spell; }

    virtual SpellType getSpellType() const = 0;

    virtual void addToGameMap() override;
    virtual void removeFromGameMap() override;

    //! \brief Some spells can be cast where the caster do not have vision. In this case, we
    //! want him and his allies to see the spell even if they don't have vision on the tile
    //! where the spell is
    virtual void notifySeatsWithVision(const std::vector<Seat*>& seats);

    virtual void doUpkeep();

    /*! \brief Exports the headers needed to recreate the Spell. It allows to extend Spells as much as wanted.
     * The content of the Spell will be exported by exportToPacket.
     */
    virtual void exportHeadersToStream(std::ostream& os) const override;
    virtual void exportHeadersToPacket(ODPacket& os) const override;

    static std::string getSpellStreamFormat();

private:
    //! \brief Number of turns the spell should be displayed before automatic deletion.
    //! If < 0, the Spell will not be removed automatically
    int32_t mNbTurns;
};

#endif // SPELL_H
