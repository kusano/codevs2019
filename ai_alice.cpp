#include "ai_alice.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <functional>
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
    cerr<<"turn: "<<game.turn<<endl;

    vector<vector<Result>> enemyResults = gaze(game, 2);
    cerr<<"gaze:"<<endl;
    for (int i=0; i<2; i++)
    {
        cerr<<(i==0 ? "ojama" : "skill");
        for (Result &result: enemyResults[i])
            cerr<<" ("<<result.ojamaRest<<", "<<result.skillReduce<<")";
        cerr<<endl;
    }

    if (checkMoves(game, enemyResults, bestMoves))
        cerr<<"check moves ok"<<endl;
    else
    {
        cerr<<"check moves failed"<<endl;

        //  命令列を再計算
        int width = game.turn==0 ? 4096 : 2048;
        vector<Moves> chain = generateChainMove(game, enemyResults, 12, width);
        vector<Moves> bomb = generateBombMove(game, enemyResults, 12, 256);

        //  1.1^i
        static double ojamaCoef[] = {
              1.00,   1.10,   1.21,   1.33,   1.46,   1.61,   1.77,   1.95,
              2.14,   2.36,   2.59,   2.85,   3.14,   3.45,   3.80,   4.18,
              4.59,   5.05,   5.56,   6.12,   6.73,   7.40,   8.14,   8.95,
              9.85,  10.83,  11.92,  13.11,  14.42,  15.86,  17.45,  19.19,
             21.11,  23.23,  25.55,  28.10,  30.91,  34.00,  37.40,  41.14,
             45.26,  49.79,  54.76,  60.24,  66.26,  72.89,  80.18,  88.20,
             97.02, 106.72, 117.39, 129.13, 142.04, 156.25, 171.87, 189.06,
            207.97, 228.76, 251.64, 276.80, 304.48, 334.93, 368.42, 405.27,
        };

        Moves best;
        double bestOjama = -1.0;
        bool isChain = false;

        for (Moves &m: chain)
        if (m.available)
        {
            double o = m.ojama/ojamaCoef[m.moves.size()];
            cerr
                <<"len: "<<m.moves.size()<<", "
                <<"ojama: "<<m.ojama<<", "
                <<"score: "<<o<<endl;
            if (!best.available ||
                 o > bestOjama)
            {
                best = m;
                bestOjama = o;
                isChain = true;
            }
        }
        for (Moves &m: bomb)
        if (m.available)
        {
            double o = m.ojama/ojamaCoef[m.moves.size()];
            cerr
                <<"len: "<<m.moves.size()<<", "
                <<"ojama: "<<m.ojama<<", "
                <<"score: "<<o<<endl;
            if (!best.available ||
                 o > bestOjama)
            {
                best = m;
                bestOjama = o;
                isChain = false;
            }
        }

        cerr<<"best moves: ("
            <<"length: "<<best.moves.size()<<", "
            <<"ojama: "<<best.ojama<<", "
            <<"score: "<<bestOjama<<", "
            <<(isChain ? "chain" : "bomb")
            <<")"<<endl;

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

//  相手の盤面をdepth手読み、お邪魔ブロック、スキル減少値が最大となるものを返す
//  ret[0]がお邪魔ブロック最大、ret[1]がスキル減少値最大
vector<vector<Result>> AIAlice::gaze(Game &game, int depth)
{
    Field &field = game.fields[1];

    vector<vector<Result>> ret(2);
    long long maxOjama = -1LL;
    int maxSkillReduce = -1;

    vector<Result> results;

    function<void(int)> search = [&](int d)
    {
        if (d>=depth)
        {
            long long ojama = 0;
            int skillReduce = 0;
            int n = (int)results.size();
            for (int i=0; i<n; i++)
            {
                //  速いターンに重みを付ける
                ojama += results[i].ojamaRest * (n-i);
                skillReduce += results[i].skillReduce * (n-i);
            }

            if (ojama > maxOjama)
            {
                maxOjama = ojama;
                ret[0] = results;
            }
            if (skillReduce > maxSkillReduce)
            {
                maxSkillReduce = skillReduce;
                ret[1] = results;
            }
        }
        else
        {
            for (int m=0; m<(Field::W-1)*4+1; m++)
            {
                Move move(m/4, m%4, m==(Field::W-1)*4);

                if (move.bomb && field.skill<80)
                    continue;

                Result result = field.move(move, game.packs[game.turn+d]);
                if (!field.isDead())
                {
                    results.push_back(result);
                    search(d+1);
                    results.pop_back();
                }
                field.undo();
            }
        }
    };
    search(0);

    return ret;
}

//  連鎖を狙う命令列を探索
//  返り値retの長さはbeamDepth+1で、ret[d].movesにはd手の命令が格納されている
vector<AIAlice::Moves> AIAlice::generateChainMove(Game &game,
    vector<vector<Result>> &enemyResults, int beamDepth, int beamWidth)
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
                //  相手がお邪魔ブロック数の最大化を目指してきたと想定
                if (depth<enemyResults[0].size())
                    field.interact(result, enemyResults[0][depth]);

                if (!field.isDead() &&
                    hash.count(field.hash)==0)
                {
                    //  連鎖候補は長く、高さは低く、ブロックは多く

                    //  お邪魔ブロックが1列降ってきても消せるようにする
                    //  パックの向きを調整すれば良いので、
                    //  お邪魔ブロックが無くてもたいていは消せるはず
                    field.ojama += 10;
                    int candChain = field.candChain();
                    field.ojama -= 10;
                    bestCandChain = max(bestCandChain, candChain);

                    long long score = (((
                        candChain)*100LL +
                        field.blockNum())*100LL +
                        -(max(0, field.maxHeight()-8)))*100LL +
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

        if (bestMoves[depth+1].available &&
            bestMoves[depth+1].ojama >= 160)
            break;
    }

    return bestMoves;
}

//  スキルを狙う命令列を探索
vector<AIAlice::Moves> AIAlice::generateBombMove(Game &game,
    vector<vector<Result>> &enemyResults, int beamDepth, int beamWidth)
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
                //  相手がスキル減少を目指してきたと想定
                if (depth<enemyResults[0].size())
                    field.interact(result, enemyResults[0][depth]);

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

        if (bestMoves[depth+1].available &&
            bestMoves[depth+1].ojama >= 160)
            break;
    }

    return bestMoves;
}

//  現在のフィールドでこの命令列が想定通りに動くか確認
bool AIAlice::checkMoves(Game &game, vector<vector<Result>> &enemyResults,
    Moves &moves)
{
    if (moves.moves.size()<4)
        return false;

    //  movesがスキル発動を目的としているならばスキル減少値が最大、
    //  連鎖目的ならばお邪魔ブロック数が最大の敵の動きを想定
    bool bomb = moves.moves.back().bomb;
    vector<Result> &enemyResult = bomb ? enemyResults[1] : enemyResults[0];

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
        if (i<enemyResult.size())
            field.interact(result, enemyResult[i]);
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
