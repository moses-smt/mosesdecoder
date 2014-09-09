// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2014- University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/


/**
  Prune the phrase table using the same translation pruning that Moses uses during decoding.
**/

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "moses/InputPath.h"
#include "moses/Parameter.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/StaticData.h"

#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/double-conversion/double-conversion.h"
#include "util/exception.hh"


using namespace Moses;
using namespace std;

namespace po = boost::program_options;

static void usage(const po::options_description& desc, char** argv) {
    cerr << "Usage: " + string(argv[0]) +  " [options] input-file output-file" << endl;
    cerr << desc << endl;
}

//Find top n translations of source, and send them to output
static void outputTopN(const StringPiece& sourcePhraseString, PhraseDictionary* phraseTable, const std::vector<FactorType> &input,  ostream& out) {
  //get list of target phrases
  Phrase sourcePhrase;
  sourcePhrase.CreateFromString(Input,input,sourcePhraseString,NULL);
  InputPath inputPath(sourcePhrase, NonTerminalSet(), WordsRange(0,sourcePhrase.GetSize()-1),NULL,NULL);
  InputPathList inputPaths;
  inputPaths.push_back(&inputPath);
  phraseTable->GetTargetPhraseCollectionBatch(inputPaths);
  const TargetPhraseCollection* targetPhrases = inputPath.GetTargetPhrases(*phraseTable);




  //print phrases
  const std::vector<FactorType>& output = StaticData::Instance().GetOutputFactorOrder();
  if (targetPhrases) {
    //if (targetPhrases->GetSize() > 10) cerr << "src " << sourcePhrase << " tgt count " << targetPhrases->GetSize() << endl;
    for (TargetPhraseCollection::const_iterator i = targetPhrases->begin(); i != targetPhrases->end(); ++i) {
      const TargetPhrase* targetPhrase = *i;
      out << sourcePhrase.GetStringRep(input);
      out << " ||| ";
      out << targetPhrase->GetStringRep(output);
      out << " ||| ";
      const ScoreComponentCollection scores = targetPhrase->GetScoreBreakdown();
      vector<float> phraseScores = scores.GetScoresForProducer(phraseTable);
      for (size_t j = 0; j < phraseScores.size(); ++j) {
        out << exp(phraseScores[j]) << " ";
      }
      out << "||| ";
      const AlignmentInfo& align = targetPhrase->GetAlignTerm();
      for (AlignmentInfo::const_iterator j = align.begin(); j != align.end(); ++j) {
        out << j->first << "-" << j->second << " ";
      }
      out << endl;
    }
  }

}

int main(int argc, char** argv) 
{
  bool help;
  string input_file;
  string config_file;


  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("input-file,i", po::value<string>(&input_file), "Input file")
  ("config-file,f", po::value<string>(&config_file), "Config file")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(argc,argv).
            options(cmdline_options).allow_unregistered().run();
  po::store(parsed, vm);
  po::notify(vm);
  if (help) {
    usage(desc, argv);
    exit(0);
  }
  if (input_file.empty()) {
    cerr << "ERROR: Please specify an input file" << endl << endl;
    usage(desc, argv);
    exit(1);
  }
  if (config_file.empty()) {
    cerr << "ERROR: Please specify a config file" << endl << endl;
    usage(desc, argv);
    exit(1);
  }

  vector<string> mosesargs;
  mosesargs.push_back(argv[0]);
  mosesargs.push_back("-f");
  mosesargs.push_back(config_file);
  for (size_t i = 0; i < parsed.options.size(); ++i) {
    if (parsed.options[i].position_key == -1 && !parsed.options[i].unregistered) continue;
    /*
    const string& key = parsed.options[i].string_key;
    if (!key.empty()) {
      mosesargs.push_back(key);
    }
    for (size_t j = 0; j < parsed.options[i].value.size(); ++j) {
      const string& value = parsed.options[i].value[j];
      if (!value.empty()) {
        mosesargs.push_back(value);
      }
    }*/

    for (size_t j = 0; j < parsed.options[i].original_tokens.size(); ++j) {
      mosesargs.push_back(parsed.options[i].original_tokens[j]);
    }
  }

  boost::scoped_ptr<Parameter> params(new Parameter());  
  char** mosesargv = new char*[mosesargs.size()];
  for (size_t i = 0; i < mosesargs.size(); ++i) {
    mosesargv[i] = new char[mosesargs[i].length() + 1];
    strcpy(mosesargv[i], mosesargs[i].c_str());
  }

  if (!params->LoadParam(mosesargs.size(), mosesargv)) {
    params->Explain();
    exit(1);
  }

  if (!StaticData::LoadDataStatic(params.get(),argv[0])) {
    exit(1);
  }

  const StaticData &staticData = StaticData::Instance();
  const std::vector<FactorType> & input = staticData.GetInputFactorOrder();

  //Find the phrase table to evaluate with
  PhraseDictionary* phraseTable = NULL;
  const vector<FeatureFunction*>& ffs = FeatureFunction::GetFeatureFunctions();
  for (size_t i = 0; i < ffs.size(); ++i) {
    PhraseDictionary* maybePhraseTable = dynamic_cast< PhraseDictionary*>(ffs[i]);
    if (maybePhraseTable) {
      UTIL_THROW_IF(phraseTable,util::Exception,"Can only score translations with one phrase table");
      phraseTable = maybePhraseTable;
    }
  }
  UTIL_THROW_IF(!phraseTable,util::Exception,"Unable to find scoring phrase table");

  Sentence sentence;
  phraseTable->InitializeForInput(sentence);

  //
  //Load and prune the phrase table. This is taken (with mods) from moses/TranslationModel/RuleTable/LoaderStandard.cpp
  //

  string lineOrig;

  std::ostream *progress = NULL;
  IFVERBOSE(1) progress = &std::cerr;
  util::FilePiece in(input_file.c_str(), progress);

  // reused variables
  vector<float> scoreVector;
  StringPiece line;

  double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::NO_FLAGS, NAN, NAN, "inf", "nan");

  StringPiece previous;

  while(true) {
    try {
      line = in.ReadLine();
    } catch (const util::EndOfFileException &e) {
      break;
    }

    util::TokenIter<util::MultiCharacter> pipes(line, "|||");
    StringPiece sourcePhraseString(*pipes);
    if (sourcePhraseString != previous) {
      outputTopN(previous, phraseTable, input, cout);
      previous = sourcePhraseString;
    }
  }
  outputTopN(previous, phraseTable, input, cout);





  return 0;
}
