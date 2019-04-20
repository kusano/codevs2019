#pragma once

#include "ai.h"
#include <random>

class AIRandom: AI
{
public:
    void initialize(Status &status) override;
    Move think(Status &status) override;

private:
    std::mt19937 random;
};
