#pragma once

//  命令
struct Move
{
    int pos;
    int rotate;
    bool bomb;

    Move(): pos(0), rotate(0), bomb(false) {}
    Move(int pos, int rotate, bool bomb):
        pos(pos), rotate(rotate), bomb(bomb) {}
};

//  1人分のフィールド
class Field
{
public:
    static const int W = 10;
    static const int H = 18;

    int time = 0;
    long long ojama = 0;
    int skill = 0;
    long long score = 0;
    char field[W][H] = {};
};
