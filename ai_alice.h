#pragma once

#include "ai.h"
#include <random>

class AIAlice: AI
{
public:
    void initialize(Game &game, int seed) override;
    Move think(Game &game) override;

private:
    static const int BeamDepth = 16;
    static const int BeamWidth = 128;

    std::mt19937 random;
    std::vector<Move> moves;

    void generateMove(Game &game);
};
