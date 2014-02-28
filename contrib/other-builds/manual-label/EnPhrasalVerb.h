#pragma once

#include "Main.h"

// roll your own identification of phrasal verbs
void EnPhrasalVerb(const Phrase &source, int revision, std::ostream &out);

bool Exist(const Phrase &source, int start, int end, int factor, const std::string &str);
size_t Found(const Phrase &source, int pos, int factor, const std::string &str);
bool Found(const Word &word, int factor, const std::vector<std::string> &soughts);

