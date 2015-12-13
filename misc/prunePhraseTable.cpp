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
#include <map>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "moses/InputPath.h"
#include "moses/Parameter.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/Timer.h"
#include "moses/StaticData.h"

#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/double-conversion/double-conversion.h"
#include "util/exception.hh"


using namespace Moses;
using namespace std;

namespace po = boost::program_options;
typedef multimap<float,string> Lines;

static void usage(const po::options_description& desc, char const** argv)
{
  cerr << "Usage: " + string(argv[0]) +  " [options] input-file output-file" << endl;
  cerr << desc << endl;
}

//Find top n translations of source, and send them to output
static void outputTopN(Lines lines, size_t maxPhrases, ostream& out)
{
  size_t count = 0;
  for (Lines::const_reverse_iterator i = lines.rbegin(); i != lines.rend(); ++i) {
    out << i->second << endl;
    ++count;
    if (count >= maxPhrases) break;
  }
}
/*
static void outputTopN(const Phrase& sourcePhrase, const multimap<float,const TargetPhrase*>& targetPhrases,
                size_t maxPhrases, const PhraseDictionary* phraseTable,
                   const vector<FactorType> & input, const vector<FactorType> & output, ostream& out) {
    size_t count = 0;
    for (multimap<float,const TargetPhrase*>::const_reverse_iterator i
         = targetPhrases.rbegin(); i != targetPhrases.rend() && count < maxPhrases; ++i, ++count) {
      const TargetPhrase* targetPhrase = i->second;
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
}*/
int main(int argc, char const** argv)
{
  bool help;
  string input_file;
  string config_file;
  size_t maxPhrases = 100;


  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("input-file,i", po::value<string>(&input_file), "Input file")
  ("config-file,f", po::value<string>(&config_file), "Config file")
  ("max-phrases,n", po::value<size_t>(&maxPhrases), "Maximum target phrases per source phrase")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(argc,argv).
                              options(cmdline_options).run();
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

  boost::scoped_ptr<Parameter> params(new Parameter());
  char const** mosesargv = new char const*[mosesargs.size()];
  for (size_t i = 0; i < mosesargs.size(); ++i) {
    mosesargv[i] = mosesargs[i].c_str();
    // mosesargv[i] = new char[mosesargs[i].length() + 1];
    // strcpy(mosesargv[i], mosesargs[i].c_str());
  }

  if (!params->LoadParam(mosesargs.size(), mosesargv)) {
    params->Explain();
    exit(1);
  }

  ResetUserTime();
  if (!StaticData::LoadDataStatic(params.get(),argv[0])) {
    exit(1);
  }

  const StaticData &staticData = StaticData::Instance();

  //Find the phrase table to manage the target phrases
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


  //
  //Load and prune the phrase table. This is taken (with mods) from moses/TranslationModel/RuleTable/LoaderStandard.cpp
  //

  std::ostream *progress = NULL;
  IFVERBOSE(1) progress = &std::cerr;
  util::FilePiece in(input_file.c_str(), progress);

  // reused variables
  vector<float> scoreVector;
  StringPiece line;

  double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::NO_FLAGS, NAN, NAN, "inf", "nan");

  string previous;
  Lines lines;


  while(true) {
    try {
      line = in.ReadLine();
    } catch (const util::EndOfFileException &e) {
      break;
    }

    util::TokenIter<util::MultiCharacter> pipes(line, "|||");
    StringPiece sourcePhraseString(*pipes);
    StringPiece targetPhraseString(*++pipes);
    StringPiece scoreString(*++pipes);
    scoreVector.clear();
    for (util::TokenIter<util::AnyCharacter, true> s(scoreString, " \t"); s; ++s) {
      int processed;
      float score = converter.StringToFloat(s->data(), s->length(), &processed);
      UTIL_THROW_IF2(isnan(score), "Bad score " << *s);
      scoreVector.push_back(FloorScore(TransformScore(score)));
    }

    if (sourcePhraseString != previous) {
      outputTopN(lines, maxPhrases, cout);
      previous = sourcePhraseString.as_string();
      lines.clear();
    }

    ScoreComponentCollection scores;
    scores.Assign(phraseTable,scoreVector);
    float score = scores.InnerProduct(staticData.GetAllWeights());
    lines.insert(pair<float,string>(score,line.as_string()));

  }
  if (!lines.empty()) {
    outputTopN(lines, maxPhrases, cout);
  }





  return 0;
}
