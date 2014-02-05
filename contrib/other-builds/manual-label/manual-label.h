#pragma once

#include <iostream>
#include <vector>
#include <string>

typedef std::vector<std::string> Word;
typedef std::vector<Word> Phrase;

bool IsA(const Phrase &source, int pos, int offset, int factor, const std::string &str);


