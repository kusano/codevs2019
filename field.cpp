#include "field.h"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <cassert>
#include "table.h"

using namespace std;

//  candChain=trueならばcandChain専用の処理
Result Field::move(Move move, const char pack[4], bool candChain/*=false*/)
{
    assert(updatePos.empty());
    assert(erasePos.empty());

    if (!candChain)
    {
        //  命令前の各値を記憶
        histOjama.push_back(ojama);
        histSkill.push_back(skill);
        histScore.push_back(score);
    }
    histHash.push_back(hash);

    size_t blockNumBefore = histBlock.size();

    //  お邪魔ブロック
    if (ojama >= 10)
    {
        if (candChain)
        {
            //  ojamaの値を変動させない
            //  パックの位置にのみ落とす
            for (int p=move.pos*H; ; p++)
                if (field[0][p]==0)
                {
                    field[0][p] = 11;
                    //hash ^= hashField[p][11];
                    histBlock.push_back(Block(p, 11, true));
                    break;
                }
        }
        else
        {
            ojama -= 10;

            for (int x=0; x<W; x++)
            {
                for (int p=x*H; ; p++)
                    if (field[0][p]==0)
                    {
                        field[0][p] = 11;
                        hash ^= hashField[p][11];
                        histBlock.push_back(Block(p, 11, true));
                        break;
                    }
            }
        }
    }

    //  削除と落下
    auto erase = [&]() -> int
    {
        int num = 0;

        //  undoの処理のため、yの降順にソート
        sort(erasePos.begin(), erasePos.end(),
            [](const int a, const int b) {
                return positionToY[a] > positionToY[b];});

        int updateCol[W];
        for (int x=0; x<W; x++)
            updateCol[x] = H;

        for (int p: erasePos)
            //  重複して登録されている場合がある
            if (field[0][p] != 0)
            {
                histBlock.push_back(
                    Block(p, field[0][p], false));
                this->hash ^= hashField[p][field[0][p]];
                field[0][p] = 0;
                int x = positionToX[p];
                updateCol[x] = min(updateCol[x], positionToY[p]);
                num++;
            }
        erasePos.clear();

        for (int x=0; x<W; x++)
        {
            int fy = 0;
            for (int y=updateCol[x]; y<H; y++)
                if (field[x][y]==0)
                {
                    fy = max(fy, y+1);
                    while (fy<H && field[x][fy]==0)
                        fy++;
                    if (fy >= H)
                        break;
                    field[x][y] = field[x][fy];
                    this->hash ^= hashField[x*H+y][field[x][fy]];
                    updatePos.push_back(x*H+y);
                    this->hash ^= hashField[x*H+fy][field[x][fy]];
                    field[x][fy] = 0;
                    fy++;
                }
        }

        return num;
    };

    int bombNum = 0;

    if (move.bomb)
    {
        assert(skill >= 80);
        skill = 0;

        //  爆発
        for (int p=0; p<W*H; p++)
        {
            if (field[0][p]==5)
            {
                for (int d=0; d<8; d++)
                {
                    int n = neighbor8[p][d];
                    if (n!=p &&
                        0<=field[0][n] && field[0][n]<=9)
                        erasePos.push_back(n);
                }
            }
        }
        bombNum = erase();
    }
    else
    {
        if (candChain)
        {
            //  pack[0]を落とす
            for (int p=move.pos*H; ; p++)
                if (field[0][p]==0)
                {
                    field[0][p] = pack[0];
                    //hash ^= hashField[p][pack[0]];
                    updatePos.push_back(p);
                    histBlock.push_back(Block(p, pack[0], true));
                    break;
                }
        }
        else
        {
            //  パックの落下
            char p[2][2];
            static const int rotate[4][2][2] =
            {
                {{2, 0}, {3, 1}},
                {{3, 2}, {1, 0}},
                {{1, 3}, {0, 2}},
                {{0, 1}, {2, 3}},
            };
            for (int x=0; x<2; x++)
            for (int y=0; y<2; y++)
                p[x][y] = pack[rotate[move.rotate][x][y]];

            for (int x=0; x<2; x++)
                if (p[x][0]==0)
                {
                    p[x][0] = p[x][1];
                    p[x][1] = 0;
                }

            for (int x=0; x<2; x++)
            {
                for (int y=0; y<H; y++)
                    if (field[move.pos+x][y]==0)
                    {
                        for (int dy=0; dy<2; dy++)
                        {
                            field[move.pos+x][y+dy] = p[x][dy];
                            hash ^= hashField[(move.pos+x)*H+(y+dy)][p[x][dy]];
                            updatePos.push_back((move.pos+x)*H+(y+dy));
                            histBlock.push_back(
                                Block((move.pos+x)*H+(y+dy), p[x][dy], true));
                        }
                        break;
                    }
            }
        }
    }

    //  連鎖
    int chain = 0;

    while (true)
    {
        for (int &p: updatePos)
        {
            for (int d=0; d<8; d++)
            {
                int n = neighbor8[p][d];
                if (n!=p&&
                    field[0][p]+field[0][n]==10)
                {
                    erasePos.push_back(p);
                    erasePos.push_back(n);
                }
            }
        }
        updatePos.clear();

        if (erasePos.empty())
            break;

        erase();
        chain++;
    }

    histBlockNum.push_back((int)(histBlock.size() - blockNumBefore));

    if (candChain)
    {
        //  連鎖数のみを返す
        return Result(chain, 0LL, 0L, 0);
    }
    else
    {
        //  お邪魔ブロックストック
        long long oj = chainScores[chain]/2 + bombScore[bombNum]/2;
        long long rest = max(0LL, oj-ojama);
        ojama = max(0LL, ojama-oj);

        //  スキルゲージ
        if (chain>0)
            skill = min(100, skill+8);
        int reduce = chain >= 3 ? 12+2*chain : 0;

        return Result(chain, oj, rest, reduce);
    }
}

