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

    struct Node
    {
        long long score = 0;
        //  TODO: stateを記録するより、movesから生成した方が良い？
        State state;
        std::vector<Move> moves;

        bool operator<(const Node &node) {
            return this->score > node.score;
        }
    };

    std::mt19937 random;
    int rand = 0;
    Moves bestMoves;

    std::vector<std::vector<Result>> gaze(Game &game, int depth);
    std::vector<Moves> generateChainMove(Game &game,
        std::vector<std::vector<Result>> &enemyResults,
        int beamDepth, int beamWidth);
    void generateChainMoveThread(const Game &game, int depth, int beamWidth,
        const std::vector<Node> &beamPre,
        const std::vector<std::vector<Result>> &enemyResults, Field &field,
        std::vector<Node> *beam, Moves *bestMoves, int *bestCandChain);
    std::vector<Moves> generateBombMove(Game &game,
        std::vector<std::vector<Result>> &enemyResults,
        int beamDepth, int beamWidth);
    bool checkMoves(Game &game, std::vector<std::vector<Result>> &enemyResults,
        Moves &moves);
    void sortAndCull(std::vector<Node> *beam, int size);
};
