#ifndef EVALFEN_H
#include <string>

class EvalFEN {
public:
	static void eval(std::string inputPath, std::string outputPath, int depth);
};
#endif EVALFEN_H
