#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include <functional>
#include <boost/filesystem.hpp>
#include "pruneGeneration.h"
#include "moses/InputFileStream.h"
#include "moses/OutputFileStream.h"

using namespace std;

int main(int argc, char **argv)
{
  cerr << "Starting" << endl;
  int limit = atoi(argv[1]);
  string inPathStem = argv[2];
  string outPathStem = argv[3];

  namespace fs = boost::filesystem;

  //cerr << "inPathStem=" << inPathStem << endl;
  fs::path p(inPathStem);
  fs::path dir = p.parent_path();
  //cerr << "dir=" << dir << endl;

  fs::path fileStem = p.filename();
  string fileStemStr = fileStem.native();
  size_t fileStemStrSize = fileStemStr.size();
  //cerr << "fileStem=" << fileStemStr << endl;

  // loop thru each file in directory
  fs::directory_iterator end_iter;
  for( fs::directory_iterator dir_iter(dir) ; dir_iter != end_iter ; ++dir_iter) {
    if (fs::is_regular_file(dir_iter->status())) {
      fs::path currPath = *dir_iter;
      string currPathStr = currPath.native();
      //cerr << "currPathStr=" << currPathStr << endl;

      fs::path currFile = currPath.filename();
      string currFileStr = currFile.native();

      if (currFileStr.find(fileStemStr) == 0) {
        // found gen table we need
        //cerr << "found=" << currPathStr << endl;
        string suffix = currFileStr.substr(fileStemStrSize, currFileStr.size() - fileStemStrSize);
        string outPath = outPathStem + suffix;
        cerr << "PRUNING " << currPathStr << " TO " << outPath << endl;

        Moses::InputFileStream inStrme(currPathStr);
        Moses::OutputFileStream outStrme(outPath);
        Process(limit, inStrme, outStrme);

      }
    }
  }

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
    outStrme << rec.line << endl;
  }
}

