#pragma once

#include <string>
#include <set>
#include <vector>

namespace Moses
{

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>&,
                      const std::vector<std::string>&); 

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>&,
                      const std::vector<std::string>&,
                      const std::vector<size_t>&); 
}