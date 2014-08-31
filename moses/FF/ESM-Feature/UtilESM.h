#pragma once

#include <string>
#include <set>
#include <vector>
#include "moses/AlignmentInfo.h"
#include "util/string_piece.hh"

namespace Moses
{

void calculateEdits(
    std::vector<std::string>&,
    const std::vector<StringPiece>&,
    const std::vector<StringPiece>&,
    const Moses::AlignmentInfo&); 
}