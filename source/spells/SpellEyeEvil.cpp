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

#include "spells/SpellEyeEvil.h"

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

const std::string SpellEyeEvilName = "eyeEvil";
const std::string SpellEyeEvilNameDisplay = "Eye of Evil";
const std::string SpellEyeEvilCooldownKey = "EyeEvilCooldown";
const SpellType SpellEyeEvil::mSpellType = SpellType::eyeEvil;

namespace
{
class SpellEyeEvilFactory : public SpellFactory
{
    SpellType getSpellType() const override
    { return SpellEyeEvil::mSpellType; }

    const std::string& getName() const override
    { return SpellEyeEvilName; }

    const std::string& getCooldownKey() const override
    { return SpellEyeEvilCooldownKey; }

    const std::string& getNameReadable() const override
    { return SpellEyeEvilNameDisplay; }

    virtual void checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    { SpellEyeEvil::checkSpellCast(gameMap, inputManager, inputCommand); }

    virtual bool castSpell(GameMap* gameMap, Player* player, ODPacket& packet) const override
    { return SpellEyeEvil::castSpell(gameMap, player, packet); }

    Spell* getSpellFromStream(GameMap* gameMap, std::istream &is) const override
    { return SpellEyeEvil::getSpellFromStream(gameMap, is); }

    Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is) const override
    { return SpellEyeEvil::getSpellFromPacket(gameMap, is); }
};

// Register the factory
static SpellRegister reg(new SpellEyeEvilFactory);
}

SpellEyeEvil::SpellEyeEvil(GameMap* gameMap) :
    Spell(gameMap, SpellManager::getSpellNameFromSpellType(getSpellType()), "FlyingSkull", 0.0,
        ConfigManager::getSingleton().getSpellConfigInt32("EyeEvilNbTurns"))
{
    mPrevAnimationState = "Triggered";
    mPrevAnimationStateLoop = true;
}

SpellEyeEvil::~SpellEyeEvil()
{
}

void SpellEyeEvil::computeVisibleTiles()
{
    uint32_t radius = ConfigManager::getSingleton().getSpellConfigUInt32("EyeEvilRadiusTiles");
    Tile* posTile = getPositionTile();
    if(posTile == nullptr)
    {
        OD_LOG_ERR("name=" + getName() + ", position=" + Helper::toString(getPosition()));
        return;
    }

    std::vector<Tile*> tiles = getGameMap()->circularRegion(posTile->getX(), posTile->getY(), radius);
    for(Tile* tile : tiles)
        tile->notifyVision(getSeat());
}

void SpellEyeEvil::checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();

    Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
    if(tile == nullptr)
        return;

    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    int32_t price = ConfigManager::getSingleton().getSpellConfigInt32("EyeEvilPrice");
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerMana < price)
        {
            std::string txt = formatCastSpell(SpellType::eyeEvil, price);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatCastSpell(SpellType::eyeEvil, price);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        std::string txt = formatCastSpell(SpellType::eyeEvil, price);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        std::vector<Tile*> tiles;
        tiles.push_back(tile);
        inputCommand.selectTiles(tiles);
        return;
    }

    inputCommand.unselectAllTiles();

    ClientNotification *clientNotification = SpellManager::createSpellClientNotification(SpellType::eyeEvil);
    gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool SpellEyeEvil::castSpell(GameMap* gameMap, Player* player, ODPacket& packet)
{
    Tile* tile = gameMap->tileFromPacket(packet);
    if(tile == nullptr)
        return false;

    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    int32_t manaCost = ConfigManager::getSingleton().getSpellConfigInt32("EyeEvilPrice");
    if(playerMana < manaCost)
        return false;

    if(!player->getSeat()->takeMana(manaCost))
        return false;

    SpellEyeEvil* spell = new SpellEyeEvil(gameMap);
    spell->setSeat(player->getSeat());
    spell->addToGameMap();
    Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                static_cast<Ogre::Real>(tile->getY()),
                                static_cast<Ogre::Real>(3.0));
    spell->createMesh();
    spell->setPosition(spawnPosition);

    return true;
}

Spell* SpellEyeEvil::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    SpellEyeEvil* spell = new SpellEyeEvil(gameMap);
    spell->importFromStream(is);
    return spell;
}

Spell* SpellEyeEvil::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    SpellEyeEvil* spell = new SpellEyeEvil(gameMap);
    spell->importFromPacket(is);
    return spell;
}

