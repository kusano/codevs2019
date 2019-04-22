#pragma once

#include "ai.h"

class AIAlice: AI
{
public:
    void initialize(Game &game) override;
    Move think(Game &game) override;

private:
    static const int BeamDepth = 16;
    static const int BeamWidth = 128;

    std::vector<Move> moves;

    void generateMove(Game &game);
};
