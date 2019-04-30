#pragma once

#include "ai.h"
#include <random>

class AIAlice: AI
{
public:
    void initialize(Game &game, int seed) override;
    Move think(Game &game) override;

private:
    struct Moves
    {
        bool available = false;
        std::vector<Move> moves;
        long long ojama = 0;
    };

    std::mt19937 random;
    int rand = 0;
    Moves bestMoves;

    std::vector<Moves> generateChainMove(Game &game, int beamDepth,
        int beamWidth);
    std::vector<Moves> generateBombMove(Game &game, int beamDepth,
        int beamWidth);
    bool checkMoves(Game &game, Moves &moves);
};
