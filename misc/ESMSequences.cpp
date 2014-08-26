#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <set>
#include <cstdlib>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "moses/FF/ESM-Feature/UtilESM.h"

int main(int argc, char * argv[]) {
  std::string line;
  while(std::getline(std::cin, line)) {
    std::vector<std::string> parts;
    boost::split(parts, line, boost::is_any_of("\t"));
    
    boost::trim(parts[0]);
    boost::trim(parts[1]);
    
    std::vector<std::string> source;
    boost::split(source, parts[0], boost::is_any_of(" "), boost::token_compress_on);
    
    std::vector<std::string> target;    
    boost::split(target, parts[1], boost::is_any_of(" "), boost::token_compress_on);
    
    std::vector<std::string> edits;
    if(parts.size() == 2) {
        // Use diff-based operations
        edits = Moses::calculateEdits(source, target);
    }
    else if(parts.size() == 3) {
        // Use alignment-based operations
        std::vector<std::string> alignmentStr;
        boost::split(alignmentStr, parts[2], boost::is_any_of(" -"));
        std::vector<size_t> alignment;
        BOOST_FOREACH(std::string a, alignmentStr)
            alignment.push_back(boost::lexical_cast<size_t>(a));
        
        edits = Moses::calculateEdits(source, target, alignment);
    }
    
    BOOST_FOREACH(std::string edit, edits)
        std::cout << edit << " ";
    std::cout << std::endl;
    
  }
  return 0;
}