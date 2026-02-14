#include <iostream>
#include "Engine/Perft.h"
#include "Engine/Evaluation.h"
#include "Engine/Magic.h"
#include "Engine/BitBoardGen.h"
#include "Engine/Zobrist.h"
#include "Engine/UCI.h"


int main() {
	BitBoardGen::initAll();
	Zobrist::init_keys();
	Evaluation::initAll();
	Magic::magicArraysInit();
	UCI::loop();
	
	//Board board = Board::fromFEN("4k3/2p5/8/p3P3/2P4P/1P2PpP1/8/4K3 b - - 0 1");
	// AttackInfo info;
	// Evaluation::computeAttacks(board, info);
	// std::cout << "Pawn atacks" << std::endl;
	// BitBoardGen::printBB(info.pawns[0]);
	// std::cout << "Rook atacks" << std::endl;
	// BitBoardGen::printBB(info.rooks[0]);
	// std::cout << "Kn atacks" << std::endl;
	// BitBoardGen::printBB(info.knights[0]);
	// std::cout << "Bishop atacks" << std::endl;
	// BitBoardGen::printBB(info.bishops[0]);
	// std::cout << "Queen atacks" << std::endl;
	// BitBoardGen::printBB(info.queens[0]);
	// int mg, eg = 0;
	// Evaluation::evalPawns(board, mg, eg);
	return 0;
}
