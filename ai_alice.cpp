#include "ai_alice.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <cassert>

using namespace std;

void AIAlice::initialize(Game &game, int seed)
{
    random.seed(seed);
    rand = random()%64;
};

Move AIAlice::think(Game &game)
{
    cerr<<"think start"<<endl;

    if (checkMoves(game, bestMoves))
        cerr<<"check moves ok"<<endl;
    else
    {
        cerr<<"check moves failed"<<endl;

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
        cerr<<"best chain moves: ("
            <<"length: "<<best.moves.size()<<", "
            <<"ojama: "<<best.ojama<<")"<<endl;

        //  より手数が短くお邪魔ブロック数が多いものがbombにあれば選択
        //  bombは妨害されやすいので無条件に選択するとハマりそう
        int depth = max(4, (int)best.moves.size());
        bool replaced = false;
        for (int d=0; d<=depth; d++)
            if (d<bomb.size() && bomb[d].available)
                if (bomb[d].ojama > best.ojama)
                {
                    best = bomb[d];
                    replaced = true;
                }
        if (replaced)
            cerr<<"replaced with bomb moves: ("
                <<"length: "<<best.moves.size()<<", "
                <<"ojama: "<<best.ojama<<")"<<endl;

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
        cerr<<"no moves"<<endl;

        move = Move(0, 0, false);
    }

    cerr<<"think finished"<<endl;
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
    long long maxOjama = 0;

    for (int depth=0; depth<beamDepth && maxOjama<30; depth++)
    {
        vector<Node> beamPre;
        beamPre.swap(beam);
        set<unsigned long long> hash;

        int bestCandChain = 0;

        for (Node &node: beamPre)
        {
            field.load(node.state);

            for (int pos=0; pos<9; pos++)
            for (int rotate=0; rotate<4; rotate++)
            {
                //  TODO: isDead/500ターン以上の考慮
                Move m(pos, rotate, false);

                Result result = field.move(m, game.packs[game.turn+depth]);

                if (!field.isDead() &&
                    hash.count(field.hash)==0)
                {
                    //  連鎖候補は長く、高さは低く、ブロックは多く
                    int candChain = field.candChain();
                    bestCandChain = max(bestCandChain, candChain);

                    long long score = (((
                        candChain)*100LL +
                        -field.maxHeight())*100LL +
                        field.blockNum())*100LL +
                        (long long)(field.hash & 0x3f ^ rand);

                    vector<Move> moves = node.moves;
                    moves.push_back(m);

                    Node nodeNew;
                    nodeNew.score = score;
                    field.save(&nodeNew.state);
                    nodeNew.moves = moves;

                    beam.push_back(nodeNew);
                    hash.insert(field.hash);

                    if (!bestMoves[depth+1].available ||
                        result.ojama > bestMoves[depth+1].ojama)
                    {
                        bestMoves[depth+1].available = true;
                        bestMoves[depth+1].moves = moves;
                        bestMoves[depth+1].ojama = result.ojama;
                        maxOjama = max(maxOjama, result.ojama);
                    }
                }

                field.undo();
            }
        }

        sort(beam.begin(), beam.end(),
            [](const Node &a, const Node &b) {return a.score > b.score;});
        if ((int)beam.size() > beamWidth)
            beam.resize(beamWidth);

        cerr<<"depth: "<<depth<<", "
            <<"chain: "<<bestCandChain<<", "
            <<"ojama: "<<bestMoves[depth+1].ojama<<endl;
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
    long long maxOjama = 0;

    for (int depth=0; depth<beamDepth && maxOjama<30; depth++)
    {
        vector<Node> beamPre;
        beamPre.swap(beam);
        set<unsigned long long> hash;

        for (Node &node: beamPre)
        {
            field.load(node.state);

            for (int pos=0; pos<9; pos++)
            for (int rotate=0; rotate<4; rotate++)
            {
                //  TODO: isDead/500ターン以上の考慮
                Move m(pos, rotate, false);

                Result result = field.move(m, game.packs[game.turn+depth]);

                if (!field.isDead() &&
                    hash.count(field.hash)==0)
                {
                    //  スキルゲージ、消せるブロック、その他のブロックを多く
                    long long score = (((
                        field.skill)*100LL +
                        field.candBomb())*100LL +
                        field.blockNum())*100LL +
                        (long long)(field.hash & 0x3f ^ rand);

                    vector<Move> moves = node.moves;
                    moves.push_back(m);

                    Node nodeNew;
                    nodeNew.score = score;
                    field.save(&nodeNew.state);
                    nodeNew.moves = moves;

                    beam.push_back(nodeNew);
                    hash.insert(field.hash);
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
                        maxOjama = max(maxOjama, result.ojama);
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

        cerr<<"depth: "<<depth<<", "
            <<"ojama: "<<bestMoves[depth+1].ojama<<endl;
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
