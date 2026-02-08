#ifndef EVALUATION_H
#define EVALUATION_H

#include "Engine/Board.h"

class Evaluation {
public:
    // Piece Values (Used for Material initialization)
    static const int PAWN_VAL = 100;
    static const int KNIGHT_VAL = 429;
    static const int BISHOP_VAL = 439;
    static const int ROOK_VAL = 685;
    static const int QUEEN_VAL = 1368;
    static const int KING_VAL = 20000;

    // PST Arrays [PieceType][Square]
    static int PIECE_VALUES[14]; // used in Board
    static int PIECE_SQUARES_MG[14][64];
    static int PIECE_SQUARES_END[14][64];

    // Helpers
    static int MIRROR64[64];
    static int PHASE_INC[14]; // How much each piece contributes to game phase
    static const int TOTAL_PHASE = 24;

    static void initAll();
    static int evaluate(const Board& board);

    // Eval mirror testing
    static Board mirrorBoard(Board& board);
    static void testEval(std::string test_file);
};

#endif