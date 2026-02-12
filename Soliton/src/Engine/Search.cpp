#include "Engine/Search.h"
#include "Engine/Evaluation.h"
#include "Engine/MoveGen.h"
#include "Engine/HashTable.h"
#include <iostream>
#include <algorithm>
#include <vector>

Search::SearchParams Search::params;

void Search::stop() {
    params.stopped = true;
}

// Helper to get current time in milliseconds
long long currentTimeMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void Search::checkTime() {
    if (params.timeLimit != -1) {
        if (currentTimeMillis() - params.startTime >= params.timeLimit) {
            params.stopped = true;
        }
    }
}

// MVV-LVA table [Victim][Attacker]
static int MVV_LVA[14][14];
static bool mvv_init = false;

void init_mvv() {
    if (mvv_init) return;
    int values[] = { 0, 0, 100, 100, 200, 200, 300, 300, 400, 400, 500, 500, 600, 600 };
    for (int v = 2; v < 14; v++) {
        for (int a = 2; a < 14; a++) {
            MVV_LVA[v][a] = values[v] + 10 - (values[a] / 100);
        }
    }
    mvv_init = true;
}

int Search::iterativeDeepening(Board& board, int maxDepth, long long moveTime, bool verbose) {
    init_mvv();
    params.nodes = 0;
    params.stopped = false;
    params.bestMove = Move::NO_MOVE;
    params.startTime = currentTimeMillis();
    params.timeLimit = moveTime;
    params.depthLimit = maxDepth;

    int alpha = -INFINITE;
    int beta = INFINITE;

    for (int d = 1; d <= params.depthLimit; d++) {
        board.ply = 0;
        int score = alphaBeta(board, alpha, beta, d, true);

        // If search was stopped during this depth, don't use the results
        if (params.stopped) break;

        int pvCount = HashTable::getPVLine(d, board);

        if (verbose) {
            std::cout << "info depth " << d << " score cp " << score << " nodes " << params.nodes
                << " time " << (currentTimeMillis() - params.startTime) << " pv ";

            for (int i = 0; i < pvCount; i++) {
                std::cout << Move::toLongNotation(board.pvArray[i]) << " ";
            }
            std::cout << std::endl;
        }
        params.bestMove = board.pvArray[0];
        if (score > MATE || score < -MATE) break;
    }

    // Final output: ensure we output a bestmove even if search was stopped
    if (params.bestMove == Move::NO_MOVE) {
        // Fallback: just get any legal move if something went wrong
        MoveList moves;
        MoveGen::pseudoLegalMoves(&board, board.state.currentPlayer, moves, false);
        for (int i = 0; i < moves.size(); i++) {
            BoardState undo = board.makeMove(moves.get(i));
            if (undo.valid) {
                params.bestMove = moves.get(i);
                board.undoMove(moves.get(i), undo);
                break;
            }
        }
    }

    if (verbose)
        std::cout << "bestmove " << Move::toLongNotation(params.bestMove) << std::endl;
    return params.bestMove;
}

int Search::iterativeDeepeningScore(Board& board, int maxDepth, long long moveTime, bool verbose) {
    init_mvv();
    params.nodes = 0;
    params.stopped = false;
    params.bestMove = Move::NO_MOVE;
    params.startTime = currentTimeMillis();
    params.timeLimit = moveTime;
    params.depthLimit = maxDepth;

    int alpha = -INFINITE;
    int beta = INFINITE;
    int finalScore = INVALID_SCORE;

    for (int d = 1; d <= params.depthLimit; d++) {
        board.ply = 0;
        int score = alphaBeta(board, alpha, beta, d, true);

        if (params.stopped) break;
        finalScore = score;

        if (score > MATE || score < -MATE) break;
    }
    return finalScore;
}

