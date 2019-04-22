#pragma once

#include "game.h"

class AI
{
public:
    virtual void initialize(Game &game, int seed) = 0;
    virtual Move think(Game &game) = 0;
};
