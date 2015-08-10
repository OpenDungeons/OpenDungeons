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

#ifndef SEAT_H
#define SEAT_H


#include <OgreVector3.h>
#include <OgreColourValue.h>
#include <string>
#include <vector>
#include <iosfwd>
#include <cstdint>

class Building;
class ConfigManager;
class Goal;
class ODPacket;
class GameMap;
class CreatureDefinition;
class Player;
class Research;
class Seat;
class Tile;

enum class ResearchType;
enum class SpellType;
enum class RoomType;
enum class TileVisual;
enum class TrapType;

//! Class used to save the last tile state notified to each seat
class TileStateNotified
{
public:
    TileStateNotified();

    TileVisual mTileVisual;
    int mSeatIdOwner;
    bool mMarkedForDigging;
    bool mVisionTurnLast;
    bool mVisionTurnCurrent;
    Building* mBuilding;
};

class Seat
{
public:
    friend class GameMap;
    friend class ODClient;
    // Constructors
    Seat(GameMap* gameMap);

    inline Player* getPlayer() const
    { return mPlayer; }

    //! \brief Adds a goal to the vector of goals which must be completed by this seat before it can be declared a winner.
    void addGoal(Goal* g);

    /** \brief A simple accessor function to return the number of goals
     * which must be completed by this seat before it can be declared a winner.
     */
    unsigned int numUncompleteGoals();

    /** \brief A simple accessor function to allow for looping over the goals
     * which must be completed by this seat before it can be declared a winner.
     */
    Goal* getUncompleteGoal(unsigned int index);

    //! \brief A simple mutator to clear the vector of unmet goals.
    void clearUncompleteGoals();

    //! \brief A simple mutator to clear the vector of met goals.
    void clearCompletedGoals();

    /** \brief Loop over the vector of unmet goals and call the isMet() and isFailed() functions on
     * each one, if it is met move it to the completedGoals vector.
     */
    unsigned int checkAllGoals();

    /** \brief Loop over the vector of met goals and call the isUnmet() function on each one,
     * if any of them are no longer satisfied move them back to the goals vector.
     */
    unsigned int checkAllCompletedGoals();

    //! \brief A simple accessor function to return the number of goals completed by this seat.
    unsigned int numCompletedGoals();

    //! \brief A simple accessor function to allow for looping over the goals completed by this seat.
    Goal* getCompletedGoal(unsigned int index);

    //! \brief A simple accessor function to return the number of goals failed by this seat.
    unsigned int numFailedGoals();

    //! \brief A simple accessor function to allow for looping over the goals failed by this seat.
    Goal* getFailedGoal(unsigned int index);

    unsigned int getNumClaimedTiles() const;
    void setNumClaimedTiles(const unsigned int& num);

    //! \brief Increment the number of claimed tiles by 1.
    void incrementNumClaimedTiles();

    /** \brief See if the goals has changed since we last checked.
     *  For use with the goal window, to avoid having to update it on every frame.
     */
    bool getHasGoalsChanged();

    void resetGoalsChanged();

    void refreshFromSeat(Seat* s);

    inline int getTeamId() const
    { return mTeamId; }

    inline bool isRogueSeat() const
    { return mId == 0; }

    void setTeamId(int teamId);

    inline const std::vector<int>& getAvailableTeamIds() const
    { return mAvailableTeamIds; }

    inline int getId() const
    { return mId; }

    inline uint32_t getTeamIndex() const
    { return mTeamIndex; }

    inline void setTeamIndex(uint32_t index)
    { mTeamIndex = index; }

    inline const std::string& getFaction() const
    { return mFaction; }

    inline void setFaction(const std::string& faction)
    { mFaction = faction; }

    inline int32_t getConfigPlayerId() const
    { return mConfigPlayerId; }

    inline void setConfigPlayerId(int32_t configPlayerId)
    { mConfigPlayerId = configPlayerId; }

    inline int32_t getConfigTeamId() const
    { return mConfigTeamId; }

    inline void setConfigTeamId(int32_t configTeamId)
    { mConfigTeamId = configTeamId; }

