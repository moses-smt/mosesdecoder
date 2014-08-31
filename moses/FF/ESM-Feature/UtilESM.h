#pragma once

#include <string>
#include <set>
#include <vector>
#include "moses/AlignmentInfo.h"
#include "util/string_piece.hh"

namespace Moses
{

struct Stats {
  Stats() : deletes(0), inserts(0), matches(0), substitutions(0) {}
    
  size_t deletes;
  size_t inserts;
  size_t matches;
  size_t substitutions;
};

void calculateEdits(
    std::vector<std::string>&,
    Stats&,
    const std::vector<StringPiece>&,
    const std::vector<StringPiece>&,
    const Moses::AlignmentInfo&); 
}