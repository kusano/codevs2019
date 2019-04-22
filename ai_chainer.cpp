#include "ai_chainer.h"
#include <iostream>

using namespace std;

void AIChainer::initialize(Game &game)
{
};

Move AIChainer::think(Game &game)
{
    Move move;
    search(0, 2, game.fields[0], game.packs+game.turn, &move);
    return move;
}

int AIChainer::search(int depth, int maxDepth, Field &field, char pack[][4],
    Move *move)
{
    if (depth==maxDepth)
    {
        if (field.isDead())
            return 0;

        //  連鎖候補は長く、ブロックは多く、高さは低く
        return
            field.candChain()*10000 +
            (100-field.maxHeight())*100 +
            (200-field.blockNum());
    }

    int maxScore = -1;

    if (depth==0)
        *move = Move(0, 0, false);

    for (int pos=0; pos<9; pos++)
    for (int rotate=0; rotate<4; rotate++)
    {
        Move m(pos, rotate, false);
        //  TODO: 500ターンを越えたとき
        Result result = field.move(m, pack[depth]);
        int score;
        if (!field.isDead())
            score = search(depth+1, maxDepth, field, pack, move);
        else
            score = 0;
        field.undo();

        //  10連鎖以上が可能なら打つ
        if (depth==0 && result.chain>=10)
        {
            *move = m;
            return score;
        }

        if (score > maxScore)
        {
            maxScore = score;
            if (depth==0)
                *move = m;
        }
    }

    return maxScore;
}
