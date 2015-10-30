#include "MorphTrie.h"
#include <fstream>
#include <ostream>
#include <string>
#include <vector>
#include "moses/Util.h"
#include "moses/Factor.h"
#include "moses/InputFileStream.h"

using namespace std;

inline void ParseLineByChar(string& line, char c, vector<string>& substrings) {
    size_t i = 0;
    size_t j = line.find(c);

    while (j != string::npos) {
        substrings.push_back(line.substr(i, j-i));
        i = ++j;
        j = line.find(c, j);

        if (j == string::npos)
            substrings.push_back(line.substr(i, line.length()));
    }
}
