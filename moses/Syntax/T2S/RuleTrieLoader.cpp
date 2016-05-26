#include "RuleTrieLoader.h"

#include <sys/stat.h>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <iostream>

#include "moses/FactorCollection.h"
#include "moses/Word.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/Range.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/FactorCollection.h"
#include "moses/Syntax/RuleTableFF.h"
#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/double-conversion/double-conversion.h"
#include "util/exception.hh"

#include "RuleTrie.h"
#include "moses/parameters/AllOptions.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

bool RuleTrieLoader::Load(Moses::AllOptions const& opts,
                          const std::vector<FactorType> &input,
                          const std::vector<FactorType> &output,
                          const std::string &inFile,
                          const RuleTableFF &ff,
                          RuleTrie &trie)
{
  PrintUserTime(std::string("Start loading text phrase table. Moses format"));

  std::size_t count = 0;

  std::ostream *progress = NULL;
  IFVERBOSE(1) progress = &std::cerr;
  util::FilePiece in(inFile.c_str(), progress);

  // reused variables
  std::vector<float> scoreVector;
  StringPiece line;

  int noflags = double_conversion::StringToDoubleConverter::NO_FLAGS;
  double_conversion::StringToDoubleConverter
  converter(noflags, NAN, NAN, "inf", "nan");

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

    StringPiece alignString;
    if (++pipes) {
      StringPiece temp(*pipes);
      alignString = temp;
    }

    ++pipes;  // counts

    bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == std::string::npos);
    if (isLHSEmpty && !opts.unk.word_deletion_enabled) { // staticData.IsWordDeletionEnabled()) {
      TRACE_ERR( ff.GetFilePath() << ":" << count << ": pt entry contains empty target, skipping\n");
      continue;
    }

    scoreVector.clear();
    for (util::TokenIter<util::AnyCharacter, true> s(scoreString, " \t"); s; ++s) {
      int processed;
      float score = converter.StringToFloat(s->data(), s->length(), &processed);
      UTIL_THROW_IF2(std::isnan(score), "Bad score " << *s << " on line " << count);
      scoreVector.push_back(FloorScore(TransformScore(score)));
    }
    const std::size_t numScoreComponents = ff.GetNumScoreComponents();
    if (scoreVector.size() != numScoreComponents) {
      UTIL_THROW2("Size of scoreVector != number (" << scoreVector.size() << "!="
                  << numScoreComponents << ") of score components on line " << count);
    }

    // parse source & find pt node

    // constituent labels
    Word *sourceLHS = NULL;
    Word *targetLHS;

    // create target phrase obj
    TargetPhrase *targetPhrase = new TargetPhrase(&ff);
    // targetPhrase->CreateFromString(Output, output, targetPhraseString, factorDelimiter, &targetLHS);
    targetPhrase->CreateFromString(Output, output, targetPhraseString, &targetLHS);
    // source
    Phrase sourcePhrase;
    // sourcePhrase.CreateFromString(Input, input, sourcePhraseString, factorDelimiter, &sourceLHS);
    sourcePhrase.CreateFromString(Input, input, sourcePhraseString, &sourceLHS);

    // rest of target phrase
    targetPhrase->SetAlignmentInfo(alignString);
    targetPhrase->SetTargetLHS(targetLHS);

    //targetPhrase->SetDebugOutput(string("New Format pt ") + line);

    if (++pipes) {
      StringPiece sparseString(*pipes);
      targetPhrase->SetSparseScore(&ff, sparseString);
    }

    if (++pipes) {
      StringPiece propertiesString(*pipes);
      targetPhrase->SetProperties(propertiesString);
    }

    targetPhrase->GetScoreBreakdown().Assign(&ff, scoreVector);
    targetPhrase->EvaluateInIsolation(sourcePhrase, ff.GetFeaturesToApply());

    TargetPhraseCollection::shared_ptr phraseColl
    = GetOrCreateTargetPhraseCollection(trie, *sourceLHS, sourcePhrase);
    phraseColl->Add(targetPhrase);

    // not implemented correctly in memory pt. just delete it for now
    delete sourceLHS;

    count++;
  }

  // sort and prune each target phrase collection
  if (ff.GetTableLimit()) {
    SortAndPrune(trie, ff.GetTableLimit());
  }

  return true;
}

}  // namespace T2S
}  // namespace Syntax
}  // namespace Moses
