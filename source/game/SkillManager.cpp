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

#include "game/SkillManager.h"

#include "game/Skill.h"
#include "game/Seat.h"
#include "game/SkillType.h"
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
//! \brief Functor to select skills from gui
class SkillSelector
{
public:
    SkillSelector(SkillType type, GameMode& gameMode):
        mType(type),
        mGameMode(gameMode)
    {
    }

    bool operator()(const CEGUI::EventArgs& e)
    {
        if(!mGameMode.skillButtonTreeClicked(mType))
            return true;

        mGameMode.refreshGuiSkill(true);
        return true;
    }
    SkillType mType;
    GameMode& mGameMode;
};

SkillManager& getSkillManager()
{
    static SkillManager manager;
    return manager;
}
}

//! \brief Class used to gather information about the skill with the corresponding
//! skill and use gui buttons
class SkillDef
{
public:
    SkillDef(const std::string& skillFamily, const std::string& buttonName, const Skill* skill) :
        mSkillFamily(skillFamily),
        mButtonName(buttonName),
        mSkill(skill)
    {
    }

    virtual ~SkillDef()
    {}

    const std::string mSkillFamily;
    const std::string mButtonName;
    const Skill* mSkill;

    virtual SkillFamily getSkillFamily() const = 0;

    virtual void connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection) const = 0;

    virtual const std::string& getGuiPath() const = 0;

    virtual void mapSkill(std::vector<std::vector<SkillType>>& skillsFamily) const = 0;
};

class SkillDefRoom : public SkillDef
{
public:
    SkillDefRoom(const std::string& skillFamily, const std::string& buttonName, const Skill* skill,
            RoomType roomType) :
        SkillDef(skillFamily, buttonName, skill),
        mRoomType(roomType)
    {
    }

    SkillFamily getSkillFamily() const
    { return SkillFamily::rooms; }

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

    void mapSkill(std::vector<std::vector<SkillType>>& skillsFamily) const
    {
        uint32_t index = static_cast<uint32_t>(SkillFamily::rooms);
        std::vector<SkillType>& family = skillsFamily[index];
        index = static_cast<uint32_t>(mRoomType);
        if(index >= family.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
            return;
        }
        family[index] = mSkill->getType();
    }

    RoomType mRoomType;
};

class SkillDefTrap : public SkillDef
{
public:
    SkillDefTrap(const std::string& skillFamily, const std::string& buttonName, const Skill* skill,
            TrapType trapType) :
        SkillDef(skillFamily, buttonName, skill),
        mTrapType(trapType)
    {
    }

    SkillFamily getSkillFamily() const
    { return SkillFamily::traps; }

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

    void mapSkill(std::vector<std::vector<SkillType>>& skillsFamily) const
    {
        uint32_t index = static_cast<uint32_t>(SkillFamily::traps);
        std::vector<SkillType>& family = skillsFamily[index];
        index = static_cast<uint32_t>(mTrapType);
        if(index >= family.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
            return;
        }
        family[index] = mSkill->getType();
    }

    TrapType mTrapType;
};

class SkillDefSpell : public SkillDef
{
public:
    SkillDefSpell(const std::string& skillFamily, const std::string& buttonName, const Skill* skill,
            SpellType spellType) :
        SkillDef(skillFamily, buttonName, skill),
        mSpellType(spellType)
    {
    }

    SkillFamily getSkillFamily() const
    { return SkillFamily::spells; }

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

    void mapSkill(std::vector<std::vector<SkillType>>& skillsFamily) const
    {
        uint32_t index = static_cast<uint32_t>(SkillFamily::spells);
        std::vector<SkillType>& family = skillsFamily[index];
        index = static_cast<uint32_t>(mSpellType);
        if(index >= family.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
            return;
        }
        family[index] = mSkill->getType();
    }

    SpellType mSpellType;
};

