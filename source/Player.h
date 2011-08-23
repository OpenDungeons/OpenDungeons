#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>


#include "Trap.h" //Class has enum, so has to include this.
#include "Room.h"
class Seat;
class Creature;

/*! \brief The player cleass contains information about a human, or computer, player in the game.
 *
 * When a new player joins a game being hosted on a server the server will
 * allocate a new Player structure and fill it in with the appropriate values.
 * Its relevant information will then be sent to the other players in the game
 * so they are aware of its presence.  In the future if we decide to do a
 * single player game, thiis is where the computer driven strategy AI
 * calculations will take place.
 */
class Player
{
public:
    Player();
    
    bool humanPlayer; /**< True: player is human.    False: player is a computer. */
    //int goldInTreasury();
    //int oreInRefinery();
    //int ironInRefinery();

    const std::string& getNick() {return nick;}
    Seat* getSeat() {return seat;}
    const Seat* getSeat() const {return seat;}
    void setNick (const std::string& nick) {this->nick = nick;}
    void setSeat(Seat* seat) {this->seat = seat;}

    // Public functions
    unsigned int numCreaturesInHand() const;
    Creature *getCreatureInHand(int i);
    const Creature* getCreatureInHand(int i) const;
    void pickUpCreature(Creature *c);
    bool dropCreature(Tile *t);
    void rotateCreaturesInHand(int n);
    inline void setGameMap(GameMap* gameMap) {this->gameMap = gameMap;};

    // Public data members
    
    Room::RoomType newRoomType;
    Trap::TrapType newTrapType;

private:
    GameMap* gameMap;
    Seat *seat;
    std::string nick; /**< The nickname used un chat, etc. */
    // Private functions
    void addCreatureToHand(Creature *c); // Private, for other classes use pickUpCreature() instead.
    void removeCreatureFromHand(int i); // Private, for other classes use dropCreature() instead.

    // Private datamembers
    std::vector<Creature*> creaturesInHand;
};

#endif

