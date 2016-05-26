// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.

/**
 * @author Ulrich Germann
 * @file tokenindex.dump.cc
 * @brief Dumps a TokenIndex (vocab file for TPPT and TPLM) to stdout.
 */

#include "../mm/tpt_tokenindex.h"
#include <iostream>
#include <iomanip>

using namespace std;
using namespace sapt;
int
main(int argc,char* argv[])
{
  if (argc > 1 && !strcmp(argv[1], "-h")) {
     printf("Usage: %s <file>\n\n", argv[0]);
     cout << "Converts a phrase table in text format to a phrase table in tighly packed format." << endl;
     cout << "input file: token index file" << endl;
     exit(1);
  }

  TokenIndex I;
  I.open(argv[1]);
  vector<char const*> foo = I.reverseIndex();
  for (size_t i = 0; i < foo.size(); i++)
    cout << setw(10) << i << " " << foo[i] << endl;
}
