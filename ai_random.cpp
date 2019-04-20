#include "ai_random.h"

void AIRandom::initialize(Status &status)
{
    random.seed(1234);
};

Move AIRandom::think(Status &status)
{
    Move move;
    move.pos = random()%9;
    move.rotate = random()%4;
    if (status.fields[0].skill >= 80)
        move.bomb = random()%2==0;
    else
        move.bomb = false;
    return move;
}
