#pragma once

#include "field.h"

//  ゲーム全体の状態
class Game
{
public:
    static const int MaxTurn = 500;

    char packs[MaxTurn][4] = {};
    int turn = 0;
    Field fields[2];
};
