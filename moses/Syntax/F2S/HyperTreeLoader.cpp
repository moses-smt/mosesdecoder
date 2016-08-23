#include "HyperTreeLoader.h"

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
#include "moses/Timer.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/Range.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/FactorCollection.h"
#include "moses/Syntax/RuleTableFF.h"
#include "moses/parameters/AllOptions.h"
#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/double-conversion/double-conversion.h"
#include "util/exception.hh"

#include "HyperPath.h"
#include "HyperPathLoader.h"
#include "HyperTree.h"

namespace Moses
{

namespace Syntax
{
namespace F2S
{

bool HyperTreeLoader::Load(AllOptions const& opts,
                           const std::vector<FactorType> &input,
                           const std::vector<FactorType> &output,
                           const std::string &inFile,
                           const RuleTableFF &ff,
                           HyperTree &trie,
                           boost::unordered_set<std::size_t> &sourceTermSet)
{
  PrintUserTime(std::string("Start loading HyperTree"));

  sourceTermSet.clear();

  std::size_t count = 0;

  std::ostream *progress = NULL;
  IFVERBOSE(1) progress = &std::cerr;
  util::FilePiece in(inFile.c_str(), progress);

  // reused variables
  std::vector<float> scoreVector;
  StringPiece line;

  double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::NO_FLAGS, NAN, NAN, "inf", "nan");

  HyperPathLoader hyperPathLoader;

  Phrase dummySourcePhrase;
  {
    Word *lhs = NULL;
    dummySourcePhrase.CreateFromString(Input, input, "hello", &lhs);
    delete lhs;
  }

  while(true) {
    try {
      line = in.ReadLine();
    } catch (const util::EndOfFileException &e) {
      break;
    }

    util::TokenIter<util::MultiCharacter> pipes(line, "|||");
    StringPiece sourceString(*pipes);
    StringPiece targetString(*++pipes);
    StringPiece scoreString(*++pipes);

    StringPiece alignString;
    if (++pipes) {
      StringPiece temp(*pipes);
      alignString = temp;
    }

    ++pipes;  // counts

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

    // Source-side
    HyperPath sourceFragment;
    hyperPathLoader.Load(sourceString, sourceFragment);
    ExtractSourceTerminalSetFromHyperPath(sourceFragment, sourceTermSet);

    // Target-side
    TargetPhrase *targetPhrase = new TargetPhrase(&ff);
    Word *targetLHS = NULL;
    targetPhrase->CreateFromString(Output, output, targetString, &targetLHS);
    targetPhrase->SetTargetLHS(targetLHS);
    targetPhrase->SetAlignmentInfo(alignString);

    if (++pipes) {
      StringPiece sparseString(*pipes);
      targetPhrase->SetSparseScore(&ff, sparseString);
    }

    if (++pipes) {
      StringPiece propertiesString(*pipes);
      targetPhrase->SetProperties(propertiesString);
    }

    targetPhrase->GetScoreBreakdown().Assign(&ff, scoreVector);
    targetPhrase->EvaluateInIsolation(dummySourcePhrase,
                                      ff.GetFeaturesToApply());

    // Add rule to trie.
    TargetPhraseCollection::shared_ptr phraseColl
    = GetOrCreateTargetPhraseCollection(trie, sourceFragment);
    phraseColl->Add(targetPhrase);

    count++;
  }

  // sort and prune each target phrase collection
  if (ff.GetTableLimit()) {
    SortAndPrune(trie, ff.GetTableLimit());
  }

  return true;
}

void HyperTreeLoader::ExtractSourceTerminalSetFromHyperPath(
  const HyperPath &hp, boost::unordered_set<std::size_t> &sourceTerminalSet)
{
  for (std::vector<HyperPath::NodeSeq>::const_iterator p = hp.nodeSeqs.begin();
       p != hp.nodeSeqs.end(); ++p) {
    for (std::vector<std::size_t>::const_iterator q = p->begin();
         q != p->end(); ++q) {
      const std::size_t factorId = *q;
      if (factorId >= moses_MaxNumNonterminals &&
          factorId != HyperPath::kComma &&
          factorId != HyperPath::kEpsilon) {
        sourceTerminalSet.insert(factorId);
      }
    }
  }
}

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
