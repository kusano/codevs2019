#pragma once

#include "field.h"

//  ゲーム全体の状態
class Status
{
public:
    static const int maxTurn = 500;

    char packs[maxTurn][4] = {};
    int turn = 0;
    Field fields[2];
};
