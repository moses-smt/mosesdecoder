
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <string>
#include "Util.h"

using namespace std;
using namespace Moses;

void createXML();

int main(int argc, char **argv)
{
  assert(argc == 2);

  string inPath(argv[1]);

  ifstream inStrme(inPath.c_str());
  ofstream rule((inPath + ".extract").c_str());
  ofstream ruleInv((inPath + ".extract.inv").c_str());

  int setenceId;
  float score;
  string source;
  string *input = NULL;
  vector<string> targets, alignments, paths;
  vector<int> counts;

  string inLine;
  
  int step = 0;
  while (!inStrme.eof())
  {
    getline(inStrme, inLine);
    cout << inLine << endl;
    switch (step)
    {
    case 0:
      setenceId = Scan<int>(inLine);
      ++step;
      break;
    case 1:
      score = Scan<float>(inLine);
      ++step;
      break;
    case 2:
      source = inLine;
      ++step;
      break;
    case 3:
      if (input == NULL) {
        input = new string(inLine);
      }
      else {
        assert(inLine == *input);
      }
      ++step;
      break;
    case 4:
      targets.push_back(inLine);
      ++step;
      break;
    case 5:
      alignments.push_back(inLine);
      ++step;
      break;
    case 6:
      paths.push_back(inLine);
      ++step;
      break;
    case 7:
      counts.push_back(Scan<int>(inLine));
      ++step;
      break;

    }

    createXML();
  }

  delete input;
  ruleInv.close();
  rule.close();
  inStrme.close();

}

void createXML()
{

}