SkillManager::SkillManager() :
    mSkills(static_cast<uint32_t>(SkillType::countSkill)),
    mSkillsFamily(static_cast<uint32_t>(SkillFamily::nb))
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(SkillFamily::nb); ++i)
    {
        std::vector<SkillType>& family = mSkillsFamily.at(i);
        SkillFamily resFamily = static_cast<SkillFamily>(i);
        switch(resFamily)
        {
            case SkillFamily::rooms:
                family = std::vector<SkillType>(static_cast<uint32_t>(RoomType::nbRooms), SkillType::nullSkillType);
                break;
            case SkillFamily::traps:
                family = std::vector<SkillType>(static_cast<uint32_t>(TrapType::nbTraps), SkillType::nullSkillType);
                break;
            case SkillFamily::spells:
                family = std::vector<SkillType>(static_cast<uint32_t>(SpellType::nbSpells), SkillType::nullSkillType);
                break;
            default:
                OD_LOG_ERR("Wrong enum value=" + Helper::toString(static_cast<uint32_t>(resFamily)) + ", size=" + Helper::toString(static_cast<uint32_t>(SkillFamily::nb)));
                break;
        }
    }

    // We build the skill tree
    SkillDef* def;
    SkillType resType;
    int32_t points;
    std::vector<const Skill*> emptyDepends;
    std::vector<const Skill*> lvl1depends;
    std::vector<const Skill*> lvl2depends;
    std::vector<const Skill*> lvl3depends;
    std::vector<const Skill*> lvl4depends;
    Skill* skill;
    uint32_t index;

    // Attack Skills

    // Lvl 1 skills
    resType = SkillType::roomTrainingHall;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, emptyDepends);
    def = new SkillDefRoom("AttackSkills/", "TrainingHallButton", skill, RoomType::trainingHall);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl1depends.push_back(skill);

    resType = SkillType::roomBridgeWooden;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, emptyDepends);
    def = new SkillDefRoom("AttackSkills/", "WoodenBridgeButton", skill, RoomType::bridgeWooden);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl1depends.push_back(skill);

    // Lvl 2 skills
    resType = SkillType::spellCallToWar;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl1depends);
    def = new SkillDefSpell("AttackSkills/", "CallToWarButton", skill, SpellType::callToWar);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl2depends.push_back(skill);

    // CreatureExplosion depends on Training Hall
    resType = SkillType::spellCreatureExplosion;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl1depends);
    def = new SkillDefSpell("AttackSkills/", "CreatureExplosionButton", skill, SpellType::creatureExplosion);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl2depends.push_back(skill);

    resType = SkillType::roomPrison;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl1depends);
    def = new SkillDefRoom("AttackSkills/", "PrisonButton", skill, RoomType::prison);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl2depends.push_back(skill);

    // Lvl 3 skills
    resType = SkillType::roomBridgeStone;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl2depends);
    def = new SkillDefRoom("AttackSkills/", "StoneBridgeButton", skill, RoomType::bridgeStone);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl3depends.push_back(skill);

    // Tech Skills
    lvl1depends.clear();
    lvl2depends.clear();
    lvl3depends.clear();

    resType = SkillType::roomTreasury;
    index = static_cast<uint32_t>(resType);
    skill = new Skill(resType, 0, emptyDepends);
    def = new SkillDefRoom("TacticSkills/", "TreasuryButton", skill, RoomType::treasury);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl1depends.push_back(skill);

    resType = SkillType::roomHatchery;
    index = static_cast<uint32_t>(resType);
    skill = new Skill(resType, 0, emptyDepends);
    def = new SkillDefRoom("TacticSkills/", "HatcheryButton", skill, RoomType::hatchery);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl1depends.push_back(skill);

    resType = SkillType::roomDormitory;
    index = static_cast<uint32_t>(resType);
    skill = new Skill(resType, 0, emptyDepends);
    def = new SkillDefRoom("TacticSkills/", "DormitoryButton", skill, RoomType::dormitory);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl1depends.push_back(skill);

    // Lvl 2 skills
    resType = SkillType::roomWorkshop;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl1depends);
    def = new SkillDefRoom("TacticSkills/", "WorkshopButton", skill, RoomType::workshop);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl2depends.push_back(skill);

    resType = SkillType::trapDoorWooden;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl1depends);
    def = new SkillDefTrap("TacticSkills/", "WoodenDoorTrapButton", skill, TrapType::doorWooden);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl2depends.push_back(skill);

    // Lvl 3 skills
    resType = SkillType::trapCannon;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl2depends);
    def = new SkillDefTrap("TacticSkills/", "CannonButton", skill, TrapType::cannon);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl3depends.push_back(skill);

    resType = SkillType::trapSpike;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl2depends);
    def = new SkillDefTrap("TacticSkills/", "SpikeTrapButton", skill, TrapType::spike);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl3depends.push_back(skill);

    // Lvl 4 research
    resType = SkillType::trapBoulder;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl3depends);
    def = new SkillDefTrap("TacticSkills/", "BoulderTrapButton", skill, TrapType::boulder);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;

    // Magic Skills
    lvl1depends.clear();
    lvl2depends.clear();
    lvl3depends.clear();

    // Lvl 1 skills
    resType = SkillType::roomLibrary;
    skill = new Skill(resType, 0, emptyDepends);
    index = static_cast<uint32_t>(resType);
    def = new SkillDefRoom("MagicSkills/", "LibraryButton", skill, RoomType::library);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl1depends.push_back(skill);

    resType = SkillType::spellSummonWorker;
    index = static_cast<uint32_t>(resType);
    skill = new Skill(resType, 0, emptyDepends);
    def = new SkillDefSpell("MagicSkills/", "SummonWorkerButton", skill, SpellType::summonWorker);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl1depends.push_back(skill);

    // Lvl 2 skills
    resType = SkillType::roomCrypt;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl1depends);
    def = new SkillDefRoom("MagicSkills/", "CryptButton", skill, RoomType::crypt);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl2depends.push_back(skill);

    // Lvl 2 skills
    resType = SkillType::spellCreatureHeal;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl1depends);
    def = new SkillDefSpell("MagicSkills/", "CreatureHealButton", skill, SpellType::creatureHeal);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl2depends.push_back(skill);

    // Lvl 3 skills
    resType = SkillType::spellCreatureHaste;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl2depends);
    def = new SkillDefSpell("MagicSkills/", "CreatureHasteButton", skill, SpellType::creatureHaste);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl3depends.push_back(skill);

    resType = SkillType::spellCreatureDefense;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl2depends);
    def = new SkillDefSpell("MagicSkills/", "CreatureDefenseButton", skill, SpellType::creatureDefense);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl3depends.push_back(skill);

    resType = SkillType::spellCreatureSlow;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl2depends);
    def = new SkillDefSpell("MagicSkills/", "CreatureSlowButton", skill, SpellType::creatureSlow);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl3depends.push_back(skill);

    // Lvl 4 skills
    resType = SkillType::spellCreatureStrength;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl3depends);
    def = new SkillDefSpell("MagicSkills/", "CreatureStrengthButton", skill, SpellType::creatureStrength);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl4depends.push_back(skill);

    resType = SkillType::spellCreatureWeak;
    index = static_cast<uint32_t>(resType);
    points = ConfigManager::getSingleton().getSkillPoints(Skills::skillTypeToString(resType));
    skill = new Skill(resType, points, lvl3depends);
    def = new SkillDefSpell("MagicSkills/", "CreatureWeakButton", skill, SpellType::creatureWeak);
    def->mapSkill(mSkillsFamily);
    mSkills[index] = def;
    lvl4depends.push_back(skill);
}

