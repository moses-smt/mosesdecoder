#pragma once

#include <vector>
#include <string>
#include "moses/src/Hypothesis.h"

std::vector<std::string> SegmentSentenceAndWord(const std::string &sentence);
std::string StringToLower(std::string strToConvert);
void HypoToString(const Moses::Hypothesis &hypo, char *output);

