#pragma once

#include <vector>
#include <string>

//  命令
struct Move
{
    int pos;
    int rotate;
    bool bomb;

    Move():
        pos(0), rotate(0), bomb(false) {}
    Move(int pos, int rotate, bool bomb):
        pos(pos), rotate(rotate), bomb(bomb) {}
};

//  結果
struct Result
{
    int chain = 0;

    Result(int chain):
        chain(chain) {}
};

//  履歴以外のフィールドの情報
class State
{
public:
    int time = 0;
    long long ojama = 0;
    int skill = 0;
    long long score = 0;
    //char field[Field::W][Field::H] = {};
    char field[10][19] = {};
};

//  1人分のフィールド
class Field
{
public:
    static const int W = 10;
    static const int H = 19;    //  16 + お邪魔 + ツモ

    int time = 0;
    long long ojama = 0;
    int skill = 0;
    long long score = 0;
    static_assert((char)-1 == -1, "char sign");
    char field[W][H] = {};

    Result move(Move move, char pack[4]);
    void undo();

    bool isDead();
    int candChain();
    int maxHeight();
    int blockNum();

    void save(State *state);
    void load(State &state);

    std::string toString();

private:
    struct Block
    {
        bool add;   //  move時に追加したブロックは真、削除は偽
        int x;
        int y;
        char block;

        Block(bool add, int x, int y, char block):
            add(add), x(x), y(y), block(block) {}
    };

    struct Pos
    {
        int x;
        int y;

        Pos(int x, int y):
            x(x), y(y) {}
    };

    //  undo用
    std::vector<long long> histOjama;
    std::vector<int> histSkill;
    std::vector<long long> histScore;
    std::vector<int> histBlockNum;
    std::vector<Block> histBlock;

    //  消去処理に使用
    static std::vector<Pos> erasePos;
};