SkillManager::~SkillManager()
{
    for(const SkillDef* skill : mSkills)
    {
        if(skill == nullptr)
            continue;

        delete skill->mSkill;
        delete skill;
    }
    mSkills.clear();
}

bool SkillManager::isRoomAvailable(RoomType type, const Seat* seat)
{
    uint32_t index = static_cast<uint32_t>(SkillFamily::rooms);
    std::vector<SkillType>& family = getSkillManager().mSkillsFamily[index];
    index = static_cast<uint32_t>(type);
    if(index >= family.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
        return false;
    }
    SkillType resType = family.at(index);
    return seat->isSkillDone(resType);
}

bool SkillManager::isSpellAvailable(SpellType type, const Seat* seat)
{
    uint32_t index = static_cast<uint32_t>(SkillFamily::spells);
    std::vector<SkillType>& family = getSkillManager().mSkillsFamily[index];
    index = static_cast<uint32_t>(type);
    if(index >= family.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
        return false;
    }
    SkillType resType = family.at(index);
    return seat->isSkillDone(resType);
}

bool SkillManager::isTrapAvailable(TrapType type, const Seat* seat)
{
    uint32_t index = static_cast<uint32_t>(SkillFamily::traps);
    std::vector<SkillType>& family = getSkillManager().mSkillsFamily[index];
    index = static_cast<uint32_t>(type);
    if(index >= family.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(family.size()));
        return false;
    }
    SkillType resType = family.at(index);
    return seat->isSkillDone(resType);
}

bool SkillManager::isAllSkillsDoneForSeat(const Seat* seat)
{
    if(seat->isSkilling())
        return false;

    for(const SkillDef* skill : getSkillManager().mSkills)
    {
        if(skill == nullptr)
            continue;
        if(seat->isSkillDone(skill->mSkill->getType()))
            continue;
        if(!skill->mSkill->canBeSkilled(seat->getSkillDone()))
            continue;
        return false;
    }
    return true;
}

