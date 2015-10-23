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

#include "game/ResearchManager.h"

#include "game/Research.h"
#include "game/Seat.h"
#include "modes/GameEditorModeBase.h"
#include "modes/GameMode.h"
#include "render/Gui.h"
#include "rooms/RoomType.h"
#include "spells/SpellType.h"
#include "traps/TrapType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <CEGUI/Window.h>
#include <CEGUI/widgets/PushButton.h>

namespace
{
//! \brief Functor to select researches from gui
class ResearchSelector
{
public:
    ResearchSelector(ResearchType type, GameMode& gameMode):
        mType(type),
        mGameMode(gameMode)
    {
    }

    bool operator()(const CEGUI::EventArgs& e)
    {
        if(!mGameMode.researchButtonTreeClicked(mType))
            return true;

        mGameMode.refreshGuiResearch(true);
        return true;
    }
    ResearchType mType;
    GameMode& mGameMode;
};

ResearchManager& getResearchManager()
{
    static ResearchManager manager;
    return manager;
}
}

//! \brief Class used to gather information about the research with the corresponding
//! research and use gui buttons
class ResearchDef
{
public:
    ResearchDef(const std::string& researchFamily, const std::string& buttonName, const Research* research) :
        mResearchFamily(researchFamily),
        mButtonName(buttonName),
        mResearch(research)
    {
    }

    virtual ~ResearchDef()
    {}

    const std::string mResearchFamily;
    const std::string mButtonName;
    const Research* mResearch;

    virtual ResearchFamily getResearchFamily() const = 0;

    virtual void connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection) const = 0;

    virtual const std::string& getGuiPath() const = 0;

    virtual void mapResearch(std::vector<std::vector<ResearchType>>& researchesFamily) const = 0;
};

class ResearchDefRoom : public ResearchDef
{
public:
    ResearchDefRoom(const std::string& researchFamily, const std::string& buttonName, const Research* research,
            RoomType roomType) :
        ResearchDef(researchFamily, buttonName, research),
        mRoomType(roomType)
    {
    }

    ResearchFamily getResearchFamily() const
    { return ResearchFamily::rooms; }

    void connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection) const override
    {
        mode->addEventConnection(
            rootWindow->getChild(getGuiPath() + mButtonName)->subscribeEvent(
              CEGUI::PushButton::EventClicked,
              CEGUI::Event::Subscriber(RoomSelector(mRoomType, playerSelection))
            )
        );
    }

    const std::string& getGuiPath() const
    {
        static const std::string guiPath = "MainTabControl/Rooms/";
        return guiPath;
    }

    void mapResearch(std::vector<std::vector<ResearchType>>& researchesFamily) const
    {
        uint32_t index = static_cast<uint32_t>(ResearchFamily::rooms);
        std::vector<ResearchType>& family = researchesFamily[index];
        index = static_cast<uint32_t>(mRoomType);
        if(index >= family.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
            return;
        }
        family[index] = mResearch->getType();
    }

    RoomType mRoomType;
};

class ResearchDefTrap : public ResearchDef
{
public:
    ResearchDefTrap(const std::string& researchFamily, const std::string& buttonName, const Research* research,
            TrapType trapType) :
        ResearchDef(researchFamily, buttonName, research),
        mTrapType(trapType)
    {
    }

    ResearchFamily getResearchFamily() const
    { return ResearchFamily::traps; }

    void connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection) const override
    {
        mode->addEventConnection(
            rootWindow->getChild(getGuiPath() + mButtonName)->subscribeEvent(
              CEGUI::PushButton::EventClicked,
              CEGUI::Event::Subscriber(TrapSelector(mTrapType, playerSelection))
            )
        );
    }

    const std::string& getGuiPath() const
    {
        static const std::string guiPath = "MainTabControl/Traps/";
        return guiPath;
    }

    void mapResearch(std::vector<std::vector<ResearchType>>& researchesFamily) const
    {
        uint32_t index = static_cast<uint32_t>(ResearchFamily::traps);
        std::vector<ResearchType>& family = researchesFamily[index];
        index = static_cast<uint32_t>(mTrapType);
        if(index >= family.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
            return;
        }
        family[index] = mResearch->getType();
    }

    TrapType mTrapType;
};

class ResearchDefSpell : public ResearchDef
{
public:
    ResearchDefSpell(const std::string& researchFamily, const std::string& buttonName, const Research* research,
            SpellType spellType) :
        ResearchDef(researchFamily, buttonName, research),
        mSpellType(spellType)
    {
    }

    ResearchFamily getResearchFamily() const
    { return ResearchFamily::spells; }

