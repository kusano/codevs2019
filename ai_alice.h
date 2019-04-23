#pragma once

#include "ai.h"
#include <random>

class AIAlice: AI
{
public:
    void initialize(Game &game, int seed) override;
    Move think(Game &game) override;

private:
    static const int BeamChainDepth = 16;
    static const int BeamChainWidth = 256;
    static const int BeamBombDepth = 16;
    static const int BeamBombWidth = 256;

    std::mt19937 random;
    std::vector<Move> moves;

    void generateChainMove(Game &game);
    void generateBombMove(Game &game);
};
