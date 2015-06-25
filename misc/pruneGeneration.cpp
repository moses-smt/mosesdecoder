#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include <functional>
#include "pruneGeneration.h"
#include "moses/InputFileStream.h"

using namespace std;

int main(int argc, char **argv)
{
  cerr << "Starting" << endl;
  int limit = atoi(argv[1]);

  Process(limit, cin, cout);
  
  cerr << "Finished" << endl;
}

void Process(int limit, istream &inStrme, ostream &outStrme)
{
  vector<Rec> records;
  string prevInWord;
  string line;
  while (getline(inStrme, line)) {
    vector<string> toks;
    Tokenize(toks, line);
    assert(toks.size() == 4);

    if (prevInWord != toks[0]) {
      Output(outStrme, records, limit);
      records.clear();
    }

    // add new record
    float prob = atof(toks[2].c_str());
    records.push_back(Rec(prob, line));

    prevInWord = toks[0];
  }

  // last
  Output(outStrme, records, limit);
  records.clear();

}

void Output(ostream &outStrme, vector<Rec> &records, int limit)
{
  std::sort(records.rbegin(), records.rend());

  for (size_t i = 0; i < limit && i < records.size(); ++i) {
    const Rec &rec = records[i];
    cout << rec.line << endl;
  }
}