    void connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection) const override
    {
        mode->addEventConnection(
            rootWindow->getChild(getGuiPath() + mButtonName)->subscribeEvent(
              CEGUI::PushButton::EventClicked,
              CEGUI::Event::Subscriber(SpellSelector(mSpellType, playerSelection))
            )
        );
    }

    const std::string& getGuiPath() const
    {
        static const std::string guiPath = "MainTabControl/Spells/";
        return guiPath;
    }

    void mapResearch(std::vector<std::vector<ResearchType>>& researchesFamily) const
    {
        uint32_t index = static_cast<uint32_t>(ResearchFamily::spells);
        std::vector<ResearchType>& family = researchesFamily[index];
        index = static_cast<uint32_t>(mSpellType);
        if(index >= family.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
            return;
        }
        family[index] = mResearch->getType();
    }

    SpellType mSpellType;
};

ResearchManager::ResearchManager() :
    mResearches(static_cast<uint32_t>(ResearchType::countResearch)),
    mResearchesFamily(static_cast<uint32_t>(ResearchFamily::nb))
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(ResearchFamily::nb); ++i)
    {
        std::vector<ResearchType>& family = mResearchesFamily.at(i);
        ResearchFamily resFamily = static_cast<ResearchFamily>(i);
        switch(resFamily)
        {
            case ResearchFamily::rooms:
                family = std::vector<ResearchType>(static_cast<uint32_t>(RoomType::nbRooms), ResearchType::nullResearchType);
                break;
            case ResearchFamily::traps:
                family = std::vector<ResearchType>(static_cast<uint32_t>(TrapType::nbTraps), ResearchType::nullResearchType);
                break;
            case ResearchFamily::spells:
                family = std::vector<ResearchType>(static_cast<uint32_t>(SpellType::nbSpells), ResearchType::nullResearchType);
                break;
            default:
                OD_LOG_ERR("Wrong enum value=" + Helper::toString(static_cast<uint32_t>(resFamily)) + ", size=" + Helper::toString(static_cast<uint32_t>(ResearchFamily::nb)));
                break;
        }
    }

    // We build the research tree
    ResearchDef* def;
    ResearchType resType;
    int32_t points;
    std::vector<const Research*> emptyDepends;
    std::vector<const Research*> lvl1depends;
    std::vector<const Research*> lvl2depends;
    std::vector<const Research*> lvl3depends;
    Research* research;
    uint32_t index;

    // Attack Skills

    // Lvl 1 researches
    resType = ResearchType::roomTrainingHall;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, emptyDepends);
    def = new ResearchDefRoom("AttackSkills/", "TrainingHallButton", research, RoomType::trainingHall);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl1depends.push_back(research);

    resType = ResearchType::roomBridgeWooden;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, emptyDepends);
    def = new ResearchDefRoom("AttackSkills/", "WoodenBridgeButton", research, RoomType::bridgeWooden);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl1depends.push_back(research);

    // Lvl 2 researches
    resType = ResearchType::spellCallToWar;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl1depends);
    def = new ResearchDefSpell("AttackSkills/", "CallToWarButton", research, SpellType::callToWar);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl2depends.push_back(research);

    // CreatureExplosion depends on Training Hall
    resType = ResearchType::spellCreatureExplosion;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl1depends);
    def = new ResearchDefSpell("AttackSkills/", "CreatureExplosionButton", research, SpellType::creatureExplosion);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl2depends.push_back(research);

    resType = ResearchType::roomPrison;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl1depends);
    def = new ResearchDefRoom("AttackSkills/", "PrisonButton", research, RoomType::prison);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl2depends.push_back(research);

    // Lvl 3 researches
    resType = ResearchType::roomBridgeStone;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl2depends);
    def = new ResearchDefRoom("AttackSkills/", "StoneBridgeButton", research, RoomType::bridgeStone);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl3depends.push_back(research);

    // Tech Skills
    lvl1depends.clear();
    lvl2depends.clear();
    lvl3depends.clear();

    resType = ResearchType::roomTreasury;
    index = static_cast<uint32_t>(resType);
    research = new Research(resType, 0, emptyDepends);
    def = new ResearchDefRoom("TacticSkills/", "TreasuryButton", research, RoomType::treasury);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl1depends.push_back(research);

    resType = ResearchType::roomHatchery;
    index = static_cast<uint32_t>(resType);
    research = new Research(resType, 0, emptyDepends);
    def = new ResearchDefRoom("TacticSkills/", "HatcheryButton", research, RoomType::hatchery);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl1depends.push_back(research);

    resType = ResearchType::roomDormitory;
    index = static_cast<uint32_t>(resType);
    research = new Research(resType, 0, emptyDepends);
    def = new ResearchDefRoom("TacticSkills/", "DormitoryButton", research, RoomType::dormitory);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl1depends.push_back(research);

    // Lvl 2 researches
    resType = ResearchType::roomWorkshop;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl1depends);
    def = new ResearchDefRoom("TacticSkills/", "WorkshopButton", research, RoomType::workshop);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl2depends.push_back(research);

    resType = ResearchType::trapDoorWooden;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl1depends);
    def = new ResearchDefTrap("TacticSkills/", "WoodenDoorTrapButton", research, TrapType::doorWooden);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl2depends.push_back(research);

    // Lvl 3 researches
    resType = ResearchType::trapCannon;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl2depends);
    def = new ResearchDefTrap("TacticSkills/", "CannonButton", research, TrapType::cannon);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl3depends.push_back(research);

    resType = ResearchType::trapSpike;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl2depends);
    def = new ResearchDefTrap("TacticSkills/", "SpikeTrapButton", research, TrapType::spike);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl3depends.push_back(research);

    // Lvl 4 research
    resType = ResearchType::trapBoulder;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl3depends);
    def = new ResearchDefTrap("TacticSkills/", "BoulderTrapButton", research, TrapType::boulder);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;

    // Magic Skills
    lvl1depends.clear();
    lvl2depends.clear();
    lvl3depends.clear();

    // Lvl 1 researches
    resType = ResearchType::roomLibrary;
    research = new Research(resType, 0, emptyDepends);
    index = static_cast<uint32_t>(resType);
    def = new ResearchDefRoom("MagicSkills/", "LibraryButton", research, RoomType::library);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl1depends.push_back(research);

    resType = ResearchType::spellSummonWorker;
    index = static_cast<uint32_t>(resType);
    research = new Research(resType, 0, emptyDepends);
    def = new ResearchDefSpell("MagicSkills/", "SummonWorkerButton", research, SpellType::summonWorker);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl1depends.push_back(research);

    // Lvl 2 researches
    resType = ResearchType::roomCrypt;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl1depends);
    def = new ResearchDefRoom("MagicSkills/", "CryptButton", research, RoomType::crypt);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl2depends.push_back(research);

    // Lvl 2 researches
    resType = ResearchType::spellCreatureHeal;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl1depends);
    def = new ResearchDefSpell("MagicSkills/", "CreatureHealButton", research, SpellType::creatureHeal);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl2depends.push_back(research);

    // Lvl 3 researches
    resType = ResearchType::spellCreatureHaste;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl2depends);
    def = new ResearchDefSpell("MagicSkills/", "CreatureHasteButton", research, SpellType::creatureHaste);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl3depends.push_back(research);

    resType = ResearchType::spellCreatureDefense;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl2depends);
    def = new ResearchDefSpell("MagicSkills/", "CreatureDefenseButton", research, SpellType::creatureDefense);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl3depends.push_back(research);

    resType = ResearchType::spellCreatureSlow;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getResearchPoints(Research::researchTypeToString(resType));
    research = new Research(resType, points, lvl2depends);
    def = new ResearchDefSpell("MagicSkills/", "CreatureSlowButton", research, SpellType::creatureSlow);
    def->mapResearch(mResearchesFamily);
    mResearches[index] = def;
    lvl3depends.push_back(research);
}

