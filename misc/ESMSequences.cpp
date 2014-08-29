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

#include "moses/AlignmentInfoCollection.h"
#include "moses/FF/ESM-Feature/UtilESM.h"

int main(int argc, char * argv[]) {
  std::string line;
  while(std::getline(std::cin, line)) {
    std::vector<std::string> parts;
    boost::split(parts, line, boost::is_any_of("\t"), boost::token_compress_on);
    
    boost::trim(parts[0]);
    boost::trim(parts[1]);
    boost::trim(parts[2]);

    std::vector<std::string> source;
    boost::split(source, parts[0], boost::is_any_of(" "), boost::token_compress_on);
    
    std::vector<std::string> target;    
    boost::split(target, parts[1], boost::is_any_of(" "), boost::token_compress_on);
    
    std::vector<std::string> edits;
    if(parts.size() == 2) {
        // Use diff-based operations
        Moses::calculateEdits(edits, source, target);
    }
    else if(parts.size() == 3) {
        // Use alignment-based operations
        std::vector<std::string> alignmentStr;
        boost::split(alignmentStr, parts[2], boost::is_any_of(" -"), boost::token_compress_on);
        std::set<std::pair<size_t, size_t> > container;
        for(size_t i = 0; i < alignmentStr.size() - 1; i += 2) {
            size_t a = boost::lexical_cast<size_t>(alignmentStr[i]);
            size_t b = boost::lexical_cast<size_t>(alignmentStr[i + 1]);
            container.insert(std::make_pair(a, b));
        }
        const Moses::AlignmentInfo* alignmentPtr = Moses::AlignmentInfoCollection::Instance().Add(container);
        Moses::calculateEdits(edits, source, target, *alignmentPtr);
    }
    
    BOOST_FOREACH(std::string edit, edits)
        std::cout << edit << " ";
    std::cout << std::endl;
    
  }
  return 0;
}