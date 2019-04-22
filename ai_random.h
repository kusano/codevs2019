#pragma once

#include "ai.h"
#include <random>

class AIRandom: AI
{
public:
    void initialize(Game &game, int seed) override;
    Move think(Game &game) override;

private:
    std::mt19937 random;
};
