#include "moses/TranslationTask.h"

#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

using namespace Moses;
using namespace std;
using namespace boost;

int main(int argc, char const* argv[])
{
  // Load standard Moses config
  Parameter params;
  if (!params.LoadParam(argc, argv) || !StaticData::LoadDataStatic(&params, argv[0])) {
    exit(2);
  }

  StaticData const& global = StaticData::Instance();
  global.SetVerboseLevel(0);
  vector<FactorType> ifo = global.options()->input.factor_order;

  // Get last PhraseDictionary in config (either single model or model combination entry)
  PhraseDictionary* pt = PhraseDictionary::GetColl()[PhraseDictionary::GetColl().size() - 1];

  // Only lookup each phrase once
  unordered_set<string> seen;

  string context_weight_spec;
  params.SetParameter(context_weight_spec,"context-weights",string(""));
  boost::shared_ptr<ContextScope> scope(new ContextScope);
  boost::shared_ptr<IOWrapper> none;
  if (context_weight_spec.size())
    scope->SetContextWeights(context_weight_spec);

  string line;
  while (true) {
    // Input line
    if (getline(cin, line, '\n').eof()) {
      break;
    }
    vector<string> words = Tokenize(line);

    // For each start word
    for (size_t i = 0; i < words.size(); ++i) {
      // Try phrases of increasing length
      for (size_t j = i + 1; j <= words.size(); ++j) {

        string phrase_str = Join(" ", words.begin() + i, words.begin() + j);
        // Unique check
        if (seen.find(phrase_str) != seen.end()) {
          continue;
        }
        seen.insert(phrase_str);

        // New phrase
        boost::shared_ptr<Sentence> phrase(new Sentence(global.options()));
        phrase->init(phrase_str);
        Phrase const& src = *phrase;


        // Setup task for phrase
        boost::shared_ptr<TranslationTask> ttask;
        ttask = TranslationTask::create(phrase, none, scope);
	
        // Support model combinations (PhraseDictionaryGroup)
        BOOST_FOREACH(PhraseDictionary* p, PhraseDictionary::GetColl()) {
          p->InitializeForInput(ttask);
        }

        // Query PhraseDictionary
        TargetPhraseCollection::shared_ptr tgts = pt->GetTargetPhraseCollectionLEGACY(ttask, src);
        // No results breaks loop over increasing phrase lengths
        if (!tgts) {
          break;
        }

        for (size_t k = 0; k < tgts->GetSize(); ++k) {
          TargetPhrase const& tgt = static_cast<TargetPhrase const&>(*(*tgts)[k]);
          ScoreComponentCollection const& scc = tgt.GetScoreBreakdown();
          size_t start = pt->GetIndex();
          size_t stop  = start + pt->GetNumScoreComponents();
          FVector const& scores = scc.GetScoresVector();
          cout << src << "||| " << static_cast<Phrase const>(tgt) << "|||";
            for (size_t k = start; k < stop; ++k) {
              float f = scores[k];
              cout << " " << f;
            }
          cout << " ||| " << tgt.GetAlignTerm() << endl;
        }
      }
    }
  }
}