    inline int32_t getConfigFactionIndex() const
    { return mConfigFactionIndex; }

    inline void setConfigFactionIndex(int32_t configFactionIndex)
    { mConfigFactionIndex = configFactionIndex; }

    inline const std::string& getColorId() const
    { return mColorId; }

    inline const Ogre::ColourValue& getColorValue() const
    { return mColorValue; }

    inline int getGold() const
    { return mGold; }

    inline int getGoldMined() const
    { return mGoldMined; }

    inline double getMana() const
    { return mMana; }

    inline double getManaDelta() const
    { return mManaDelta; }

    inline int getNumCreaturesFighters() const
    { return mNumCreaturesFighters; }

    inline int getNumCreaturesFightersMax() const
    { return mNumCreaturesFightersMax; }

    bool takeMana(double mana);

    inline Ogre::Vector3 getStartingPosition() const
    { return Ogre::Vector3(static_cast<Ogre::Real>(mStartingX), static_cast<Ogre::Real>(mStartingY), 0); }

    inline void addGoldMined(int quantity)
    { mGoldMined += quantity; }

    inline bool getIsDebuggingVision()
    { return mIsDebuggingVision; }

    uint32_t getNbRooms(RoomType roomType) const;

    inline const std::string& getPlayerType() const
    { return mPlayerType; }

    inline void setPlayerType(const std::string& playerType)
    { mPlayerType = playerType; }

    inline const std::vector<ResearchType>& getResearchDone() const
    { return mResearchDone; }

    inline const std::vector<ResearchType>& getResearchPending() const
    { return mResearchPending; }

    inline const std::vector<ResearchType>& getResearchNotAllowed() const
    { return mResearchNotAllowed; }

    void setPlayer(Player* player);

    void addAlliedSeat(Seat* seat);

    void initSeat();

    void setMapSize(int x, int y);

    //! \brief Returns the next fighter creature class to spawn.
    const CreatureDefinition* getNextFighterClassToSpawn(const GameMap& gameMap, const ConfigManager& configManager );

    //! \brief Returns the first (default) worker class definition.
    inline const CreatureDefinition* getWorkerClassToSpawn()
    { return mDefaultWorkerClass; }

    //! \brief Returns true if the given seat is allied. False otherwise
    bool isAlliedSeat(const Seat *seat) const;

    //! \brief Checks if the seat is allowed to do corresponding action
    bool canOwnedCreatureBePickedUpBy(const Seat* seat) const;
    bool canOwnedTileBeClaimedBy(const Seat* seat) const;
    bool canOwnedCreatureUseRoomFrom(const Seat* seat) const;
    bool canBuildingBeDestroyedBy(const Seat* seat) const;

    void clearTilesWithVision();
    void notifyVisionOnTile(Tile* tile);
    void notifyTileClaimedByEnemy(Tile* tile);

    //! \brief Returns true if this seat can see the given tile and false otherwise
    bool hasVisionOnTile(Tile* tile);

    //! \brief Checks if the visible tiles seen by this seat have changed and notify
    //! the players if yes
    void notifyChangedVisibleTiles();

    //! \brief Server side to display the tile this seat has vision on
    void displaySeatVisualDebug(bool enable);

    //! Sends a message to the player on this seat to refresh the list of tiles he has vision on
    void sendVisibleTiles();

    //! \brief Client side to display the tile this seat has vision on
    void refreshVisualDebugEntities(const std::vector<Tile*>& tiles);
    void stopVisualDebugEntities();

    inline const std::vector<Seat*>& getAlliedSeats()
    { return mAlliedSeats; }

    void computeSeatBeforeSendingToClient();

    //! \brief Gets whether a research is being done
    bool isResearching() const
    { return mCurrentResearch != nullptr; }

    //! \brief Gets the current type and percentage of research done. (0.0f-1.0f).
    //! Returns true if a research is in progress and false otherwise
    bool getCurrentResearchProgress(ResearchType& type, float& progress) const;

    //! \brief Tells whether the given research type is in the pending queue.
    //! \return The number of the pending research in the research queue or 0 if not there.
    uint32_t isResearchPending(ResearchType resType) const;

