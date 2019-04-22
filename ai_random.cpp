#include "ai_random.h"

void AIRandom::initialize(Game &game, int seed)
{
    random.seed(seed);
};

Move AIRandom::think(Game &game)
{
    Move move;
    move.pos = random()%9;
    move.rotate = random()%4;
    if (game.fields[0].skill >= 80)
        move.bomb = random()%2==0;
    else
        move.bomb = false;
    return move;
}
