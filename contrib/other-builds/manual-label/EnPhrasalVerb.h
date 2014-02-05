#pragma once

#include "manual-label.h"

void EnPhrasalVerb(const Phrase &source, std::ostream &out);

size_t Found(const Phrase &source, int pos, int factor, const std::string &str);
bool Found(const Word &word, int factor, const std::vector<std::string> &soughts);

