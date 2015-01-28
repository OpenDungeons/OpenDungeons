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
#include <ostream>
#include <istream>

class Goal;
class ODPacket;
class GameMap;
class CreatureDefinition;
class Player;
class Tile;

class Seat
{
public:
    friend class GameMap;
    friend class ODClient;
    // Constructors
    Seat(GameMap* gameMap);

    inline Player* getPlayer()
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

    unsigned int getNumClaimedTiles();
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

    inline const std::string& getFaction() const
    { return mFaction; }

    inline void setFaction(const std::string& faction)
    { mFaction = faction; }

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

    inline int getStartingGold() const
    { return mStartingGold; }

    inline Ogre::Vector3 getStartingPosition() const
    { return Ogre::Vector3(static_cast<Ogre::Real>(mStartingX), static_cast<Ogre::Real>(mStartingY), 0); }

    inline void addGoldMined(int quantity)
    { mGoldMined += quantity; }

    inline bool getIsDebuggingVision()
    { return mIsDebuggingVision; }

    inline int getNbTreasuries() const
    { return mNbTreasuries; }

    inline const std::string& getPlayerType() const
    { return mPlayerType; }

    inline void setPlayerType(const std::string& playerType)
    { mPlayerType = playerType; }

    void setPlayer(Player* player);

    void addAlliedSeat(Seat* seat);

    void initSpawnPool();

    void setMapSize(int x, int y);

    //! \brief Returns the next fighter creature class to spawn.
    const CreatureDefinition* getNextFighterClassToSpawn();

    //! \brief Returns the first (default) worker class definition.
    const CreatureDefinition* getWorkerClassToSpawn()
    {
        return mDefaultWorkerClass;
    }

    //! \brief Returns true if the given seat is allied. False otherwise
    bool isAlliedSeat(Seat *seat);

    //! \brief Checks if the seat is allowed to do corresponding action
    bool canOwnedCreatureBePickedUpBy(Seat* seat);
    bool canOwnedTileBeClaimedBy(Seat* seat);
    bool canOwnedCreatureUseRoomFrom(Seat* seat);
    bool canRoomBeDestroyedBy(Seat* seat);
    bool canTrapBeDestroyedBy(Seat* seat);

    void clearTilesWithVision();
    void notifyVisionOnTile(Tile* tile);

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

    const std::vector<Seat*>& getAlliedSeats()
    { return mAlliedSeats; }

    void computeSeatBeforeSendingToClient();

    static bool sortForMapSave(Seat* s1, Seat* s2);

    static Seat* getRogueSeat(GameMap* gameMap);

    static std::string getFormat();
    friend ODPacket& operator<<(ODPacket& os, Seat *s);
    friend ODPacket& operator>>(ODPacket& is, Seat *s);
    friend std::ostream& operator<<(std::ostream& os, Seat *s);

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

    int mNumCreaturesControlled;

    int mStartingGold;

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

    //! \broef The default workers spawned in temples.
    const CreatureDefinition* mDefaultWorkerClass;

    //! \brief List of all the tiles in the gamemap (used for human players seats only). The first vector stores the X position.
    //! The second vector stores the Y position. The first bool from the pair is set to true if the seat had vision on the concerned
    //! tile during the last turn. The second bool is the same but for the current turn. That allows to notify only the tiles where
    //! vision changed
    std::vector<std::vector<std::pair<bool, bool>>> mTilesVision;
    std::vector<Tile*> mVisualDebugEntityTiles;

    //! \brief How many tiles have been claimed by this seat, updated in GameMap::doTurn().
    unsigned int mNumClaimedTiles;

    bool mHasGoalsChanged;

    //! \brief The total amount of gold coins in the keeper's treasury and in the dungeon heart.
    int mGold;

    //! \brief The seat id. Allows to identify this seat. Must be unique per level file.
    int mId;

    //! \brief The number of treasuries the player owns. Useful to display the first free tile on client side.
    int mNbTreasuries;

    bool mIsDebuggingVision;
};

#endif // SEAT_H
