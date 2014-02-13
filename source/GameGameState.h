#include <AbstractGameState.h>


class GameGameState: public AbstractGameState {

    GameGameState(GameStateManager& gm):
        AbstractGameState(gm)
    {}

    ~GameGameState() {}

};
