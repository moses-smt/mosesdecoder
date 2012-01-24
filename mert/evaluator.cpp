#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <getopt.h>
#include <math.h>

#include "Scorer.h"
#include "Timer.h"
#include "Util.h"

#include "ScorerFactory.h"

using namespace std;

Scorer* scorer;
int bootstrap = 0;
void evaluate(const string& candFile);
void addStats(vector<int>& stats1, const vector<int>& stats2);
float average(const vector<float>& list);
float stdDeviation(const vector<float>& list, float avg);
string int2string(int n);

bool moreFiles = false;
bool moreScorers = false;

void usage()
{
  cerr<<"usage: evaluator [options] --reference ref1[,ref2[,ref3...]] --candidate cand1[,cand2[,cand3...]] "<<endl;
  cerr<<"[--sctype|-s] the scorer type (default BLEU)"<<endl;
  cerr<<"[--scconfig|-c] configuration string passed to scorer"<<endl;
  cerr<<"\tThis is of the form NAME1:VAL1,NAME2:VAL2 etc "<<endl;
  cerr<<"[--reference|-R] comma separated list of reference files"<<endl;
  cerr<<"[--candidate|-C] comma separated list of candidate files"<<endl;
  cerr<<"[--bootstrap|-b] number of booststraped samples (default 0 - no bootstraping)"<<endl;
  cerr<<"[--rseed|-r] the random seed for bootstraping (defaults to system clock)"<<endl;
  cerr<<"[--help|-h] print this message and exit"<<endl;
  cerr<<endl;
  cerr<<"Evaluator is able to compute more metrics at once. To do this,"<<endl;
  cerr<<"separate scorers with semicolon (note that comma is used to separate"<<endl;
  cerr<<"scorers in the interpolated scorer)."<<endl;
  cerr<<endl;
  cerr<<"If you specify only one metric and one candidate file, only the final score"<<endl;
  cerr<<"will be printed to stdout. Otherwise each line will contain metric name"<<endl;
  cerr<<"and/or filename and the final score. Since most of the metrics prints some"<<endl;
  cerr<<"debuging info, consider redirecting stderr to /dev/null."<<endl;
  exit(1);
}

static struct option long_options[] = {
  {"sctype",required_argument,0,'s'},
  {"scconfig",required_argument,0,'c'},
  {"reference",required_argument,0,'R'},
  {"candidate",required_argument,0,'C'},
  {"bootstrap",required_argument,0,'b'},
  {"rseed",required_argument,0,'r'},
  {"help",no_argument,0,'h'},
  {0, 0, 0, 0}
};
int option_index;

int main(int argc, char** argv)
{
  ResetUserTime();

  //defaults
  string scorerType("BLEU");
  string scorerConfig("");
  string reference("");
  string candidate("");

  int seed = 0;
  bool hasSeed = false;

  int c;
  while ((c=getopt_long (argc,argv, "s:c:R:C:b:r:h", long_options, &option_index)) != -1) {
    switch(c) {
      case 's':
        scorerType = string(optarg);
        break;
      case 'c':
        scorerConfig = string(optarg);
        break;
      case 'R':
        reference = string(optarg);
        break;
      case 'C':
        candidate = string(optarg);
        break;
      case 'b':
        bootstrap = atoi(optarg);
        break;
      case 'r':
        seed = strtol(optarg, NULL, 10);
        hasSeed = true;
        break;
      default:
        usage();
    }
  }

  if (bootstrap)
  {
    if (hasSeed) {
      cerr << "Seeding random numbers with " << seed << endl;
      srandom(seed);
    } else {
      cerr << "Seeding random numbers with system clock " << endl;
      srandom(time(NULL));
    }
  }

  try {
    vector<string> refFiles;
    vector<string> candFiles;
    vector<string> scorerTypes;

    

    if (reference.length() == 0) throw runtime_error("You have to specify at least one reference file.");
    split(reference,',',refFiles);

    if (candidate.length() == 0) throw runtime_error("You have to specify at least one candidate file.");
    split(candidate,',',candFiles);

    if (scorerType.length() == 0) throw runtime_error("You have to specify at least one scorer.");
    split(scorerType,';',scorerTypes);
    
    if (candFiles.size() > 1) moreFiles = true;
    if (scorerTypes.size() > 1) moreScorers = true;
        
    for (vector<string>::const_iterator fileIt = candFiles.begin(); fileIt != candFiles.end(); ++fileIt)
    {
        for (vector<string>::const_iterator scorerIt = scorerTypes.begin(); scorerIt != scorerTypes.end(); ++scorerIt)
        {
            scorer = ScorerFactory::getScorer(*scorerIt,scorerConfig);
            scorer->setReferenceFiles(refFiles);
            evaluate(*fileIt);
            delete scorer;
        }
    }

    return EXIT_SUCCESS;
  } catch (const exception& e) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

}

void evaluate(const string& candFile)
{
  ifstream cand(candFile.c_str());
  if (!cand.good()) throw runtime_error("Error opening candidate file");

  vector<ScoreStats> entries;

  // Loading sentences and preparing statistics
  ScoreStats scoreentry;
  string line;
  while (getline(cand, line))
  {
    scorer->prepareStats(entries.size(), line, scoreentry);
    entries.push_back(scoreentry);
  }

  int n = entries.size();
  if (bootstrap)
  {
    vector<float> scores;
    for (int i = 0; i < bootstrap; ++i)
    {
      // TODO: Use smart pointer for exceptional-safety.
      ScoreData* scoredata = new ScoreData(*scorer);
      for (int j = 0; j < n; ++j)
      {
        int randomIndex = random() % n;
        string str_j = int2string(j);
        scoredata->add(entries[randomIndex], str_j);
      }
      scorer->setScoreData(scoredata);
      candidates_t candidates(n, 0);
      float score = scorer->score(candidates);
      scores.push_back(score);
      delete scoredata;
    }

    float avg = average(scores);
    float dev = stdDeviation(scores, avg);

    if (moreFiles) cout << candFile << "\t";
    if (moreScorers) cout << scorer->getName() << "\t";

    cout.setf(ios::fixed,ios::floatfield);
    cout.precision(4);
    cout << avg << " [+-" << dev << "]"<< endl;
  }
  else
  {
    // TODO: Use smart pointer for exceptional-safety.
    ScoreData* scoredata = new ScoreData(*scorer);
    for (int sid = 0; sid < n; ++sid)
    {
      string str_sid = int2string(sid);
      scoredata->add(entries[sid], str_sid);
    }
    scorer->setScoreData(scoredata);
    candidates_t candidates(n, 0);
    float score = scorer->score(candidates);
    delete scoredata;
    
    if (moreFiles) cout << candFile << "\t";
    if (moreScorers) cout << scorer->getName() << "\t";
    
    cout.setf(ios::fixed,ios::floatfield);
    cout.precision(4);
    cout << score << endl;
  }
}

string int2string(int n)
{
  stringstream ss;
  ss << n;
  return ss.str();
}

float average(const vector<float>& list)
{
  float sum = 0;
  for (vector<float>::const_iterator it = list.begin(); it != list.end(); ++it)
    sum += *it;

  return sum / list.size();
}

float stdDeviation(const vector<float>& list, float avg)
{
  vector<float> tmp;
  for (vector<float>::const_iterator it = list.begin(); it != list.end(); ++it)
    tmp.push_back(pow(*it - avg, 2));

  return sqrt(average(tmp));
}