    ResearchType getFirstResearchPending() const;

    //! \brief Returns true if the given ResearchType is already done for this player. False
    //! otherwise
    bool isResearchDone(ResearchType type) const;

    //! \brief Called when the research entity reaches its destination. From there, the
    //! researched thing is available
    //! Returns true if the type was inserted and false otherwise
    bool addResearch(ResearchType type);

    //! \brief Server side function. Called when a fresh grimoire is brought to the dungeon
    //! temple. When enough points are gathered, the corresponding research will become available
    void addResearchPoints(int32_t points);

    //! \brief Used on both client and server side. On server side, the research tree's validity
    //! will be checked. If ok, it will be sent to the client. If not, the research tree will not
    //! be changed. Note that the order of the research matters as the first researches in the
    //! given vector can needed for the next ones
    void setResearchTree(const std::vector<ResearchType>& researches);

    //! \brief Used on both client and server side
    void setResearchesDone(const std::vector<ResearchType>& researches);

    inline bool getGuiResearchNeedsRefresh() const
    { return mGuiResearchNeedsRefresh; }

    inline void guiResearchRefreshed()
    { mGuiResearchNeedsRefresh = false; }

    //! Called when a tile is notified to the seat player. That allows to save the state
    //! Used on server side only
    void updateTileStateForSeat(Tile* tile);

    //! Called when a tile is marked and notified to a player. That allows to save the state
    //! Note that the tile is marked depending on what the player knows about it, not
    //! on its real state.
    //! Used on server side only
    void tileMarkedDiggingNotifiedToPlayer(Tile* tile, bool isDigSet);

    //! \brief Tells whether the tile is diggable by dig-capable creatures. It relies
    //! on the tile state the client knows, not on the real tile state
    //! Called on server side only
    bool isTileDiggableForClient(Tile* tile) const;

    //! \brief Called for each seat when a building is removed from the gamemap. That allows
    //! the seats to clear the pointers to the building that they may have
    void notifyBuildingRemovedFromGameMap(Building* building, Tile* tile);

    void setVisibleBuildingOnTile(Building* building, Tile* tile);

    /*! \brief Exports the tile data to the packet so that the client associated to the seat have the needed information
     *         to display the tile correctly
     */
    void exportTileToPacket(ODPacket& os, const Tile* tile) const;

    static bool sortForMapSave(Seat* s1, Seat* s2);

    static Seat* createRogueSeat(GameMap* gameMap);

    friend ODPacket& operator<<(ODPacket& os, Seat *s);
    friend ODPacket& operator>>(ODPacket& is, Seat *s);

    bool importSeatFromStream(std::istream& is);
    bool exportSeatToStream(std::ostream& os) const;
    static void loadFromLine(const std::string& line, Seat *s);
    static const std::string getFactionFromLine(const std::string& line);

    static const std::string PLAYER_TYPE_HUMAN;
    static const std::string PLAYER_TYPE_AI;
    static const std::string PLAYER_TYPE_INACTIVE;
    static const std::string PLAYER_TYPE_CHOICE;

    static const std::string PLAYER_FACTION_CHOICE;

private:
    void goalsHasChanged();

    //! \brief The game map this seat belongs to
    GameMap* mGameMap;

    //! \brief The player sitting on this seat
    Player* mPlayer;

    //! \brief The team id of the player sitting in this seat.
    int mTeamId;

    //! \brief The type of player (can be Human or AI).
    std::string mPlayerType;

    //! \brief The name of the faction that this seat is playing as (can be Keeper or Hero).
    std::string mFaction;

    //! \brief The amount of 'keeper mana' the player has.
    double mMana;

    //! \brief The amount of 'keeper mana' the player gains/loses per turn, updated in GameMap::doTurn().
    double mManaDelta;

    //! \brief The starting camera location (in tile coordinates) of this seat.
    int mStartingX;
    int mStartingY;

    //! \brief The total amount of gold coins mined by workers under this seat's control.
    int mGoldMined;

    //! \brief The number of living creatures fighters under this seat's control
    int mNumCreaturesFighters;
    int mNumCreaturesFightersMax;

