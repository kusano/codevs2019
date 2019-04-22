#include "field.h"
#include <algorithm>
#include <sstream>
#include <cassert>

using namespace std;

vector<Field::Pos> Field::erasePos;

Result Field::move(Move move, char pack[4])
{
    //  命令前の各値を記憶
    histOjama.push_back(ojama);
    histSkill.push_back(skill);
    histScore.push_back(score);

    size_t blockNumBefore = histBlock.size();

    //  お邪魔ブロック
    if (ojama >= 10)
    {
        ojama -= 10;

        for (int x=0; x<W; x++)
        {
            for (int y=0; y<H; y++)
                if (field[x][y]==0)
                {
                    field[x][y] = 11;
                    histBlock.push_back(Block(true, x, y, 11));
                    break;
                }
        }
    }

    //  削除と落下
    auto erase = [&]() -> int
    {
        int num = 0;

#ifndef NDEBUG
        //  yの降順で並んでいないとundoに失敗する
        for (size_t i=0; i+1<erasePos.size(); i++)
        {
            assert(erasePos[i].y >= erasePos[i+1].y);
        }
#endif
        for (Pos &pos: erasePos)
        {
            //assert(field[pos.x][pos.y] != 0);
            if (field[pos.x][pos.y] != 0){
            histBlock.push_back(
                Block(false, pos.x, pos.y, field[pos.x][pos.y]));
            field[pos.x][pos.y] = 0;
            num++;
            }
        }
        erasePos.clear();

        for (int x=0; x<W; x++)
        {
            int fy = 0;
            for (int y=0; y<H; y++)
                if (field[x][y]==0)
                {
                    fy = max(fy, y+1);
                    while (fy<H && field[x][fy]==0)
                        fy++;
                    if (fy >= H)
                        break;
                    field[x][y] = field[x][fy];
                    field[x][fy] = 0;
                    fy++;
                }
        }

        return num;
    };

    if (move.bomb)
    {
        //  爆発
        //  上から順に消す
        for (int y=H-1; y>=0; y--)
        for (int x=0; x<W; x++)
            if (0<=field[x][y] && field[x][y]<=9)
            {
                bool e = false;
                for (int dx=-1; dx<=1 && !e; dx++)
                for (int dy=-1; dy<=1 && !e; dy++)
                {
                    int fx = x + dx;
                    int fy = y + dy;
                    if (0<=fx && fx<W &&
                        0<=fy && fy<H &&
                        field[fx][fy]==5)
                    {
                        e = true;
                    }
                }
                if (e)
                    erasePos.push_back(Pos(x, y));
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
                    field[move.pos+x][y] = p[x][0];
                    histBlock.push_back(Block(true, move.pos+x, y, p[x][y]));
                    field[move.pos+x][y+1] = p[x][1];
                    histBlock.push_back(Block(true, move.pos+x, y+1, p[x][y]));
                    break;
                }
        }
    }

    //  連鎖
    int chain = 0;

    while (true)
    {
        //  上から順に消す
        for (int y=H-1; y>=0; y--)
        for (int x=0; x<W; x++)
        {
            bool e = false;
            for (int dx=-1; dx<=1 && !e; dx++)
            for (int dy=-1; dy<=1 && !e; dy++)
            if (dx!=0 || dy!=0)
            {
                int tx = x + dx;
                int ty = y + dy;
                if (0<=tx && tx<W &&
                    0<=ty && ty<H &&
                    field[x][y]+field[tx][ty]==10)
                {
                    e = true;
                }
            }
            if (e)
                erasePos.push_back(Pos(x, y));
        }

        if (erasePos.empty())
            break;

        erase();
        chain++;
    }

    histBlockNum.push_back((int)(histBlock.size() - blockNumBefore));

    return Result(chain);
}

void Field::undo()
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

    //  各数値を戻す
    histOjama.push_back(ojama);
    histSkill.push_back(skill);
    histScore.push_back(score);
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
    //  TODO: move/undoを使うとお邪魔ブロックが落下し、無駄な処理がある
    int maxChain = 0;
    for (int x=0; x<W; x++)
    for (int b=0; b<H; b++)
    {
        char pack[4] = {};
        if (x<W-1)
            pack[2] = b;
        else
            pack[3] = b;
        Move move(x<W-1 ? x : x-1, 0, false);

        Result result = this->move(move, pack);
        undo();

        maxChain = max(maxChain, result.chain);
    }

    return maxChain;
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
}

void Field::load(State &state)
{
    time = state.time;
    ojama = state.ojama;
    skill = state.skill;
    score = state.score;
    memcpy(field, state.field, sizeof field);
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
