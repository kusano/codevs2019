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
        generateChainMove(game);

    assert(!moves.empty());
    Move move = moves[0];
    moves.erase(moves.begin());
    return move;
}

//  連鎖を狙う命令列を探索
void AIAlice::generateChainMove(Game &game)
{
    struct Node
    {
        long long score = 0;
        //  TODO: stateを記録するより、movesから生成した方が良い？
        State state;
        std::vector<Move> moves;
    };

    //  各深さで連鎖数が最大となる命令列を記録
    vector<int> maxChain(BeamChainDepth, -1);
    vector<vector<Move>> maxMoves(BeamChainDepth);

    vector<Node> beam(1);
    game.fields[0].save(&beam[0].state);

    Field field;

    for (int depth=0; depth<BeamChainDepth; depth++)
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
                long long score = ((((
                    field.isDead() ? -1 : 0)*100LL +
                    field.candChain())*100LL +
                    -field.maxHeight())*100LL +
                    -field.blockNum())*100LL +
                    random()%64;

                Node nodeNew;
                nodeNew.score = score;
                field.save(&nodeNew.state);
                nodeNew.moves = node.moves;
                nodeNew.moves.push_back(m);

                beam.push_back(nodeNew);

                if (!field.isDead() &&
                    result.chain > maxChain[depth])
                {
                    maxChain[depth] = result.chain;
                    maxMoves[depth] = nodeNew.moves;
                }

                field.undo();
            }
        }

        sort(beam.begin(), beam.end(),
            [](const Node &a, const Node &b) {return a.score > b.score;});
        if ((int)beam.size() > BeamChainWidth)
            beam.resize(BeamChainWidth);
    }

    //  連鎖数が最大の命令列を採用
    //  TODO: 浅い深さのものをより重視
    int bestDepth = 0;
    for (int depth=1; depth<BeamChainDepth; depth++)
        if (maxChain[depth] > maxChain[bestDepth])
            bestDepth = depth;
    moves = maxMoves[bestDepth];
}

//  スキルを狙う命令列を探索
void AIAlice::generateBombMove(Game &game)
{
    struct Node
    {
        long long score = 0;
        //  TODO: stateを記録するより、movesから生成した方が良い？
        State state;
        std::vector<Move> moves;
    };

    //  各深さでお邪魔ブロック数が最大となる命令列を記録
    vector<long long> maxOjama(BeamBombDepth, -1);
    vector<vector<Move>> maxMoves(BeamBombDepth);

    vector<Node> beam(1);
    game.fields[0].save(&beam[0].state);

    Field field;

    for (int depth=0; depth<BeamBombDepth; depth++)
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

                //  スキルゲージ、消せるブロック、その他のブロックを多く
                long long score = ((((
                    field.isDead() ? -1 : 0)*100LL +
                    field.skill)*100LL +
                    field.candBomb())*100LL +
                    field.blockNum())*100LL +
                    random()%64;

                Node nodeNew;
                nodeNew.score = score;
                field.save(&nodeNew.state);
                nodeNew.moves = node.moves;
                nodeNew.moves.push_back(m);

                beam.push_back(nodeNew);

                field.undo();
            }

            //  ここでスキルを使用した場合のお邪魔ブロック数が多ければ記録
            if (field.skill>=80)
            {
                Move bomb(0, 0, true);
                Result result = field.move(bomb, nullptr);

                if (!field.isDead() &&
                    result.ojama > maxOjama[depth])
                {
                    maxOjama[depth] = result.ojama;
                    maxMoves[depth] = node.moves;
                    maxMoves[depth].push_back(bomb);
                }

                field.undo();
            }
        }

        sort(beam.begin(), beam.end(),
            [](const Node &a, const Node &b) {return a.score > b.score;});
        if ((int)beam.size() > BeamBombWidth)
            beam.resize(BeamBombWidth);

        //  スキルを使用できなかった場合は
        //  最もスコアの高いノードの命令列を格納しておく
        if (maxOjama[depth]==-1)
        {
            maxOjama[depth] = 0;
            maxMoves[depth] = beam[0].moves;
        }
    }

    //  お邪魔ブロック数が100以上で最も浅いものを採用
    //  存在しなければお邪魔ブロック数が最も多いもの
    for (int depth=0; depth<BeamBombDepth; depth++)
        if (maxOjama[depth] >= 100)
        {
            moves = maxMoves[depth];
            return;
        }
    int bestDepth = 0;
    for (int depth=0; depth<BeamBombDepth; depth++)
        if (maxOjama[depth] > maxOjama[bestDepth])
            bestDepth = depth;
    moves = maxMoves[bestDepth];
}