//  お邪魔ブロックとスキルゲージのやりとり
//  moveと合わせてundoされる
void Field::interact(Result self, Result enemy)
{
    ojama += max(0LL, enemy.ojamaRest-self.ojamaRest);
    skill = max(0, skill-enemy.skillReduce);
}

//  candChain=trueならばcandChain専用の処理
void Field::undo(bool candChain/*=false*/)
{
    //  ブロックを元に戻す
    int num = histBlockNum.back();
    histBlockNum.pop_back();

    for (int i=0; i<num; i++)
    {
        Block block = histBlock.back();
        histBlock.pop_back();

        if (block.add)
        {
            field[0][block.pos] = 0;
        }
        else
        {
            int p = block.pos;
            char b = block.block;
            while (b != 0)
            {
                swap(b, field[0][p]);
                p++;
            }
        }
    }

    if (!candChain)
    {
        //  各数値を戻す
        ojama = histOjama.back();
        histOjama.pop_back();
        skill = histSkill.back();
        histSkill.pop_back();
        score = histScore.back();
        histScore.pop_back();
    }
    hash = histHash.back();
    histHash.pop_back();
}

bool Field::isDead()
{
    for (int x=0; x<W; x++)
        if (field[x][16] != 0)
            return true;
    return false;
}

//  ブロックを1個落としたときの最大チェイン数を返す
int Field::candChain()
{
    char pack[4] = {};

    int maxChain = 0;
    for (int x=0; x<W; x++)
    {
        //  連鎖になる数字のみ試す
        bool c[10] = {};
        int y = 0;
        while (field[x][y]!=0)
            y++;
        if (ojama >= 10)
            y++;
        int p = x*H+y;
        for (int d=0; d<8; d++)
        {
            int n = neighbor8[p][d];
            if (n!=p &&
                1<=field[0][n] && field[0][n]<=9)
                c[10-field[0][n]] = true;
        }

        for (int b=1; b<=9; b++)
        if (c[b])
        {
            pack[0] = b;
            Move move(x, 0, false);

            Result result = this->move(move, pack, true);
            undo(true);

            maxChain = max(maxChain, result.chain);
        }
    }

    return maxChain;
}

//  スキルを発動させたときに消えるブロック数を返す
int Field::candBomb()
{
    for (int p=0; p<W*H; p++)
        if (field[0][p]==5)
        {
            for (int d=0; d<8; d++)
            {
                int n = neighbor8[p][d];
                if (n!=p &&
                    1<=field[0][n] && field[0][n]<=9)
                {
                    eraseBlock[p] = true;
                }
            }
        }

    int num = 0;
    for (int p=0; p<W*H; p++)
        if (eraseBlock[p])
        {
            eraseBlock[p] = false;
            num++;
        }
    return num;
}

int Field::maxHeight()
{
    for (int y=H-1; y>=0; y--)
        for (int x=0; x<W; x++)
            if (field[x][y] != 0)
                return y+1;
    return 0;
}

int Field::blockNum()
{
    int n = 0;
    for (int p=0; p<W*H; p++)
        if (field[0][p] != 0)
            n++;
    return n;
}

void Field::save(State *state)
{
    state->time = time;
    state->ojama = ojama;
    state->skill = skill;
    state->score = score;
    memcpy(state->field, field, sizeof state->field);
    state->hash = hash;
}

void Field::load(const State &state)
{
    time = state.time;
    ojama = state.ojama;
    skill = state.skill;
    score = state.score;
    memcpy(field, state.field, sizeof field);
    hash = state.hash;
}

string Field::toString()
{
    stringstream ss;

    ss<<time<<endl;
    ss<<ojama<<endl;
    ss<<skill<<endl;
    ss<<score<<endl;

    for (int y=0; y<H; y++)
    {
        for (int x=0; x<W; x++)
        {
            if (x > 0)
                ss<<" ";
            char c = field[x][H-y-1];
            switch (c)
            {
            case 0: ss<<"."; break;
            case 11: ss<<"#"; break;
            default: ss<<(int)c; break;
            }
        }
        ss<<endl;
    }

    return ss.str();
}
