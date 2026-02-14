#include "Engine/Evaluation.h"
#include "Engine/defs.h" // For numberOfTrailingZeros
#include "Engine/Zobrist.h"
#include <fstream>
#include <iostream>

int Evaluation::PIECE_SQUARES_MG[14][64];
int Evaluation::PIECE_SQUARES_END[14][64];
int Evaluation::PHASE_INC[14];
int Evaluation::MIRROR64[64] = {
    56, 57, 58, 59, 60, 61, 62, 63,
    48, 49, 50, 51, 52, 53, 54, 55,
    40, 41, 42, 43, 44, 45, 46, 47,
    32, 33, 34, 35, 36, 37, 38, 39,
    24, 25, 26, 27, 28, 29, 30, 31,
    16, 17, 18, 19, 20, 21, 22, 23,
     8,  9, 10, 11, 12, 13, 14, 15,
     0,  1,  2,  3,  4,  5,  6,  7
};

// Used in Board
int Evaluation::PIECE_VALUES[14] = { 0, 0, PAWN_VAL, -PAWN_VAL, KNIGHT_VAL, -KNIGHT_VAL, BISHOP_VAL,
                    -BISHOP_VAL, ROOK_VAL, -ROOK_VAL, QUEEN_VAL, -QUEEN_VAL, KING_VAL, -KING_VAL };

// ============================================================================
// PIECE SQUARE TABLES
// Format: 0..63 (A1..H8).
// The First line is RANK 1. The Last line is RANK 8.
// ============================================================================

