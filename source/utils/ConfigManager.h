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

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <OgreSingleton.h>
#include <OgreColourValue.h>

#include <cstdint>

class CreatureDefinition;
class Weapon;
class SpawnCondition;
class Skill;
class TileSet;
class TileSetValue;

enum class TileVisual;

namespace Config
{
//! \brief Config options names
// Categories
enum Ctg : std::size_t
{
    NONE  = 0,
    VIDEO = 1,
    AUDIO = 2,
    GAME  = 3,
    INPUT = 4,
    TOTAL = 5
};

// Video
const std::string RENDERER = "Renderer";
const std::string VIDEO_MODE = "Video Mode";
const std::string VSYNC = "VSync";
const std::string FULL_SCREEN = "Full Screen";
// Audio
const std::string MUSIC_VOLUME = "Music Volume";
// Input
const std::string KEYBOARD_GRAB = "Keyboard Grab";
const std::string MOUSE_GRAB = "Mouse Grab";
const std::string AUTOSCROLL = "Autoscroll";
// Game
const std::string NICKNAME = "Nickname";
const std::string KEEPERVOICE = "KeeperVoice";
const std::string MINIMAP_TYPE = "MinimapType";
const std::string LIGHT_FACTOR = "LightFactor";
}

//! \brief This class is used to manage global configuration such as network configuration, global creature stats, ...
//! It should NOT be used to load level specific stuff. For that, there is GameMap.
class ConfigManager : public Ogre::Singleton<ConfigManager>
{
public:
    //! \brief Loads the game configuration files.
    //! \param configPath The system configuration path.
    //! \param userConfigPath The user profile config path or empty if not used.
    //! \note In server mode, the configuration doesn't load the user config and thus,
    //! doesn't set the userConfigPath.
    ConfigManager(const std::string& configPath, const std::string& userConfigPath,
        const std::string& soundPath);
    ~ConfigManager();

    static const std::string DefaultWorkerCreatureDefinition;
    static const std::string DEFAULT_TILESET_NAME;
    static const std::string DEFAULT_KEEPER_VOICE;

    const Ogre::ColourValue& getColorFromId(const std::string& id) const;
    inline const std::map<std::string, CreatureDefinition*>& getCreatureDefinitions() const
    { return mCreatureDefs; }
    const CreatureDefinition* getCreatureDefinition(const std::string& name) const;

    inline const std::vector<const Weapon*>& getWeapons() const
    { return mWeapons; }
    const Weapon* getWeapon(const std::string& name) const;

    inline uint32_t getCreatureDeathCounter() const
    { return mCreatureDeathCounter; }

    inline uint32_t getMaxCreaturesPerSeatAbsolute() const
    { return mMaxCreaturesPerSeatAbsolute; }

    inline uint32_t getMaxCreaturesPerSeatDefault() const
    { return mMaxCreaturesPerSeatDefault; }

    inline int32_t getCreatureBaseMood() const
    { return mCreatureBaseMood; }

    inline int32_t getCreatureMoodHappy() const
    { return mCreatureMoodHappy; }

    inline int32_t getCreatureMoodUpset() const
    { return mCreatureMoodUpset; }

    inline int32_t getCreatureMoodAngry() const
    { return mCreatureMoodAngry; }

    inline int32_t getCreatureMoodFurious() const
    { return mCreatureMoodFurious; }

    inline double getSlapDamagePercent() const
    { return mSlapDamagePercent; }

    inline uint32_t getSlapEffectDuration() const
    { return mSlapEffectDuration; }

    inline int64_t getTimePayDay() const
    { return mTimePayDay; }

    inline uint32_t getNetworkPort() const
    { return mNetworkPort; }

    inline uint32_t getClientConnectionTimeout() const
    { return mClientConnectionTimeout; }

    inline uint32_t getBaseSpawnPoint() const
    { return mBaseSpawnPoint; }

    inline int32_t getNbTurnsFuriousMax() const
    { return mNbTurnsFuriousMax; }

    inline double getMaxManaPerSeat() const
    { return mMaxManaPerSeat; }

    inline double getClaimingWallPenalty() const
    { return mClaimingWallPenalty; }

    inline double getDigCoefGold() const
    { return mDigCoefGold; }

    inline double getDigCoefGem() const
    { return mDigCoefGem; }