ResearchManager::~ResearchManager()
{
    for(const ResearchDef* research : mResearches)
    {
        if(research == nullptr)
            continue;

        delete research->mResearch;
        delete research;
    }
    mResearches.clear();
}

bool ResearchManager::isRoomAvailable(RoomType type, const Seat* seat)
{
    uint32_t index = static_cast<uint32_t>(ResearchFamily::rooms);
    std::vector<ResearchType>& family = getResearchManager().mResearchesFamily[index];
    index = static_cast<uint32_t>(type);
    if(index >= family.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
        return false;
    }
    ResearchType resType = family.at(index);
    return seat->isResearchDone(resType);
}

bool ResearchManager::isSpellAvailable(SpellType type, const Seat* seat)
{
    uint32_t index = static_cast<uint32_t>(ResearchFamily::spells);
    std::vector<ResearchType>& family = getResearchManager().mResearchesFamily[index];
    index = static_cast<uint32_t>(type);
    if(index >= family.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
        return false;
    }
    ResearchType resType = family.at(index);
    return seat->isResearchDone(resType);
}

bool ResearchManager::isTrapAvailable(TrapType type, const Seat* seat)
{
    uint32_t index = static_cast<uint32_t>(ResearchFamily::traps);
    std::vector<ResearchType>& family = getResearchManager().mResearchesFamily[index];
    index = static_cast<uint32_t>(type);
    if(index >= family.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
        return false;
    }
    ResearchType resType = family.at(index);
    return seat->isResearchDone(resType);
}

