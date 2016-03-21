/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "spells/SpellCallToWar.h"

#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "spells/SpellManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string SpellCallToWarName = "callToWar";
const std::string SpellCallToWarNameDisplay = "Call to war";
const std::string SpellCallToWarCooldownKey = "CallToWarCooldown";
const SpellType SpellCallToWar::mSpellType = SpellType::callToWar;

namespace
{
class SpellCallToWarFactory : public SpellFactory
{
    SpellType getSpellType() const override
    { return SpellCallToWar::mSpellType; }

    const std::string& getName() const override
    { return SpellCallToWarName; }

    const std::string& getCooldownKey() const override
    { return SpellCallToWarCooldownKey; }

    const std::string& getNameReadable() const override
    { return SpellCallToWarNameDisplay; }

    virtual void checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    { SpellCallToWar::checkSpellCast(gameMap, inputManager, inputCommand); }

    virtual bool castSpell(GameMap* gameMap, Player* player, ODPacket& packet) const override
    { return SpellCallToWar::castSpell(gameMap, player, packet); }

    Spell* getSpellFromStream(GameMap* gameMap, std::istream &is) const override
    { return SpellCallToWar::getSpellFromStream(gameMap, is); }

    Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is) const override
    { return SpellCallToWar::getSpellFromPacket(gameMap, is); }
};

// Register the factory
static SpellRegister reg(new SpellCallToWarFactory);
}

SpellCallToWar::SpellCallToWar(GameMap* gameMap) :
    Spell(gameMap, SpellManager::getSpellNameFromSpellType(getSpellType()), "WarBanner", 0.0,
        ConfigManager::getSingleton().getSpellConfigInt32("CallToWarNbTurnsMax"))
{
    mPrevAnimationState = "Loop";
    mPrevAnimationStateLoop = true;
}

SpellCallToWar::~SpellCallToWar()
{
}

bool SpellCallToWar::canSlap(Seat* seat)
{
    // Only the spell caster can slap the spell
    if(getSeat() != seat)
        return false;

    if(!getIsOnMap())
        return false;

    return true;
}

void SpellCallToWar::slap()
{
    if(!getIsOnServerMap())
        return;

    removeFromGameMap();
    deleteYourself();
}

void SpellCallToWar::checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();

    Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
    if(tile == nullptr)
        return;

    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    int32_t price = ConfigManager::getSingleton().getSpellConfigInt32("CallToWarPrice");
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerMana < price)
        {
            std::string txt = formatCastSpell(SpellType::callToWar, price);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatCastSpell(SpellType::callToWar, price);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        std::string txt = formatCastSpell(SpellType::callToWar, price);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        std::vector<Tile*> tiles;
        tiles.push_back(tile);
        inputCommand.selectTiles(tiles);
        return;
    }

    inputCommand.unselectAllTiles();

    ClientNotification *clientNotification = SpellManager::createSpellClientNotification(SpellType::callToWar);
    gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool SpellCallToWar::castSpell(GameMap* gameMap, Player* player, ODPacket& packet)
{
    Tile* tile = gameMap->tileFromPacket(packet);
    if(tile == nullptr)
        return false;

    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    int32_t manaCost = ConfigManager::getSingleton().getSpellConfigInt32("CallToWarPrice");
    if(playerMana < manaCost)
        return false;

    if(!player->getSeat()->takeMana(manaCost))
        return false;

    SpellCallToWar* spell = new SpellCallToWar(gameMap);
    spell->setSeat(player->getSeat());
    spell->addToGameMap();
    Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                static_cast<Ogre::Real>(tile->getY()),
                                static_cast<Ogre::Real>(0.0));
    spell->createMesh();
    spell->setPosition(spawnPosition);

    return true;
}

Spell* SpellCallToWar::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    SpellCallToWar* spell = new SpellCallToWar(gameMap);
    spell->importFromStream(is);
    return spell;
}

Spell* SpellCallToWar::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    SpellCallToWar* spell = new SpellCallToWar(gameMap);
    spell->importFromPacket(is);
    return spell;
}

