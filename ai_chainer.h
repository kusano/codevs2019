#pragma once

#include "ai.h"

class AIChainer: AI
{
public:
    void initialize(Game &game, int seed) override;
    Move think(Game &game) override;
private:
    int search(int depth, int maxDepth, Field &field, char pack[][4],
        Move *move);
};
