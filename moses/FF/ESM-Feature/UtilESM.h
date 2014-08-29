#pragma once

#include <string>
#include <set>
#include <vector>
#include "moses/AlignmentInfo.h"

namespace Moses
{

void calculateEdits(
    std::vector<std::string>&,
    const std::vector<std::string>&,
    const std::vector<std::string>&); 

void calculateEdits(
    std::vector<std::string>&,
    const std::vector<std::string>&,
    const std::vector<std::string>&,
    const Moses::AlignmentInfo&); 
}