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
    if (!checkMoves(game, bestMoves))
    {
        //  命令列を再計算
        vector<Moves> chain = generateChainMove(game, 16, 256);
        vector<Moves> bomb = generateBombMove(game, 16, 256);

        //  chainから最もお邪魔ブロック数が多いものを選択
        //  TODO: 浅いものを優先するべき？
        Moves best;
        for (Moves &m: chain)
            if (m.available)
                if (!best.available ||
                    m.ojama > best.ojama)
                    best = m;

        //  より手数が短くお邪魔ブロック数が多いものがbombにあれば選択
        //  bombは妨害されやすいので無条件に選択するとハマりそう
        size_t chainDepth = best.moves.size();
        for (size_t d=0; d<=chainDepth; d++)
            if (d<bomb.size() && bomb[d].available)
                if (bomb[d].ojama > best.ojama)
                    best = bomb[d];

        bestMoves = best;
    }

    Move move;
    if (bestMoves.available &&
        !bestMoves.moves.empty())
    {
        move = bestMoves.moves[0];
        bestMoves.moves.erase(bestMoves.moves.begin());
    }
    else
    {
        //  どの命令でも死ぬ場合
        move = Move(0, 0, false);
    }
    return move;
}

//  連鎖を狙う命令列を探索
//  返り値retの長さはbeamDepth+1で、ret[d].movesにはd手の命令が格納されている
vector<AIAlice::Moves> AIAlice::generateChainMove(Game &game, int beamDepth,
    int beamWidth)
{
    struct Node
    {
        long long score = 0;
        //  TODO: stateを記録するより、movesから生成した方が良い？
        State state;
        std::vector<Move> moves;
    };

    //  各深さでお邪魔ブロック数が最大となる命令列を記録
    vector<Moves> bestMoves(beamDepth+1);

    vector<Node> beam(1);
    game.fields[0].save(&beam[0].state);

    Field field;

    for (int depth=0; depth<beamDepth; depth++)
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

                if (!field.isDead())
                {
                    //  連鎖候補は長く、高さは低く、ブロックは多く
                    long long score = (((
                        field.candChain())*100LL +
                        -field.maxHeight())*100LL +
                        field.blockNum())*100LL +
                        random()%64;

                    vector<Move> moves = node.moves;
                    moves.push_back(m);

                    Node nodeNew;
                    nodeNew.score = score;
                    field.save(&nodeNew.state);
                    nodeNew.moves = moves;

                    beam.push_back(nodeNew);

                    if (!bestMoves[depth+1].available ||
                        result.ojama > bestMoves[depth+1].ojama)
                    {
                        bestMoves[depth+1].available = true;
                        bestMoves[depth+1].moves = moves;
                        bestMoves[depth+1].ojama = result.ojama;
                    }
                }

                field.undo();
            }
        }

        sort(beam.begin(), beam.end(),
            [](const Node &a, const Node &b) {return a.score > b.score;});
        if ((int)beam.size() > beamWidth)
            beam.resize(beamWidth);
    }

    return bestMoves;
}

//  スキルを狙う命令列を探索
vector<AIAlice::Moves> AIAlice::generateBombMove(Game &game, int beamDepth,
    int beamWidth)
{
    struct Node
    {
        long long score = 0;
        //  TODO: stateを記録するより、movesから生成した方が良い？
        State state;
        std::vector<Move> moves;
    };

    //  各深さでお邪魔ブロック数が最大となる命令列を記録
    vector<Moves> bestMoves(beamDepth+1);

    vector<Node> beam(1);
    game.fields[0].save(&beam[0].state);

    Field field;

    for (int depth=0; depth<beamDepth; depth++)
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

                if (!field.isDead())
                {
                    //  スキルゲージ、消せるブロック、その他のブロックを多く
                    long long score = (((
                        field.skill)*100LL +
                        field.candBomb())*100LL +
                        field.blockNum())*100LL +
                        random()%64;

                    vector<Move> moves = node.moves;
                    moves.push_back(m);

                    Node nodeNew;
                    nodeNew.score = score;
                    field.save(&nodeNew.state);
                    nodeNew.moves = moves;

                    beam.push_back(nodeNew);
                }

                field.undo();
            }

            //  ここでスキルを使用した場合のお邪魔ブロック数
            if (field.skill>=80)
            {
                Move bomb(0, 0, true);
                Result result = field.move(bomb, nullptr);

                if (!field.isDead())
                {
                    if (!bestMoves[depth+1].available ||
                        result.ojama > bestMoves[depth+1].ojama)
                    {
                        bestMoves[depth+1].available = true;
                        bestMoves[depth+1].moves = node.moves;
                        bestMoves[depth+1].moves.push_back(bomb);
                        bestMoves[depth+1].ojama = result.ojama;
                    }
                }

                field.undo();
            }
        }

        sort(beam.begin(), beam.end(),
            [](const Node &a, const Node &b) {return a.score > b.score;});
        if ((int)beam.size() > beamWidth)
            beam.resize(beamWidth);

        //  スキルを使用できなかった場合は
        //  最もスコアの高いノードの命令列を格納しておく
        if (!bestMoves[depth+1].available &&
            !beam.empty())
        {
            bestMoves[depth+1].available = true;
            bestMoves[depth+1].moves = beam[0].moves;
            bestMoves[depth+1].ojama = 0;
        }
    }

    return bestMoves;
}

//  現在のフィールドでこの命令列が想定通りに動くか確認
bool AIAlice::checkMoves(Game &game, Moves &moves)
{
    if (moves.moves.empty())
        return false;

    Field &field = game.fields[0];

    bool ok = true;
    int count = 0;
    for (size_t i=0; i<moves.moves.size(); i++)
    {
        //  スキルが使用できる
        if (moves.moves[i].bomb &&
            field.skill < 80)
        {
            ok = false;
            break;
        }

        Result result = field.move(
            moves.moves[i],
            game.packs[game.turn+i]);
        count++;

        //  死なない
        if (field.isDead())
        {
            ok = false;
            break;
        }

        //  最後の命令ならば発生お邪魔ブロック数が想定通り
        if (i+1==moves.moves.size() &&
            result.ojama != moves.ojama)
        {
            ok = false;
            break;
        }
    }

    for (int i=0; i<count; i++)
        field.undo();

    return ok;
}