// PAWN
const int mg_pawn_table[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    5,  10,  10, -20, -20,  10,  10,   5,
    5,  -5, -10,   0,   0, -10,  -5,   5,
    0,   0,   0,  20,  20,   0,   0,   0,
    5,   5,  10,  25,  25,  10,   5,   5,
    10,  10,  20,  30,  30,  20,  10,  10,
    50,  50,  50,  50,  50,  50,  50,  50,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int mg_knight_table[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

const int mg_bishop_table[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

const int mg_rook_table[64] = {
    0,   0,   0,   5,   5,   0,   0,   0,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
    5,  10,  10,  10,  10,  10,  10,   5,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int mg_queen_table[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -10,   5,   5,   5,   5,   5,   0, -10,
     0,   0,   5,   5,   5,   5,   0,  -5,
    -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

const int mg_king_table[64] = {
    20,  30,  10,   0,   0,  10,  30,  20,
    20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30
};

// End Game Kings usually want to be in the center
const int eg_king_table[64] = {
    -50, -30, -30, -30, -30, -30, -30, -50,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -50, -40, -30, -20, -20, -30, -40, -50
};


void Evaluation::initAll() {
    // 1. Initialize Phase Increments
    for (int i = 0; i < 14; i++) PHASE_INC[i] = 0;
    PHASE_INC[Board::WHITE_KNIGHT] = 1; PHASE_INC[Board::BLACK_KNIGHT] = 1;
    PHASE_INC[Board::WHITE_BISHOP] = 1; PHASE_INC[Board::BLACK_BISHOP] = 1;
    PHASE_INC[Board::WHITE_ROOK] = 2; PHASE_INC[Board::BLACK_ROOK] = 2;
    PHASE_INC[Board::WHITE_QUEEN] = 4; PHASE_INC[Board::BLACK_QUEEN] = 4;

    // 2. Initialize Tables
    for (int i = 0; i < 64; i++) {
        // Defaults: use MG table for End if no specific table exists
        // Pawns
        PIECE_SQUARES_MG[Board::WHITE_PAWN][i] = mg_pawn_table[i];
        PIECE_SQUARES_MG[Board::BLACK_PAWN][i] = mg_pawn_table[MIRROR64[i]];
        PIECE_SQUARES_END[Board::WHITE_PAWN][i] = mg_pawn_table[i];
        PIECE_SQUARES_END[Board::BLACK_PAWN][i] = mg_pawn_table[MIRROR64[i]];

        // Knights
        PIECE_SQUARES_MG[Board::WHITE_KNIGHT][i] = mg_knight_table[i];
        PIECE_SQUARES_MG[Board::BLACK_KNIGHT][i] = mg_knight_table[MIRROR64[i]];
        PIECE_SQUARES_END[Board::WHITE_KNIGHT][i] = mg_knight_table[i];
        PIECE_SQUARES_END[Board::BLACK_KNIGHT][i] = mg_knight_table[MIRROR64[i]];

        // Bishops
        PIECE_SQUARES_MG[Board::WHITE_BISHOP][i] = mg_bishop_table[i];
        PIECE_SQUARES_MG[Board::BLACK_BISHOP][i] = mg_bishop_table[MIRROR64[i]];
        PIECE_SQUARES_END[Board::WHITE_BISHOP][i] = mg_bishop_table[i];
        PIECE_SQUARES_END[Board::BLACK_BISHOP][i] = mg_bishop_table[MIRROR64[i]];

        // Rooks
        PIECE_SQUARES_MG[Board::WHITE_ROOK][i] = mg_rook_table[i];
        PIECE_SQUARES_MG[Board::BLACK_ROOK][i] = mg_rook_table[MIRROR64[i]];
        PIECE_SQUARES_END[Board::WHITE_ROOK][i] = mg_rook_table[i];
        PIECE_SQUARES_END[Board::BLACK_ROOK][i] = mg_rook_table[MIRROR64[i]];

        // Queens
        PIECE_SQUARES_MG[Board::WHITE_QUEEN][i] = mg_queen_table[i];
        PIECE_SQUARES_MG[Board::BLACK_QUEEN][i] = mg_queen_table[MIRROR64[i]];
        PIECE_SQUARES_END[Board::WHITE_QUEEN][i] = mg_queen_table[i];
        PIECE_SQUARES_END[Board::BLACK_QUEEN][i] = mg_queen_table[MIRROR64[i]];

        // Kings (Specific END table)
        PIECE_SQUARES_MG[Board::WHITE_KING][i] = mg_king_table[i];
        PIECE_SQUARES_MG[Board::BLACK_KING][i] = mg_king_table[MIRROR64[i]];
        PIECE_SQUARES_END[Board::WHITE_KING][i] = eg_king_table[i];
        PIECE_SQUARES_END[Board::BLACK_KING][i] = eg_king_table[MIRROR64[i]];
    }
}

/*Valuation Features */
//Eval material
void Evaluation::materialBalance(const Board& board, int& mg, int& eg){
	mg+= board.material[Board::WHITE] - board.material[Board::BLACK];
	eg+= board.material[Board::WHITE] - board.material[Board::BLACK];
}

void Evaluation::pieceSquares(const Board& board, int& mg, int& eg, int &gamePhase){
    U64 pieces = board.bitboards[Board::WHITE];

    while (pieces) {
        int sq = numberOfTrailingZeros(pieces);
        int piece = board.board[sq];

        mg+= PIECE_SQUARES_MG[piece][sq];
        eg+= PIECE_SQUARES_END[piece][sq];
        gamePhase += PHASE_INC[piece];
        pieces &= pieces - 1;
    }

    pieces = board.bitboards[Board::BLACK];
    while (pieces) {
        int sq = numberOfTrailingZeros(pieces);
        int piece = board.board[sq];

        // Tables for Black are already mirrored in initAll, 
        mg-= PIECE_SQUARES_MG[piece][sq];
        eg-= PIECE_SQUARES_END[piece][sq];
        gamePhase += PHASE_INC[piece];
        pieces &= pieces - 1;
    }
}


int Evaluation::evaluate(const Board& board) {
    int mg = 0;
    int eg = 0;
    int phase = 0;

    materialBalance(board, mg, eg);
    pieceSquares(board, mg, eg, phase);

    if (phase > TOTAL_PHASE) phase = TOTAL_PHASE;
    int score = ((mg * phase) + (eg * (TOTAL_PHASE - phase))) / TOTAL_PHASE;
    return (board.state.currentPlayer == Board::WHITE) ? score : -score;
}

/* Eval mirror testing */
Board Evaluation::mirrorBoard(Board& board) {
    Board mBoard;
    int tmpCastle = 0;
    int tmpEP = 0;

    //Castle
    if (board.state.white_can_castle_ks()) tmpCastle |= BoardState::BK_CASTLE;
    if (board.state.white_can_castle_qs()) tmpCastle |= BoardState::BQ_CASTLE;
    if (board.state.black_can_castle_ks()) tmpCastle |= BoardState::WK_CASTLE;
    if (board.state.black_can_castle_qs()) tmpCastle |= BoardState::WQ_CASTLE;

    //Ep square
    if (board.state.epSquare != 0)
        tmpEP = Evaluation::MIRROR64[board.state.epSquare];

    int mPieces[] = { 0, 0, Board::BLACK_PAWN, Board::WHITE_PAWN, Board::BLACK_KNIGHT, Board::WHITE_KNIGHT,
        Board::BLACK_BISHOP, Board::WHITE_BISHOP, Board::BLACK_ROOK, Board::WHITE_ROOK,
        Board::BLACK_QUEEN, Board::WHITE_QUEEN, Board::BLACK_KING, Board::WHITE_KING };

    //mirror pieces
    for (int i = 0; i < 64; i++)
        mBoard.board[i] = mPieces[board.board[Evaluation::MIRROR64[i]]];

    //setup bitboards
    for (int s = 0; s < 64; s++) {
        if (mBoard.board[s] == Board::EMPTY)
            continue;
        mBoard.bitboards[mBoard.board[s]] |= BitBoardGen::ONE << s;
        mBoard.bitboards[mBoard.board[s] & 1] |= BitBoardGen::ONE << s;
    }

    //material
    mBoard.material[0] = board.material[1];
    mBoard.material[1] = board.material[0];

    //king square
    mBoard.kingSQ[0] = numberOfTrailingZeros(mBoard.bitboards[Board::WHITE_KING]);
    mBoard.kingSQ[1] = numberOfTrailingZeros(mBoard.bitboards[Board::BLACK_KING]);

    int side = board.state.currentPlayer;
    mBoard.histPly = board.histPly;
    mBoard.state = BoardState(tmpEP, board.state.halfMoves, side ^ 1, tmpCastle, Zobrist::getKey(mBoard));
    mBoard.ply = 0;
    mBoard.hashTable = NULL;

    return mBoard;
}

void Evaluation::testEval(std::string test_file) {
    std::ifstream file(test_file);
    std::string line;
    int n = 0;

    printf("Running eval test\n");

    while (std::getline(file, line)) {
        Board board = FenParser::parseFEN(line);
        Board mBoard = mirrorBoard(board);
        int ev1 = evaluate(board);
        int ev2 = evaluate(mBoard);

        if (ev1 != ev2) {
            std::cout << "Eval test fail at FEN: " << line << std::endl;
            return;
        }
        n++;
    }
    printf("Evaluation ok: tested %d positions.\n", n);
}