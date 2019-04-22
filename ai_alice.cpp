#include "ai_alice.h"
#include <iostream>
#include <algorithm>
#include <cassert>

using namespace std;

void AIAlice::initialize(Game &game, int seed)
{
    random.seed(seed);
};

Move AIAlice::think(Game &game)
{
    if (moves.empty() ||
        game.fields[0].ojama>=10)
        generateMove(game);

    assert(!moves.empty());
    Move move = moves[0];
    moves.erase(moves.begin());
    return move;
}

void AIAlice::generateMove(Game &game)
{
    struct Node
    {
        long long score = 0;
        //  TODO: stateを記録するより、movesから生成した方が良い？
        State state;
        std::vector<Move> moves;
    };

    //  各深さで連鎖数が最大となる命令列を記録
    vector<int> maxChain(BeamDepth, -1);
    vector<vector<Move>> maxMoves(BeamDepth);

    vector<Node> beam(1);
    game.fields[0].save(&beam[0].state);

    Field field;

    for (int depth=0; depth<BeamDepth; depth++)
    {
        cerr<<depth<<endl;
        vector<Node> beamPre;
        beamPre.swap(beam);

        for (Node &node: beamPre)
        {
            field.load(node.state);

            for (int pos=0; pos<9; pos++)
            for (int rotate=0; rotate<4; rotate++)
            {
                //  TODO: isDead/500ターン以上の考慮
                Move m(pos, rotate, false);

                Result result = field.move(m, game.packs[game.turn+depth]);

                //  連鎖候補は長く、ブロックは多く、高さは低く
                long long score =
                    field.candChain()*1000000LL +
                    -field.maxHeight()*10000LL +
                    -field.blockNum()*100LL +
                    random()%64;

                Node nodeNew;
                nodeNew.score = score;
                field.save(&nodeNew.state);
                nodeNew.moves = node.moves;
                nodeNew.moves.push_back(m);

                beam.push_back(nodeNew);

                if (result.chain > maxChain[depth])
                {
                    maxChain[depth] = result.chain;
                    maxMoves[depth] = nodeNew.moves;
                }

                field.undo();
            }
        }

        sort(beam.begin(), beam.end(),
            [](const Node &a, const Node &b) {return a.score > b.score;});
        if ((int)beam.size() > BeamWidth)
            beam.resize(BeamWidth);
    }

    //  連鎖数が最大の命令列を採用
    //  TODO: 浅い深さのものをより重視
    int bestDepth = 0;
    for (int depth=1; depth<BeamDepth; depth++)
        if (maxChain[depth] > maxChain[bestDepth])
            bestDepth = depth;
    moves = maxMoves[bestDepth];
}
