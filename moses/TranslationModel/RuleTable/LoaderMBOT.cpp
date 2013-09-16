//Fabienne Braune
//Loader for l-MBOT rules

#include "LoaderMBOT.h"
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>
#include "PhraseDictionarySCFG.h"
#include "PhraseDictionaryMBOT.h"
#include "moses/FactorCollection.h"
#include "moses/Word.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/WordsRange.h"
#include "moses/UserMessage.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/FactorCollection.h"
#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/double-conversion/double-conversion.h"

//new : include target phrase
#include "moses/TargetPhraseMBOT.h"
#include "moses/WordSequence.h"

using namespace std;

namespace Moses
{

bool RuleTableLoaderMBOT::Load(const std::vector<FactorType> &input,
            const std::vector<FactorType> &output,
            const std::string &inFile,
            const std::vector<float> &weight,
            size_t tableLimit,
            const LMList &languageModels,
            const WordPenaltyProducer* wpProducer,
            RuleTableTrie  &ruleTable)
{
  PrintUserTime("Start loading MBOT rules");

  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();

  string lineOrig;
  size_t count = 0;

  std::ostream *progress = NULL;
  IFVERBOSE(1) progress = &std::cerr;
  util::FilePiece in(inFile.c_str(), progress);

  // reused variables
  vector<float> scoreVector;
  StringPiece line;

  double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::NO_FLAGS, NAN, NAN, "inf", "nan");

  while(true) {
    try {
      line = in.ReadLine();
    } catch (const util::EndOfFileException &e) { break; }

    util::TokenIter<util::MultiCharacter> pipes(line, "|||");
       //Fabienne Braune : skip [MBOT] leading token. Could be put elsewhere
       pipes++;
       StringPiece sourcePhraseString(*pipes);
       StringPiece targetPhraseString(*++pipes);
       StringPiece scoreString(*++pipes);
       StringPiece alignString(*++pipes);

       // Allow but ignore rule count.
        if (++pipes && ++pipes) {
          stringstream strme;
          strme << "Syntax error at " << ruleTable.GetFilePath() << ":" << count;
          UserMessage::Add(strme.str());
          abort();
        }

        bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
        if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
          TRACE_ERR( ruleTable.GetFilePath() << ":" << count << ": pt entry contains empty target, skipping\n");
          continue;
        }

        scoreVector.clear();
        for (util::TokenIter<util::AnyCharacter, true> s(scoreString, " \t"); s; ++s) {
          int processed;
          float score = converter.StringToFloat(s->data(), s->length(), &processed);
          UTIL_THROW_IF(isnan(score), util::Exception, "Bad score " << *s << " on line " << count);
          scoreVector.push_back(FloorScore(TransformScore(score)));
        }
        const size_t numScoreComponents = ruleTable.GetFeature()->GetNumScoreComponents();
        if (scoreVector.size() != numScoreComponents) {
          stringstream strme;
          strme << "Size of scoreVector != number (" << scoreVector.size() << "!="
                << numScoreComponents << ") of score components on line " << count;
          UserMessage::Add(strme.str());
          abort();
        }

    // constituent labels
    Word sourceLHS;
    WordSequence targetLHS;

    // source
    Phrase sourcePhrase(0);
    sourcePhrase.CreateFromStringNewFormat(Input, input, sourcePhraseString, factorDelimiter, sourceLHS);

    //Fabienne Braune : source side of rule is stored in the target phrase (field of TargetPhraseMBOT)
    //For l-MBOT we need this to match the input tree during parsing
    TargetPhraseMBOT *targetPhrase = new TargetPhraseMBOT(sourcePhrase);
    targetPhrase->CreateFromStringForSequence(Output, output, targetPhraseString, factorDelimiter, targetLHS);

    //std::cerr << "Sequences in target : " << targetPhrase->GetMBOTPhrases() << std::endl;

    //set source lhs in target phrase
    targetPhrase->SetSourceLHS(sourceLHS);

    //SetAlignmentInfo deals with splitting the alignment at || and creating vectors
    targetPhrase->SetAlignmentInfo(alignString);

    //SetScoreChart computes special scores for discontiguous units
    targetPhrase->SetScoreChart(ruleTable.GetFeature(), scoreVector, weight, languageModels, wpProducer);

    TargetPhraseCollection &phraseColl = GetOrCreateTargetPhraseCollection(ruleTable, sourcePhrase, *targetPhrase, sourceLHS);

    phraseColl.Add(targetPhrase);

    count++;
  }

  // sort and prune each target phrase collection
  SortAndPrune(ruleTable);

  return true;
}

}
