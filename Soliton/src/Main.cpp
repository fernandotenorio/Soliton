#include <iostream>
#include "Engine/Perft.h"
#include "Engine/Evaluation.h""
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
	return 0;
}
