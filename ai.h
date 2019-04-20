#pragma once

#include "status.h"

class AI
{
public:
    virtual void initialize(Status &status) = 0;
    virtual Move think(Status &status) = 0;
};