bool ResearchManager::isAllResearchesDoneForSeat(const Seat* seat)
{
    if(seat->isResearching())
        return false;

    for(const ResearchDef* research : getResearchManager().mResearches)
    {
        if(research == nullptr)
            continue;
        if(seat->isResearchDone(research->mResearch->getType()))
            continue;
        if(!research->mResearch->canBeResearched(seat->getResearchDone()))
            continue;
        return false;
    }
    return true;
}

const Research* ResearchManager::getResearch(ResearchType resType)
{
    uint32_t index = static_cast<uint32_t>(resType);
    if(index >= getResearchManager().mResearches.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(getResearchManager().mResearches.size()));
        return nullptr;
    }

    const ResearchDef* research = getResearchManager().mResearches[index];
    if(research == nullptr)
    {
        OD_LOG_ERR("null research index=" + Helper::toString(index));
        return nullptr;
    }
    return research->mResearch;
}

void ResearchManager::buildRandomPendingResearchesForSeat(std::vector<ResearchType>& researches,
        const Seat* seat)
{
    const std::vector<ResearchType>& researchNotAllowed = seat->getResearchNotAllowed();
    std::vector<const ResearchDef*> availableResearches;
    // We consider as done researches already done for the given seat as well as the researches
    // already pending
    std::vector<ResearchType> doneResearches = seat->getResearchDone();
    doneResearches.insert(doneResearches.begin(), researches.begin(), researches.end());
    // We fill availableResearches with all the researches that are not already done and
    // that can be researched
    for(const ResearchDef* research : getResearchManager().mResearches)
    {
        if(research == nullptr)
            continue;

        ResearchType resType = research->mResearch->getType();
        // We do not consider researches already pending or done
        if(std::find(doneResearches.begin(), doneResearches.end(), resType) != doneResearches.end())
            continue;

        if(research->mResearch->dependsOn(researchNotAllowed))
            continue;

        availableResearches.push_back(research);
        doneResearches.push_back(resType);
    }

    // Now, availableResearches only contains researches that can be done (but unsorted).
    // We need to shuffle that and fill researches.
    doneResearches = seat->getResearchDone();
    std::random_shuffle(availableResearches.begin(), availableResearches.end());
    for(const ResearchDef* research : availableResearches)
    {
        // Since buildDependencies guarantees to not add duplicate researches, it is safe
        // to call it for every research not already done
        research->mResearch->buildDependencies(doneResearches, researches);
    }
}

void ResearchManager::listAllResearches(const std::function<void(const std::string&, const std::string&,
        const std::string&, ResearchType)>& func)
{
    for(const ResearchDef* research : getResearchManager().mResearches)
    {
        if(research == nullptr)
            continue;

        func(research->mResearchFamily + research->mButtonName, research->getGuiPath() + research->mButtonName,
             research->mResearchFamily + research->mButtonName + "/" + research->mButtonName + "ProgressBar",
             research->mResearch->getType());
    }
}

void ResearchManager::listAllSpellsProgressBars(const std::function<void(SpellType spellType, const std::string&)>& func)
{
    std::vector<const ResearchDef*>& researches = getResearchManager().mResearches;
    uint32_t index = static_cast<uint32_t>(ResearchFamily::spells);
    std::vector<ResearchType>& family = getResearchManager().mResearchesFamily.at(index);
    uint32_t nbSpells = static_cast<uint32_t>(SpellType::nbSpells);
    for(uint32_t i = 0; i < nbSpells; ++i)
    {
        if(i >= family.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(i) + ", size=" + Helper::toString(family.size()));
            return;
        }

        ResearchType resType = family[i];
        if(resType == ResearchType::nullResearchType)
            continue;

        index = static_cast<uint32_t>(resType);
        if(index >= researches.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(getResearchManager().mResearches.size()));
            continue;
        }

        const ResearchDef* research = researches[index];
        if(research == nullptr)
        {
            OD_LOG_ERR("null research index=" + Helper::toString(index));
            continue;
        }

        SpellType spellType = static_cast<SpellType>(i);
        func(spellType, research->getGuiPath() + research->mButtonName + "/" + research->mButtonName + "ProgressBar");
    }
}

void ResearchManager::connectResearches(GameMode* mode, CEGUI::Window* rootWindow)
{
    for(const ResearchDef* research : getResearchManager().mResearches)
    {
        if(research == nullptr)
            continue;

        mode->addEventConnection(
            rootWindow->getChild("ResearchTreeWindow/Skills/" + research->mResearchFamily + research->mButtonName)->subscribeEvent(
              CEGUI::PushButton::EventClicked,
              CEGUI::Event::Subscriber(ResearchSelector(research->mResearch->getType(), *mode))
            )
        );
    }
}

void ResearchManager::connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection)
{
    for(const ResearchDef* research : getResearchManager().mResearches)
    {
        if(research == nullptr)
            continue;

        research->connectGuiButtons(mode, rootWindow, playerSelection);
    }
}