    inline double getDigCoefClaimedWall() const
    { return mDigCoefClaimedWall; }

    inline int32_t getNbTurnsKoCreatureAttacked() const
    { return mNbTurnsKoCreatureAttacked; }

    inline const std::string& getMainMenuMusic() const
    { return mMainMenuMusic; }

    inline const std::string& getMasterServerUrl() const
    { return mMasterServerUrl; }

    const std::vector<const SpawnCondition*>& getCreatureSpawnConditions(const CreatureDefinition* def) const;

    //! \brief Get the fighter creature definition spawnable in portals according to the given faction.
    const std::vector<std::string>& getFactionSpawnPool(const std::string& faction) const;

    //! \brief Get the default worker creature definition spawnable according to the given faction.
    const std::string& getFactionWorkerClass(const std::string& faction) const;

    const std::string& getRogueWorkerClass() const
    { return mDefaultWorkerRogue; }

    inline const std::vector<std::string>& getFactions() const
    { return mFactions; }

    //! Rooms configuration
    const std::string& getRoomConfigString(const std::string& param) const;
    uint32_t getRoomConfigUInt32(const std::string& param) const;
    int32_t getRoomConfigInt32(const std::string& param) const;
    double getRoomConfigDouble(const std::string& param) const;

    //! Traps configuration
    const std::string& getTrapConfigString(const std::string& param) const;
    uint32_t getTrapConfigUInt32(const std::string& param) const;
    int32_t getTrapConfigInt32(const std::string& param) const;
    double getTrapConfigDouble(const std::string& param) const;

    //! Spells configuration
    const std::string& getSpellConfigString(const std::string& param) const;
    uint32_t getSpellConfigUInt32(const std::string& param) const;
    int32_t getSpellConfigInt32(const std::string& param) const;
    double getSpellConfigDouble(const std::string& param) const;

    int32_t getSkillPoints(const std::string& res) const;

    inline const CreatureDefinition* getCreatureDefinitionDefaultWorker() const
    { return mCreatureDefinitionDefaultWorker; }

    inline uint32_t getNbWorkersDigSameFaceTile() const
    { return mNbWorkersDigSameFaceTile; }

    inline uint32_t getNbWorkersClaimSameTile() const
    { return mNbWorkersClaimSameTile; }

    //! Returns the tileset for the given name. If the tileset is not found, returns the default tileset
    const TileSet* getTileSet(const std::string& tileSetName) const;

    //! \brief Set a config value. Only permits predetermined types.
    void setAudioValue(const std::string& param, const std::string& value)
    { mUserConfig[Config::Ctg::AUDIO][param] = value; }
    void setVideoValue(const std::string& param, const std::string& value)
    { mUserConfig[Config::Ctg::VIDEO][param] = value; }
    void setInputValue(const std::string& param, const std::string& value)
    { mUserConfig[Config::Ctg::INPUT][param] = value; }
    void setGameValue(const std::string& param, const std::string& value)
    { mUserConfig[Config::Ctg::GAME][param] = value; }

    //! \brief Get a config value.
    inline const std::string getAudioValue(const std::string& param,
                                            const std::string& defaultValue = std::string(),
                                            bool triggerError = true) const
    { return getUserValue(Config::Ctg::AUDIO, param, defaultValue, triggerError); }
    inline const std::string getVideoValue(const std::string& param,
                                            const std::string& defaultValue = std::string(),
                                            bool triggerError = true) const
    { return getUserValue(Config::Ctg::VIDEO, param, defaultValue, triggerError); }
    inline const std::string getInputValue(const std::string& param,
                                            const std::string& defaultValue = std::string(),
                                            bool triggerError = true) const
    { return getUserValue(Config::Ctg::INPUT, param, defaultValue, triggerError); }
    inline const std::string getGameValue(const std::string& param,
                                           const std::string& defaultValue = std::string(),
                                           bool triggerError = true) const
    { return getUserValue(Config::Ctg::GAME, param, defaultValue, triggerError); }

    //! \brief Save the user configuration file.
    bool saveUserConfig();

    //! \brief Tries to restore the previous video config
    //! To be called at startup once the user config has been loaded.
    bool initVideoConfig(Ogre::Root& ogreRoot);