const Skill* SkillManager::getSkill(SkillType resType)
{
    uint32_t index = static_cast<uint32_t>(resType);
    if(index >= getSkillManager().mSkills.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(getSkillManager().mSkills.size()));
        return nullptr;
    }

    const SkillDef* skill = getSkillManager().mSkills[index];
    if(skill == nullptr)
    {
        OD_LOG_ERR("null skill index=" + Helper::toString(index));
        return nullptr;
    }
    return skill->mSkill;
}

void SkillManager::buildRandomPendingSkillsForSeat(std::vector<SkillType>& skills,
        const Seat* seat)
{
    const std::vector<SkillType>& skillNotAllowed = seat->getSkillNotAllowed();
    std::vector<const SkillDef*> availableSkills;
    // We consider as done skills already done for the given seat as well as the skills
    // already pending
    std::vector<SkillType> doneSkills = seat->getSkillDone();
    doneSkills.insert(doneSkills.begin(), skills.begin(), skills.end());
    // We fill availableSkills with all the skills that are not already done and
    // that can be researched
    for(const SkillDef* skill : getSkillManager().mSkills)
    {
        if(skill == nullptr)
            continue;

        SkillType resType = skill->mSkill->getType();
        // We do not consider skills already pending or done
        if(std::find(doneSkills.begin(), doneSkills.end(), resType) != doneSkills.end())
            continue;

        if(skill->mSkill->dependsOn(skillNotAllowed))
            continue;

        availableSkills.push_back(skill);
        doneSkills.push_back(resType);
    }

    // Now, availableSkills only contains skills that can be done (but unsorted).
    // We need to shuffle that and fill skills.
    doneSkills = seat->getSkillDone();
    std::random_shuffle(availableSkills.begin(), availableSkills.end());
    for(const SkillDef* skill : availableSkills)
    {
        // Since buildDependencies guarantees to not add duplicate skills, it is safe
        // to call it for every skill not already done
        skill->mSkill->buildDependencies(doneSkills, skills);
    }
}

void SkillManager::listAllSkills(const std::function<void(const std::string&, const std::string&,
        const std::string&, SkillType)>& func)
{
    for(const SkillDef* skill : getSkillManager().mSkills)
    {
        if(skill == nullptr)
            continue;

        func(skill->mSkillFamily + skill->mButtonName, skill->getGuiPath() + skill->mButtonName,
             skill->mSkillFamily + skill->mButtonName + "/" + skill->mButtonName + "ProgressBar",
             skill->mSkill->getType());
    }
}

void SkillManager::listAllSpellsProgressBars(const std::function<void(SpellType spellType, const std::string&)>& func)
{
    std::vector<const SkillDef*>& skills = getSkillManager().mSkills;
    uint32_t index = static_cast<uint32_t>(SkillFamily::spells);
    std::vector<SkillType>& family = getSkillManager().mSkillsFamily.at(index);
    uint32_t nbSpells = static_cast<uint32_t>(SpellType::nbSpells);
    for(uint32_t i = 0; i < nbSpells; ++i)
    {
        if(i >= family.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(i) + ", size=" + Helper::toString(family.size()));
            return;
        }

        SkillType resType = family[i];
        if(resType == SkillType::nullSkillType)
            continue;

        index = static_cast<uint32_t>(resType);
        if(index >= skills.size())
        {
            OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(getSkillManager().mSkills.size()));
            continue;
        }

        const SkillDef* skill = skills[index];
        if(skill == nullptr)
        {
            OD_LOG_ERR("null skill index=" + Helper::toString(index));
            continue;
        }

        SpellType spellType = static_cast<SpellType>(i);
        func(spellType, skill->getGuiPath() + skill->mButtonName + "/" + skill->mButtonName + "ProgressBar");
    }
}

void SkillManager::connectSkills(GameMode* mode, CEGUI::Window* rootWindow)
{
    for(const SkillDef* skill : getSkillManager().mSkills)
    {
        if(skill == nullptr)
            continue;

        mode->addEventConnection(
            rootWindow->getChild("SkillTreeWindow/Skills/" + skill->mSkillFamily + skill->mButtonName)->subscribeEvent(
              CEGUI::PushButton::EventClicked,
              CEGUI::Event::Subscriber(SkillSelector(skill->mSkill->getType(), *mode))
            )
        );
    }
}

void SkillManager::connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection)
{
    for(const SkillDef* skill : getSkillManager().mSkills)
    {
        if(skill == nullptr)
            continue;

        skill->connectGuiButtons(mode, rootWindow, playerSelection);
    }
}
