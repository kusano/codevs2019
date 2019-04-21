#pragma once

#include "ai.h"

class AIChainer: AI
{
public:
    void initialize(Status &status) override;
    Move think(Status &status) override;
private:
    int search(int depth, int maxDepth, Field &field, char pack[][4],
        Move *move);
};
