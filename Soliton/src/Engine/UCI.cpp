#include "Engine/UCI.h"
#include "Engine/Search.h"
#include "Engine/Evaluation.h"
#include "Engine/TestSuite.h"
#include "Engine/EvalFen.h"
#include <iostream>
#include <sstream>

void UCI::loop() {
    Evaluation::initAll();
    Board board = Board::fromStartPosition();
    HashTable* tt = new HashTable(256);
    board.setHashTable(tt);

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "uci") {
            std::cout << "id name Soliton" << std::endl;
            std::cout << "id Fernando Mir" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if (line == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if (line == "ucinewgame") {
            // 1. Clear the Transposition Table
            if (board.hashTable) {
                board.hashTable->reset();
            }

            // 2. Clear Search Heuristics
            board.resetSearchHeuristics();

            // 3. Reset the board to startpos (often expected, though parsePosition usually follows)
            board = Board::fromStartPosition();
            board.setHashTable(tt); // Ensure the new board object has the TT pointer
        }
        else if (line.find("position") == 0) {
            parsePosition(line, board, tt);
        }
        else if (line.find("go") == 0) {
            parseGo(line, board);
        }
        else if (line.find("evaltest") == 0) {
            Evaluation::testEval("positions.fen");
        }
        else if (line.find("eval") == 0) {
            std::stringstream ss(line);
            std::string cmd, inputPath, outputPath;
            int depth;
            ss >> cmd; // This consumes the word "eval" from the stream

            std::string fl;

            // Now expects: eval <input> <output> <depth>
            if (ss >> inputPath >> outputPath >> depth) {         
                EvalFEN::eval(inputPath, outputPath, depth);
            }
            else {
                std::cout << "Error: Invalid format. Usage: eval <filename> <depth>" << std::endl;
            }
        }
        else if (line.find("bench") == 0) {
            TestSuite::runFile("bench.epd", 50);
        }
        else if (line == "quit") {
            break;
        }
    }
}

void UCI::parsePosition(std::string line, Board& board, HashTable* tt) {
    // Remove "position " from the beginning
    std::string input = line.substr(9);

    // Find if a "moves" section exists
    size_t movesPos = input.find("moves ");

    std::string posStr;
    std::string movesStr = "";

    if (movesPos != std::string::npos) {
        // If moves exist, split the string into position and moves
        posStr = input.substr(0, movesPos);
        movesStr = input.substr(movesPos + 6);
    }
    else {
        // Otherwise, the whole string is the position
        posStr = input;
    }

    // Trim any trailing whitespace from the position string
    posStr.erase(posStr.find_last_not_of(" \n\r\t") + 1);

    // Now, parse the isolated position string
    if (posStr == "startpos") {
        board = Board::fromStartPosition();
    }
    else if (posStr.find("fen") == 0) {
        // The FEN is everything after "fen "
        std::string fen = posStr.substr(4);
        board = Board::fromFEN(fen);
    }

    // Re-link the hash table to the new board object
    board.setHashTable(tt);

    // Apply moves if they were found
    if (!movesStr.empty()) {
        board.applyMoves(movesStr);
    }
}

void UCI::parseGo(std::string line, Board& board) {
    int depth = Board::MAX_DEPTH; // Default to maximum depth
    long long movetime = -1;      // Default to no time limit

    std::stringstream ss(line);
    std::string token;

    while (ss >> token) {
        if (token == "depth") {
            ss >> depth;
        }
        else if (token == "movetime") {
            ss >> movetime;
        }
        // You can also handle "wtime" or "btime" here similarly
    }

    // Start search with parameters
    Search::iterativeDeepening(board, depth, movetime, true);
}