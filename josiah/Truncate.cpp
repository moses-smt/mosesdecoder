/**
  * Truncate a phrase table to the top n translation options, given an 
  * input corpus.
**/
#include <iostream>
#include <iomanip>
#include <fstream>
#include <set>

#include <boost/program_options.hpp>

#include "Decoder.h"
#include "InputSource.h"
#include "DummyScoreProducers.h"
#include "ScoreProducer.h"
#include "StaticData.h"

using namespace std;
using namespace Josiah;
using namespace Moses;

namespace po = boost::program_options;


int main(int argc, char** argv) {
  size_t toptionLimit;
  string inputFile;
  string configFile;
  bool help;
  po::options_description visible("Allowed options");
  visible.add_options()
    ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
    ("toption-limit,t", po::value<size_t>(&toptionLimit)->default_value(20), "Number of translation options to prune to");

  po::options_description hidden("Hidden options");
  hidden.add_options()
    ("config-file", po::value<string>(&configFile), "config file")  
    ("input-file", po::value<string>(&inputFile), "input file");  

  po::positional_options_description p;
  p.add("config-file", 1);
  p.add("input-file", 1);

  po::options_description cmdline_options;
  cmdline_options.add(visible).add(hidden);

  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).positional(p).run(), vm);
  po::notify(vm);
  
  if (inputFile.empty() || configFile.empty()) help = true;
  
  
  if (help) {
    std::cout << "Usage: " + string(argv[0]) +  " [options] config-file input-file" << std::endl;
    std::cout << visible << std::endl;
    return 0;
  }
  cerr << "Truncating the model " << configFile << " using the input file " << inputFile << endl;


  //set up moses
  vector<string> extraArgs;
  extraArgs.push_back("-ttable-limit");
  ostringstream toptionLimitStr;
  toptionLimitStr << toptionLimit;
  extraArgs.push_back(toptionLimitStr.str());
  extraArgs.push_back("-persistent-cache-size");
  extraArgs.push_back("0");
  initMoses(configFile,0,extraArgs);


  //store source phrases already output
  set<Phrase> sourcePhrases;

  ifstream in(inputFile.c_str());
  if (!in) {
    cerr << "Unable to open input file " << inputFile << endl;
    return 1;
  }

  //only print the 1st factor
  vector<FactorType> factors;
  factors.push_back(0);

  //Assume single phrase feature
  StaticData& staticData =
    const_cast<StaticData&>(StaticData::Instance());
  PhraseDictionaryFeature* ptable  = StaticData::Instance().GetTranslationSystem
    (TranslationSystem::DEFAULT).GetPhraseDictionaries()[0];

  //To detect unknown words
  const ScoreProducer* uwp  = staticData. GetTranslationSystem
    (TranslationSystem::DEFAULT).GetUnknownWordPenaltyProducer();


  string line;
  while (getline(in,line)) {
    //cerr << line << endl;
    TranslationHypothesis translation(line);
    size_t length = translation.getWords().size();
    size_t maxPhraseSize = staticData.GetMaxPhraseLength();
    for (size_t start = 0; start < length; ++start) {
      for (size_t end = start; end < start + maxPhraseSize && end < length; ++end) {
        TranslationOptionList& options = translation.getToc()->GetTranslationOptionList(start,end);
        if (!options.size()) continue;
        const Phrase* sourcePhrase = options.Get(0)->GetSourcePhrase();
        if (sourcePhrases.find(*sourcePhrase) != sourcePhrases.end()) continue;
        if (options.Get(0)->GetScoreBreakdown().GetScoreForProducer(uwp)) continue;
        sourcePhrases.insert(*sourcePhrase);
        for (size_t i = 0; i < options.size(); ++i) {
          const TranslationOption* option = options.Get(i);
          cout << sourcePhrase->GetStringRep(factors);
          cout << " ||| ";
          cout << option->GetTargetPhrase().GetStringRep(factors);
          cout << " |||";
          vector<float>  scores = option->GetScoreBreakdown().GetScoresForProducer(ptable);
          for (size_t j = 0; j < scores.size(); ++j) {
            cout << " " << exp(scores[j]);
          }
          cout << " ||| |||";
          cout << endl;
        }
      }
    }
  }

}
