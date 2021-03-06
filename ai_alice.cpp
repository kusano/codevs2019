#include "ai_alice.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <functional>
#include <cassert>

using namespace std;

#ifdef LOCAL
    #define THREAD_NUM 8
    #define BEAM_WIDTH_FIRST 45056
    #define BEAM_WIDTH 32768
#else
    #define THREAD_NUM 1
    #define BEAM_WIDTH_FIRST 4096
    #define BEAM_WIDTH 4096
#endif

#if THREAD_NUM>1
    #include <thread>
#endif

#ifndef LOCAL
//  標準エラー出力でエラーになる？
//  https://twitter.com/kakki_wriggle/status/1126145534018838529
//  https://twitter.com/kakki_wriggle/status/1126146120780926977
#include <sstream>
static stringstream cerr_dummy;
#define cerr cerr_dummy
#endif

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
        int width = game.turn==0 ?
            BEAM_WIDTH_FIRST :
            (int)(BEAM_WIDTH*(game.fields[0].time/180000.0));
        cerr<<"width: "<<width<<endl;
        int depth = game.turn==0 ? 14 : 12;
        vector<Moves> chain = generateChainMove(game, enemyResults, depth,
            width);
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
    //  各深さでお邪魔ブロック数が最大となる命令列を記録
    vector<Moves> bestMoves(beamDepth+1);

    vector<Node> beam(1);
    game.fields[0].save(&beam[0].state);

    Field fieldThread[THREAD_NUM];
    vector<Node> beamPre;
    vector<Node> beamPreThread[THREAD_NUM];
    vector<Node> beamThread[THREAD_NUM];
    set<unsigned long long> hash;

    for (int depth=0; depth<beamDepth; depth++)
    {
        beamPre.clear();
        beamPre.swap(beam);

        for (int i=0; i<THREAD_NUM; i++)
            beamPreThread[i].clear();
        for (size_t i=0; i<beamPre.size(); i++)
            beamPreThread[i%THREAD_NUM].push_back(beamPre[i]);
        Moves bestMovesThread[THREAD_NUM];
        int bestCandChainThread[THREAD_NUM] = {};

#if THREAD_NUM>1
        thread threads[THREAD_NUM];
        for (int i=0; i<THREAD_NUM; i++)
            threads[i] = std::move(thread([&](int id)
                {
                    generateChainMoveThread(game, depth, beamWidth,
                        beamPreThread[id], enemyResults, fieldThread[id],
                        &beamThread[id], &bestMovesThread[id],
                        &bestCandChainThread[id]);
                }, i));
        for (int i=0; i<THREAD_NUM; i++)
            threads[i].join();
#else
        generateChainMoveThread(game, depth, beamWidth, beamPreThread[0],
            enemyResults, fieldThread[0], &beamThread[0], &bestMovesThread[0],
            &bestCandChainThread[0]);
#endif

        hash.clear();
        int ti[THREAD_NUM] = {};
        for (int i=0; i<beamWidth; i++)
        {
            int t = -1;
            for (int j=0; j<THREAD_NUM; j++)
            {
                while (
                    ti[j]<(int)beamThread[j].size() &&
                    hash.count(beamThread[j][ti[j]].state.hash)!=0)
                    ti[j]++;
                if (ti[j]>=(int)beamThread[j].size())
                    continue;
                if (t==-1 ||
                    beamThread[j][ti[j]] < beamThread[t][ti[t]])
                    t = j;
            }
            if (t==-1)
                break;
            beam.push_back(beamThread[t][ti[t]]);
            hash.insert(beamThread[t][ti[t]].state.hash);
            ti[t]++;
        }

        int bestCandChain = 0;
        for (int i=0; i<THREAD_NUM; i++)
        {
            if (bestMovesThread[i].available)
                if (!bestMoves[depth+1].available ||
                    bestMovesThread[i].ojama > bestMoves[depth+1].ojama)
                    bestMoves[depth+1] = bestMovesThread[i];
            bestCandChain = max(bestCandChain, bestCandChainThread[i]);
        }

        cerr<<"depth: "<<depth<<", "
            <<"chain: "<<bestCandChain<<", "
            <<"ojama: "<<bestMoves[depth+1].ojama<<endl;

        if (bestMoves[depth+1].available &&
            bestMoves[depth+1].ojama >= 160)
            break;
    }

    return bestMoves;
}

//  各スレッドで実行
void AIAlice::generateChainMoveThread(const Game &game, int depth,
    int beamWidth, const vector<Node> &beamPre,
    const vector<vector<Result>> &enemyResults, Field &field,
    vector<Node> *beam, Moves *bestMoves, int *bestCandChain)
{
    vector<Node> beamTemp;
    Moves bestMovesTemp;
    int bestCandChainTemp = 0;

    set<unsigned long long> hash;

    for (const Node &node: beamPre)
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
                //  連鎖候補は長く、ブロックは多く、ブロックをかため、高さは低く

                //  お邪魔ブロックが1列降ってきても消せるようにする
                //  パックの向きを調整すれば良いので、
                //  お邪魔ブロックが無くてもたいていは消せるはず
                field.ojama += 10;
                int candChain = field.candChain();
                field.ojama -= 10;
                bestCandChainTemp = max(bestCandChainTemp, candChain);

                long long score = ((((
                    candChain)*100LL +
                    field.blockNum())*100LL +
                    -field.chunkNum())*100LL +
                    -(max(0, field.maxHeight()-8)))*100LL +
                    (long long)(field.hash & 0x3f ^ rand);

                vector<Move> moves = node.moves;
                moves.push_back(m);

                Node nodeNew;
                nodeNew.score = score;
                field.save(&nodeNew.state);
                nodeNew.moves = moves;

                beamTemp.push_back(nodeNew);
                hash.insert(field.hash);

                if (!bestMovesTemp.available ||
                    result.ojama > bestMovesTemp.ojama)
                {
                    bestMovesTemp.available = true;
                    bestMovesTemp.moves = moves;
                    bestMovesTemp.ojama = result.ojama;
                }
            }

            field.undo();
        }
    }

    //  統合後にbeamWidth番目以降のノードが選択されることは無いので、
    //  ここで切り捨てる
    sortAndCull(&beamTemp, beamWidth);

    *beam = beamTemp;
    *bestMoves = bestMovesTemp;
    *bestCandChain = bestCandChainTemp;
}

//  スキルを狙う命令列を探索
vector<AIAlice::Moves> AIAlice::generateBombMove(Game &game,
    vector<vector<Result>> &enemyResults, int beamDepth, int beamWidth)
{
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

        sortAndCull(&beam, beamWidth);

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

//  ソートして個数をsize個以下にする
void AIAlice::sortAndCull(std::vector<Node> *beam, int size)
{
    std::vector<Node *> v;
    for (Node &node: *beam)
        v.push_back(&node);

    sort(v.begin(), v.end(), [](Node *a, Node *b){return *a<*b;});
    if ((int)v.size() > size)
        v.resize(size);

    vector<Node> tmp;
    for (Node *p: v)
        tmp.push_back(*p);
    *beam = tmp;
}
