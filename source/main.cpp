//#include "jsonAnalysis-boost.hpp"
#include "jsonAnalysis-std.hpp"

int main() {
    JsonAnalysis::JsonAnalysis json(std::fstream("../test.json", std::ios::in));
    json.PrintJsonElem();
    return 0;
}
