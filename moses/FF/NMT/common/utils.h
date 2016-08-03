#pragma once
#include <string>
#include <boost/algorithm/string.hpp>

void Trim(std::string& s);

void Split(const std::string& line, std::vector<std::string>& pieces, const std::string del=" ");
