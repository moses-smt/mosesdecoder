#include "util/file_piece.hh"

#include "util/file.hh"
#include "util/scoped.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/murmur_hash.hh"
#include "util/probing_hash_table.hh"
#include "util/usage.hh"

#include "moses/TranslationModel/ProbingPT/quering.hh"

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h> //For finding size of file
#include <boost/functional/hash.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{
  if (argc != 2) {
    // Tell the user how to run the program
    std::cerr << "Usage: " << argv[0] << " path_to_directory" << std::endl;
    return 1;
  }

  Moses::QueryEngine queries(argv[1]);

  //Interactive search
  std::cout << "Please enter a string to be searched, or exit to exit." << std::endl;
  while (true) {
    std::string cinstr = "";
    getline(std::cin, cinstr);
    if (cinstr == "exit") {
      break;
    } else {
      //Actual lookup
      std::pair<bool, std::vector<target_text> > query_result;
      query_result = queries.query(StringPiece(cinstr));

      if (query_result.first) {
        queries.printTargetInfo(query_result.second);
      } else {
        std::cout << "Key not found!" << std::endl;
      }
    }
  }

  util::PrintUsage(std::cout);

  return 0;
}
