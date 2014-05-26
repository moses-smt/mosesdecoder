#pragma once

#include <iostream>
#include <vector>
#include <string>

typedef std::vector<std::string> Word;
typedef std::vector<Word> Phrase;

void LabelDeEn(const Phrase &source, std::ostream &out);
