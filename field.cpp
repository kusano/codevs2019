#include "field.h"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <cassert>
#include "table.h"

using namespace std;

vector<Field::Pos> Field::updatePos;
vector<Field::Pos> Field::erasePos;

//  candChain=trueならばcandChain専用の処理
Result Field::move(Move move, char pack[4], bool candChain/*=false*/)
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
            for (int y=0; y<H; y++)
                if (field[move.pos][y]==0)
                {
                    field[move.pos][y] = 11;
                    //hash ^= hashField[move.pos][y][11];
                    histBlock.push_back(Block(true, move.pos, y, 11));
                    break;
                }
        }
        else
        {
            ojama -= 10;

            for (int x=0; x<W; x++)
            {
                for (int y=0; y<H; y++)
                    if (field[x][y]==0)
                    {
                        field[x][y] = 11;
                        hash ^= hashField[x][y][11];
                        histBlock.push_back(Block(true, x, y, 11));
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
            [](const Pos &a, const Pos &b) {return a.y > b.y;});

        int updateCol[W];
        for (int x=0; x<W; x++)
            updateCol[x] = H;

        for (Pos &pos: erasePos)
            //  重複して登録されている場合がある
            if (field[pos.x][pos.y] != 0)
            {
                histBlock.push_back(
                    Block(false, pos.x, pos.y, field[pos.x][pos.y]));
                this->hash ^= hashField[pos.x][pos.y][field[pos.x][pos.y]];
                field[pos.x][pos.y] = 0;
                updateCol[pos.x] = min(updateCol[pos.x], pos.y);
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
                    this->hash ^= hashField[x][y][field[x][fy]];
                    updatePos.push_back(Pos(x, y));
                    this->hash ^= hashField[x][fy][field[x][fy]];
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
        for (int x=0; x<W; x++)
        for (int y=0; y<H; y++)
        {
            if (field[x][y]==5)
            {
                for (int dx=-1; dx<=1; dx++)
                for (int dy=-1; dy<=1; dy++)
                {
                    int tx = x + dx;
                    int ty = y + dy;
                    if (0<=tx && tx<W &&
                        0<=ty && ty<H &&
                        0<=field[tx][ty] && field[tx][ty]<=9)
                    {
                        erasePos.push_back(Pos(tx, ty));
                    }
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
            for (int y=0; y<H; y++)
                if (field[move.pos][y]==0)
                {
                    field[move.pos][y] = pack[0];
                    //hash ^= hashField[move.pos][y][pack[0]];
                    updatePos.push_back(Pos(move.pos, y));
                    histBlock.push_back(Block(true, move.pos, y, pack[0]));
                    break;
                }
        }
        else
        {
            //  パックの落下
            char p[2][2];
            static int rotate[4][2][2] =
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
                            hash ^= hashField[move.pos+x][y+dy][p[x][dy]];
                            updatePos.push_back(Pos(move.pos+x, y+dy));
                            histBlock.push_back(
                                Block(true, move.pos+x, y+dy, p[x][dy]));
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
        for (Pos &p: updatePos)
        {
            int x = p.x;
            int y = p.y;

            for (int dx=-1; dx<=1; dx++)
            for (int dy=-1; dy<=1; dy++)
            if (!(dx==0 && dy==0))
            {
                int tx = x + dx;
                int ty = y + dy;
                if (0<=tx && tx<W &&
                    0<=ty && ty<H &&
                    field[x][y]+field[x+dx][y+dy]==10)
                {
                    erasePos.push_back(Pos(x, y));
                    erasePos.push_back(Pos(x+dx, y+dy));
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
            field[block.x][block.y] = 0;
        }
        else
        {
            int y = block.y;
            char b = block.block;
            while (b != 0)
            {
                swap(b, field[block.x][y]);
                y++;
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
    for (int b=1; b<=9; b++)
    {
        pack[0] = b;
        Move move(x, 0, false);

        Result result = this->move(move, pack, true);
        undo(true);

        maxChain = max(maxChain, result.chain);
    }

    return maxChain;
}

//  スキルを発動させたときに消えるブロック数を返す
int Field::candBomb()
{
    static bool erase[W][H] = {};

    for (int x=0; x<W; x++)
    for (int y=0; y<H; y++)
        if (field[x][y]==5)
        {
            for (int dx=-1; dx<=1; dx++)
            for (int dy=-1; dy<=1; dy++)
            {
                int tx=x+dx;
                int ty=y+dy;
                if (0<=tx && tx<W &&
                    0<=ty && ty<H &&
                    1<=field[tx][ty] && field[tx][ty]<=9)
                {
                    erase[tx][ty] = true;
                }
            }
        }

    int num = 0;
    for (int x=0; x<W; x++)
    for (int y=0; y<H; y++)
        if (erase[x][y])
        {
            erase[x][y] = false;
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
    for (int x=0; x<W; x++)
    for (int y=0; y<H; y++)
        if (field[x][y] != 0)
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

void Field::load(State &state)
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
