#include <iostream>
#include <string>

#include "QueryPhraseTableMin.h"

int main(int argc, char** argv) {
    std::string path = argv[1];
    std::cout << path << std::endl;
    
    QueryPhraseTableMin test(path);
    std::cout << test.query("man") << std::endl;
}