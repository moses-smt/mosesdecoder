#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <functional>
#include "pruneGeneration.h"

using namespace std;

int main(int argc, char **argv)
{
  cerr << "Starting" << endl;
  int limit = atoi(argv[1]);
  
  vector<Rec> records;
  string prevInWord;
  string line;
  while (getline(cin, line)) {
    vector<string> toks;
    Tokenize(toks, line);
    assert(toks.size() == 4);
    
    if (prevInWord != toks[0]) {
      Output(limit, records);
      records.clear();
    }
    
    // add new record
    float prob = atof(toks[2].c_str());
    records.push_back(Rec(prob, line));

    prevInWord = toks[0];
  }

  // last
  Output(limit, records);
  records.clear();

  cerr << "Finished" << endl;  
}

void Output(int limit, vector<Rec> &records)
{
  Prune(limit, records);
  
  for (size_t i = 0; i < limit && i < records.size(); ++i) {
    const Rec &rec = records[i];
    cout << rec.line << endl;
  }
}

void Prune(int limit, std::vector<Rec> &records)
{
  std::sort(records.rbegin(), records.rend());
  
}