    const std::vector<std::string>& getKeeperVoices() const
    { return mKeeperVoices; }

private:
    //! \brief Function used to load the global configuration. They should return true if the configuration
    //! is ok and false if a mandatory parameter is missing
    bool loadGlobalConfig(const std::string& configPath);
    bool loadGlobalConfigSeatColors(std::stringstream& configFile);
    bool loadGlobalConfigDefinitionFiles(std::stringstream& configFile);
    bool loadGlobalGameConfig(std::stringstream& configFile);
    bool loadCreatureDefinitions(const std::string& fileName);
    bool loadEquipements(const std::string& fileName);
    bool loadSpawnConditions(const std::string& fileName);
    bool loadFactions(const std::string& fileName);
    bool loadRooms(const std::string& fileName);
    bool loadTraps(const std::string& fileName);
    bool loadSpellConfig(const std::string& fileName);
    bool loadSkills(const std::string& fileName);
    bool loadTilesets(const std::string& fileName);
    bool loadTilesetValues(std::istream& defFile, TileVisual tileVisual, std::vector<TileSetValue>& tileValues);

    //! \brief Loads the user configuration values, and use default ones if it cannot do it.
    void loadUserConfig(const std::string& fileName);

    void loadKeeperVoices(const std::string& soundPath);

    //! \brief Get a config value.
    const std::string getUserValue(Config::Ctg category,
                                    const std::string& param,
                                    const std::string& defaultValue = std::string(),
                                    bool triggerError = true) const;

    std::map<std::string, Ogre::ColourValue> mSeatColors;
    std::map<std::string, CreatureDefinition*> mCreatureDefs;
    std::vector<const Weapon*> mWeapons;
    std::string mFilenameCreatureDefinition;
    std::string mFilenameEquipmentDefinition;
    std::string mFilenameSpawnConditions;
    std::string mFilenameFactions;
    std::string mFilenameRooms;
    std::string mFilenameTraps;
    std::string mFilenameSpells;
    std::string mFilenameSkills;
    std::string mFilenameTilesets;
    std::string mFilenameUserCfg;
    uint32_t mNetworkPort;
    uint32_t mClientConnectionTimeout;
    uint32_t mBaseSpawnPoint;
    uint32_t mCreatureDeathCounter;
    uint32_t mMaxCreaturesPerSeatAbsolute;
    uint32_t mMaxCreaturesPerSeatDefault;
    int32_t mCreatureBaseMood;
    int32_t mCreatureMoodHappy;
    int32_t mCreatureMoodUpset;
    int32_t mCreatureMoodAngry;
    int32_t mCreatureMoodFurious;
    double mSlapDamagePercent;
    uint32_t mSlapEffectDuration;
    int64_t mTimePayDay;
    int32_t mNbTurnsFuriousMax;
    double mMaxManaPerSeat;
    double mClaimingWallPenalty;
    double mDigCoefGold;
    double mDigCoefGem;
    double mDigCoefClaimedWall;
    int32_t mNbTurnsKoCreatureAttacked;
    std::string mDefaultWorkerRogue;
    std::string mMainMenuMusic;
    std::string mMasterServerUrl;
    std::map<const CreatureDefinition*, std::vector<const SpawnCondition*> > mCreatureSpawnConditions;
    std::map<const std::string, std::vector<std::string> > mFactionSpawnPool;

    //! \brief Stores the faction default worker creature definition.
    std::map<const std::string, std::string> mFactionDefaultWorkerClass;

    std::vector<std::string> mFactions;
    std::map<const std::string, std::string> mRoomsConfig;
    std::map<const std::string, std::string> mTrapsConfig;
    std::map<const std::string, std::string> mSpellConfig;
    std::map<const std::string, int32_t> mSkillPoints;

    //! \brief Default definition for the editor. At map loading, it will spawn a creature from
    //! the default seat worker depending on seat faction
    CreatureDefinition* mCreatureDefinitionDefaultWorker;

    uint32_t mNbWorkersDigSameFaceTile;
    uint32_t mNbWorkersClaimSameTile;

    //! \brief Allowed tilesets
    std::map<std::string, const TileSet*> mTileSets;

    //! \brief User config values
    //! < category, < param, value > >
    std::vector< std::map<std::string, std::string> > mUserConfig;

    //! \brief List of the found keeper voices (in the relative sound folder)
    std::vector<std::string> mKeeperVoices;
};

#endif //CONFIGMANAGER_H
