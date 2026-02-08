#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"
#include "Move.h"
#include <chrono>

class Search {
public:
    static const int INFINITE = 30000;
    static const int MATE = 29000;

    struct SearchParams {
        long nodes;
        int bestMove;
        int depthLimit;
        long long timeLimit; // in milliseconds
        long long startTime;
        bool stopped;
    };

    // Updated entry point
    static int iterativeDeepening(Board& board, int maxDepth, long long moveTime, bool verbose);

private:
    static int alphaBeta(Board& board, int alpha, int beta, int depth, bool doNull);
    static int quiescence(Board& board, int alpha, int beta);
    static void checkTime(); // Checks if we should stop the search

    static int scoreMove(const Board& board, int move, int pvMove);
    static void sortMoves(MoveList& moves, const Board& board, int pvMove, int ply);

    static SearchParams params;
};

#endif