    //! \brief The actual color that this color index translates into.
    std::string mColorId;
    Ogre::ColourValue mColorValue;

    //! \brief Currently unmet goals, the first Seat to empty this wins.
    std::vector<Goal*> mUncompleteGoals;

    //! \brief Currently met goals.
    std::vector<Goal*> mCompletedGoals;

    //! \brief Currently failed goals which cannot possibly be met in the future.
    std::vector<Goal*> mFailedGoals;

    //! \brief Team ids this seat can use defined in the level file.
    std::vector<int> mAvailableTeamIds;

    //! \brief Contains all the seats allied with the current one, not including it. Used on server side only.
    std::vector<Seat*> mAlliedSeats;

    //! \brief The creatures the current seat is allowed to spawn (when following the conditions). CreatureDefinition
    //! are managed by the configuration manager and should NOT be deleted. The boolean will be set to false at beginning
    //! if the spawning conditions are not empty and are met, we will set it to true and force spawning of the related creature
    std::vector<std::pair<const CreatureDefinition*, bool> > mSpawnPool;

    //! \brief The default workers spawned in temples.
    const CreatureDefinition* mDefaultWorkerClass;

    //! \brief List of all the tiles in the gamemap (used for human players seats only). The first vector stores the X position.
    //! The second vector stores the Y position. TileStateNotified contains information about the tile
    //! state (last tile state notified, vision last turn for this seat, vision for current turn, ...
    std::vector<std::vector<TileStateNotified>> mTilesStates;

    std::map<std::pair<int, int>, TileStateNotified> mTilesStateLoaded;

    std::vector<Tile*> mVisualDebugEntityTiles;

    //! \brief How many tiles have been claimed by this seat, updated in GameMap::doTurn().
    unsigned int mNumClaimedTiles;

    bool mHasGoalsChanged;

    //! \brief The total amount of gold coins in the keeper's treasury and in the dungeon heart.
    int mGold;

    //! \brief The seat id. Allows to identify this seat. Must be unique per level file.
    int mId;

    //! \brief Index of the team in the gamemap (from 0 to N). Must be set when the seat is added to the gamemap
    //! and never changed after
    uint32_t mTeamIndex;

    //! \brief The number of rooms the player owns (room index being room type).
    //! Useful to display the first free tile on client side for example
    std::vector<uint32_t> mNbRooms;

    bool mIsDebuggingVision;

    //! \brief Counter for research points
    int32_t mResearchPoints;

    //! \brief Progress for current research. Allows to display the progressbar on the client side
    ResearchType mCurrentResearchType;
    float mCurrentResearchProgress;

    //! \brief Currently researched Research. This pointer is external and should not be deleted
    const Research* mCurrentResearch;

    //! \brief true when the Research tree Gui needs to be refreshed. Used by the game mode
    //! to know when to update its corresponding window.
    bool mGuiResearchNeedsRefresh;

    //! \brief Researches already done. This is used on both client and server side and should be updated
    std::vector<ResearchType> mResearchDone;

    //! \brief Researches pending. Used on both client and server side and should be updated.
    std::vector<ResearchType> mResearchPending;

    //! \brief Researches not allowed. Used on server side only
    std::vector<ResearchType> mResearchNotAllowed;

    //! \brief During seat configuration, we save temporary player id here
    int32_t mConfigPlayerId;
    int32_t mConfigTeamId;
    int32_t mConfigFactionIndex;

    //! \brief Server side function. Sets mCurrentResearch to the first entry in mResearchPending. If the pending
    //! list in empty, mCurrentResearch will be set to null
    //! researchedType is the currently researched type if any (nullResearchType if none)
    void setNextResearch(ResearchType researchedType);

    //! Fills mTilesStateLoaded with the tiles of the given tileVisual is the given istream.
    //! Returns 0 if the seat end tile has been reached, 1 if the read success and -1 if there is an error
    int readTilesVisualInitialStates(TileVisual tileVisual, std::istream& is);

    //! exports the tiles of the corresponding TileVisual this seat have seen
    void exportTilesVisualInitialStates(TileVisual tileVisual, std::ostream& os) const;
};

#endif // SEAT_H