int Search::alphaBeta(Board& board, int alpha, int beta, int depth, bool doNull) {
    // Check time every 2048 nodes to avoid overhead of system clock calls
    if ((params.nodes & 2047) == 0) checkTime();
    if (params.stopped) return 0;

    params.nodes++;
    if (depth <= 0) return quiescence(board, alpha, beta);

    if ((board.state.halfMoves >= 100 || board.isRepetition()) && board.ply){
        return 0;
    }

    // Safety check for search depth to prevent stack overflow in extreme tactical scenarios
    if (board.ply >= Board::MAX_DEPTH - 1) {
        return Evaluation::evaluate(board);
    }

    int pvMove = Move::NO_MOVE;
    int hashScore = 0;
    if (HashTable::probeHashEntry(board, &pvMove, &hashScore, alpha, beta, depth)) {
        return hashScore;
    }

    int side = board.state.currentPlayer;
    bool inCheck = MoveGen::isSquareAttacked(&board, board.kingSQ[side], side ^ 1);

    if (doNull && !inCheck && depth >= 3 && board.material[side] > 500) {
        BoardState undo = board.makeNullMove();
        int score = -alphaBeta(board, -beta, -beta + 1, depth - 3, false);
        board.undoNullMove(undo);
        if (params.stopped) return 0;
        if (score >= beta) return beta;
    }

    MoveList moves;
    MoveGen::pseudoLegalMoves(&board, side, moves, inCheck);
    sortMoves(moves, board, pvMove, board.ply);

    int legalMovesCount = 0;
    int oldAlpha = alpha;
    int bestMove = Move::NO_MOVE;

    for (int i = 0; i < moves.size(); i++) {
        int move = moves.get(i);
        BoardState undo = board.makeMove(move);
        if (!undo.valid) continue;

        legalMovesCount++;
        board.ply++;
        int score = -alphaBeta(board, -beta, -alpha, depth - 1, true);
        board.ply--;
        board.undoMove(move, undo);

        if (params.stopped) return 0;

        if (score >= beta) {
            if (Move::captured(move) == Board::EMPTY) {
                board.searchKillers[1][board.ply] = board.searchKillers[0][board.ply];
                board.searchKillers[0][board.ply] = move;

                 // History heuristic
                int piece = board.board[Move::from(bestMove)];
                board.searchHistory[piece][Move::to(bestMove)] += depth * depth;
            }
            HashTable::storeHashEntry(board, move, beta, HFBETA, depth);
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            bestMove = move;
        }
    }

    if (legalMovesCount == 0) {
        return inCheck ? (-MATE + board.ply) : 0;
    }

    int flag = (alpha > oldAlpha) ? HFEXACT : HFALPHA;
    HashTable::storeHashEntry(board, bestMove, alpha, flag, depth);
    return alpha;
}

int Search::scoreMove(const Board& board, int move, int pvMove) {
    if (move == pvMove) return 2000000;

    int captured = Move::captured(move);
    if (captured != Board::EMPTY) {
        int attacker = board.board[Move::from(move)];
        return 1000000 + MVV_LVA[captured][attacker];
    }

    // Killer moves
    if (board.searchKillers[0][board.ply] == move) return 900000;
    if (board.searchKillers[1][board.ply] == move) return 800000;

    // History heuristic
    return board.searchHistory[board.board[Move::from(move)]][Move::to(move)];
}

void Search::sortMoves(MoveList& moves, const Board& board, int pvMove, int ply) {
    struct MoveScore {
        int move;
        int score;
    };

    std::vector<MoveScore> scoredMoves;
    for (int i = 0; i < moves.size(); i++) {
        scoredMoves.push_back({ moves.get(i), scoreMove(board, moves.get(i), pvMove) });
    }

    // Simple selection sort for the MoveList
    for (int i = 0; i < moves.size(); i++) {
        for (int j = i + 1; j < moves.size(); j++) {
            if (scoredMoves[j].score > scoredMoves[i].score) {
                std::swap(scoredMoves[i], scoredMoves[j]);
            }
        }
        moves.set(i, scoredMoves[i].move);
    }
}


int Search::quiescence(Board& board, int alpha, int beta) {
    // 1. Periodic Time Check
    if ((params.nodes & 2047) == 0) checkTime();
    if (params.stopped) return 0;

    params.nodes++;

    if (board.state.halfMoves >= 100 || board.isRepetition()){	
		return 0;
	}

    // Safety check for search depth to prevent stack overflow in extreme tactical scenarios
    if (board.ply >= Board::MAX_DEPTH - 1) {
        return Evaluation::evaluate(board);
    }

    // 2. Stand-Pat Score
    // We get a "static" evaluation of the position. If it's already good enough 
    // to cause a beta cutoff, we stop immediately without even looking at captures.
    int standPat = Evaluation::evaluate(board);
    if (standPat >= beta) {
        return beta;
    }
    if (standPat > alpha) {
        alpha = standPat;
    }

    // 3. Generate and Sort Captures
    int side = board.state.currentPlayer;
    MoveList moves;
    MoveGen::pseudoLegalCaptureMoves(&board, side, moves);

    // Sort captures using MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
    // This is passed NO_MOVE for the PV move because we are in Quiescence.
    sortMoves(moves, board, Move::NO_MOVE, board.ply);

    // 4. Capture Search Loop
    for (int i = 0; i < moves.size(); i++) {
        int move = moves.get(i);

        // 5. Post-Move Legality Check
        // Make the move and see if the board state considers it legal
        BoardState undo = board.makeMove(move);
        if (!undo.valid) {
            continue;
        }

        board.ply++;
        // Recursively call Quiescence Search
        int score = -quiescence(board, -beta, -alpha);
        board.ply--;
        board.undoMove(move, undo);

        // Exit if time has run out
        if (params.stopped) return 0;

        // 6. Alpha-Beta Pruning
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}