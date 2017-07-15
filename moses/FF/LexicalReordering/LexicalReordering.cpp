#include <sstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>

#include "moses/FF/FFState.h"
#include "moses/TranslationOptionList.h"
#include "LexicalReordering.h"
#include "LRState.h"
#include "moses/StaticData.h"
#include "moses/Util.h"
#include "moses/InputPath.h"

using namespace std;
using namespace boost::algorithm;

namespace Moses
{
LexicalReordering::
LexicalReordering(const std::string &line)
  : StatefulFeatureFunction(line,false)
{
  VERBOSE(1, "Initializing Lexical Reordering Feature.." << std::endl);

  map<string,string> sparseArgs;
  m_haveDefaultScores = false;
  for (size_t i = 0; i < m_args.size(); ++i) {
    const vector<string> &args = m_args[i];

    if (args[0] == "type") {
      m_configuration.reset(new LRModel(args[1]));
      m_configuration->SetScoreProducer(this);
      m_modelTypeString = m_configuration->GetModelString();
    } else if (args[0] == "input-factor")
      m_factorsF =Tokenize<FactorType>(args[1]);
    else if (args[0] == "output-factor")
      m_factorsE =Tokenize<FactorType>(args[1]);
    else if (args[0] == "path")
      m_filePath = args[1];
    else if (starts_with(args[0], "sparse-"))
      sparseArgs[args[0].substr(7)] = args[1];
    else if (args[0] == "default-scores") {
      vector<string> tokens = Tokenize(args[1],",");
      for(size_t i=0; i<tokens.size(); i++)
        m_defaultScores.push_back( TransformScore( Scan<float>(tokens[i])));
      m_haveDefaultScores = true;
    } else UTIL_THROW2("Unknown argument " + args[0]);
  }

  switch(m_configuration->GetCondition()) {
  case LRModel::FE:
  case LRModel::E:
    UTIL_THROW_IF2(m_factorsE.empty(),
                   "TL factor mask for lexical reordering is "
                   << "unexpectedly empty");

    if(m_configuration->GetCondition() == LRModel::E)
      break; // else fall through
  case LRModel::F:
    UTIL_THROW_IF2(m_factorsF.empty(),
                   "SL factor mask for lexical reordering is "
                   << "unexpectedly empty");
    break;
  default:
    UTIL_THROW2("Unknown conditioning option!");
  }

  // sanity check: number of default scores
  size_t numScores
  = m_numScoreComponents
    = m_numTuneableComponents
      = m_configuration->GetNumScoreComponents();
  UTIL_THROW_IF2(m_haveDefaultScores && m_defaultScores.size() != numScores,
                 "wrong number of default scores (" << m_defaultScores.size()
                 << ") for lexicalized reordering model (expected "
                 << m_configuration->GetNumScoreComponents() << ")");

  m_configuration->ConfigureSparse(sparseArgs, this);
  // this->Register();
}

LexicalReordering::
~LexicalReordering()
{ }

void
LexicalReordering::
Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  typedef LexicalReorderingTable LRTable;
  if (m_filePath.size())
    m_table.reset(LRTable::LoadAvailable(m_filePath, m_factorsF,
                                         m_factorsE, std::vector<FactorType>()));
}

Scores
LexicalReordering::
GetProb(const Phrase& f, const Phrase& e) const
{
  return m_table->GetScore(f, e, Phrase(ARRAY_SIZE_INCR));
}

FFState*
LexicalReordering::
EvaluateWhenApplied(const Hypothesis& hypo,
                    const FFState* prev_state,
                    ScoreComponentCollection* out) const
{
  VERBOSE(3,"LexicalReordering::Evaluate(const Hypothesis& hypo,...) START" << std::endl);
  const LRState *prev = static_cast<const LRState *>(prev_state);
  LRState *next_state = prev->Expand(hypo.GetTranslationOption(), hypo.GetInput(), out);

  VERBOSE(3,"LexicalReordering::Evaluate(const Hypothesis& hypo,...) END" << std::endl);

  return next_state;
}

FFState const*
LexicalReordering::EmptyHypothesisState(const InputType &input) const
{
  return m_configuration->CreateLRState(input);
}

bool
LexicalReordering::
IsUseable(const FactorMask &mask) const
{
  BOOST_FOREACH(FactorType const& f, m_factorsE) {
    if (!mask[f]) return false;
  }
  return true;
}


void
LexicalReordering::
SetCache(TranslationOption& to) const
{
  if (to.GetLexReorderingScores(this)) return;
  // Scores were were set already (e.g., by sampling phrase table)

  if (m_table) {
    Phrase const& sphrase = to.GetInputPath().GetPhrase();
    Phrase const& tphrase = to.GetTargetPhrase();
    to.CacheLexReorderingScores(*this, this->GetProb(sphrase,tphrase));
  } else { // e.g. OOV with Mmsapt
    // Scores vals(GetNumScoreComponents(), 0);
    // to.CacheLexReorderingScores(*this, vals);
  }
}

LRModel const&
LexicalReordering
::GetModel() const
{
  return *m_configuration;
}


void
LexicalReordering::
SetCache(TranslationOptionList& tol) const
{
  BOOST_FOREACH(TranslationOption* to, tol)
  this->SetCache(*to);
}


}

