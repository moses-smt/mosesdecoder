// -*- c++ -*-
#include <vector>
#include <string>

#include "moses/FF/FFState.h"
#include "moses/Hypothesis.h"
#include "moses/WordsRange.h"
#include "moses/TranslationOption.h"

#include "LexicalReordering.h"
#include "LexicalReorderingState.h"
#include "ReorderingStack.h"

namespace Moses
{

  typedef LexicalReorderingConfiguration LexReoConf;

  bool
  IsMonotonicStep(WordsRange  const& prev, // words range of last source phrase
		  WordsRange  const& cur,  // words range of current source phrase
		  WordsBitmap const& cov)  // coverage bitmap
  {
    size_t e = prev.GetEndPos() + 1;
    size_t s = cur.GetStartPos();
    return (s == e || (s >= e && !cov.GetValue(e)));
  }
  
  bool
  IsSwap(WordsRange const& prev, WordsRange const& cur, WordsBitmap const& cov)
  {
    size_t s = prev.GetStartPos();
    size_t e = cur.GetEndPos();
    return (e+1 == s || (e < s && !cov.GetValue(s-1)));
  }

  size_t 
  LexicalReorderingConfiguration::
  GetNumberOfTypes() const
  {
    return ((m_modelType == LexReoConf::MSD)  ? 3 :
	    (m_modelType == LexReoConf::MSLR) ? 4 : 2);
  }
  
  size_t 
  LexicalReorderingConfiguration::
  GetNumScoreComponents() const
  {
    size_t score_per_dir = m_collapseScores ? 1 : GetNumberOfTypes();
    return ((m_direction == Bidirectional) 
	    ? 2 * score_per_dir + m_additionalScoreComponents
	    : score_per_dir + m_additionalScoreComponents);
  }
  
  void 
  LexicalReorderingConfiguration::
  ConfigureSparse(std::map<std::string,std::string> const& sparseArgs, 
		  const LexicalReordering* producer)
  {
    if (sparseArgs.size()) 
      m_sparse.reset(new SparseReordering(sparseArgs, producer));
  }

  void LexicalReorderingConfiguration::SetAdditionalScoreComponents(size_t number)
  {
    m_additionalScoreComponents = number;
  }

  LexicalReorderingConfiguration::
  LexicalReorderingConfiguration(const std::string &modelType)
    : m_modelString(modelType)
    , m_scoreProducer(NULL)
    , m_modelType(None)
    , m_phraseBased(true)
    , m_collapseScores(false)
    , m_direction(Backward)
    , m_additionalScoreComponents(0)
  {
    std::vector<std::string> config = Tokenize<std::string>(modelType, "-");

    for (size_t i=0; i<config.size(); ++i) 
      {
        if      (config[i] == "hier")   { m_phraseBased = false; } 
        else if (config[i] == "phrase") { m_phraseBased = true; } 
        else if (config[i] == "wbe")    { m_phraseBased = true; }
        // no word-based decoding available, fall-back to phrase-based
        // This is the old lexical reordering model combination of moses

        else if (config[i] == "msd")          { m_modelType = MSD; } 
        else if (config[i] == "mslr")         {	m_modelType = MSLR; } 
        else if (config[i] == "monotonicity") {	m_modelType = Monotonic; } 
        else if (config[i] == "leftright")    { m_modelType = LeftRight; } 

        else if (config[i] == "backward")  { m_direction = Backward; } 

	// note: unidirectional is deprecated, use backward instead
        else if (config[i] == "unidirectional") { m_direction = Backward; } 
        else if (config[i] == "forward")        { m_direction = Forward; } 
        else if (config[i] == "bidirectional")  { m_direction = Bidirectional; } 

        else if (config[i] == "f")  { m_condition = F; } 
        else if (config[i] == "fe") { m_condition = FE; } 

        else if (config[i] == "collapseff") { m_collapseScores = true; } 
        else if (config[i] == "allff") { m_collapseScores = false; } 
        else 
          {
            std::cerr 
              << "Illegal part in the lexical reordering configuration string: " 
              << config[i] << std::endl;
            exit(1);
          }
      }
    
    if (m_modelType == None) 
      {
        std::cerr 
          << "You need to specify the type of the reordering model "
          << "(msd, monotonicity,...)" << std::endl;
        exit(1);
      }
  }
  
  LexicalReorderingState *
  LexicalReorderingConfiguration::
  CreateLexicalReorderingState(const InputType &input) const
  {
    LexicalReorderingState *bwd = NULL, *fwd = NULL;
    size_t offset = 0;
    
    switch(m_direction) 
      {
      case Backward:
      case Bidirectional:
        bwd = (m_phraseBased 
               ? new PhraseBasedReorderingState(*this, Backward, offset);
               : new HierarchicalReorderingBackwardState(*this, offset));
        offset += m_collapseScores ? 1 : GetNumberOfTypes();
        if (m_direction == Backward) return bwd; // else fall through
      case Forward:
        fwd = (m_phraseBased 
               ? new PhraseBasedReorderingState(*this, Forward, offset)
               : new HierarchicalReorderingForwardState(*this, input.GetSize(), 
                                                        offset));
        offset += m_collapseScores ? 1 : GetNumberOfTypes();
        if (m_direction == Forward) return fwd;
      }
    return new BidirectionalReorderingState(*this, bwd, fwd, 0);
  }

  void 
  LexicalReorderingState::
  CopyScores(ScoreComponentCollection*  accum, 
             const TranslationOption &topt, 
             const InputType& input,  
             ReorderingType reoType) const
  {
    // don't call this on a bidirectional object
    UTIL_THROW_IF2(m_direction != Backward && m_direction != Forward,
		   "Unknown direction: " << m_direction);

    TranslationOption const* 
      relevantOpt = (m_direction == Backward) ? &topt :  m_prevOption;
    
    LexicalReordering* reotable = m_configuration.GetScoreProducer();
    Scores const* cachedScores  = relevantOpt->GetLexReorderingScores(reotable);

    size_t off_remote = m_offset + reoType;
    size_t off_local = m_configuration.CollapseScores() ? m_offset : off_remote;

    // look up applicable score from vectore of scores
    if(cachedScores) 
      {
        Scores scores(reotable->GetNumScoreComponents(),0);
        socres[off_local ] (*cachedScores)[off_remote];
        accum->PlusEquals(reotable, scores);
      }

    // else: use default scores (if specified)
    else if (reotable->GetHaveDefaultScores()) 
      {
        Scores scores(reotable->GetNumScoreComponents(),0);
        scores[off_local] = reotable->GetDefaultScore(off_remote);
        accum->PlusEquals(m_configuration.GetScoreProducer(), scores);
      }
    // note: if no default score, no cost
    
    const SparseReordering* sparse = m_configuration.GetSparseReordering();
    if (sparse) sparse->CopyScores(*relevantOpt, m_prevOption, input, reoType, 
                                   m_direction, accum);
  }
  

  int 
  LexicalReorderingState::
  ComparePrevScores(const TranslationOption *other) const
  {
    LexicalReordering* reotable = m_configuration.GetScoreProducer();
    const Scores* myPrevScores = m_prevOption->GetLexReorderingScores(reotable);
    const Scores* otherPrevScores = other->GetLexReorderingScores(reotable);

    if(myPrevScores == otherPrevScores)
      return 0;

    // The pointers are NULL if a phrase pair isn't found in the reordering table.
    if(otherPrevScores == NULL)
      return -1;
    if(myPrevScores == NULL)
      return 1;

    for(size_t i = m_offset; i < m_offset + m_configuration.GetNumberOfTypes(); i++)
      if((*myPrevScores)[i] < (*otherPrevScores)[i])
	return -1;
      else if((*myPrevScores)[i] > (*otherPrevScores)[i])
	return 1;

    return 0;
  }

  bool PhraseBasedReorderingState::m_useFirstBackwardScore = true;

  PhraseBasedReorderingState::PhraseBasedReorderingState(const PhraseBasedReorderingState *prev, const TranslationOption &topt)
    : LexicalReorderingState(prev, topt), m_prevRange(topt.GetSourceWordsRange()), m_first(false) {}


  PhraseBasedReorderingState::PhraseBasedReorderingState(const LexReoConf &config,
							 LexReoConf::Direction dir, size_t offset)
    : LexicalReorderingState(config, dir, offset), m_prevRange(NOT_FOUND,NOT_FOUND), m_first(true) {}


  int PhraseBasedReorderingState::Compare(const FFState& o) const
  {
    if (&o == this)
      return 0;

    const PhraseBasedReorderingState* other = static_cast<const PhraseBasedReorderingState*>(&o);
    if (m_prevRange == other->m_prevRange) {
      if (m_direction == LexReoConf::Forward) {
	return ComparePrevScores(other->m_prevOption);
      } else {
	return 0;
      }
    } else if (m_prevRange < other->m_prevRange) {
      return -1;
    }
    return 1;
  }

  LexicalReorderingState* PhraseBasedReorderingState::Expand(const TranslationOption& topt, const InputType& input,ScoreComponentCollection* scores) const
  {
    ReorderingType reoType;
    const WordsRange currWordsRange = topt.GetSourceWordsRange();
    const LexReoConf::ModelType modelType = m_configuration.GetModelType();

    if ((m_direction != LexReoConf::Forward && m_useFirstBackwardScore)  || !m_first) {
      if (modelType == LexReoConf::MSD) {
	reoType = GetOrientationTypeMSD(currWordsRange);
      } else if (modelType == LexReoConf::MSLR) {
	reoType = GetOrientationTypeMSLR(currWordsRange);
      } else if (modelType == LexReoConf::Monotonic) {
	reoType = GetOrientationTypeMonotonic(currWordsRange);
      } else {
	reoType = GetOrientationTypeLeftRight(currWordsRange);
      }
      CopyScores(scores, topt, input, reoType);
    }

    return new PhraseBasedReorderingState(this, topt);
  }

  LexicalReorderingState::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMSD(WordsRange currRange) const
  {
    if (m_first) {
      if (currRange.GetStartPos() == 0) {
	return M;
      } else {
	return D;
      }
    }
    if (m_prevRange.GetEndPos() == currRange.GetStartPos()-1) {
      return M;
    } else if (m_prevRange.GetStartPos() == currRange.GetEndPos()+1) {
      return S;
    }
    return D;
  }

  LexicalReorderingState::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMSLR(WordsRange currRange) const
  {
    if (m_first) {
      if (currRange.GetStartPos() == 0) {
	return M;
      } else {
	return DR;
      }
    }
    if (m_prevRange.GetEndPos() == currRange.GetStartPos()-1) {
      return M;
    } else if (m_prevRange.GetStartPos() == currRange.GetEndPos()+1) {
      return S;
    } else if (m_prevRange.GetEndPos() < currRange.GetStartPos()) {
      return DR;
    }
    return DL;
  }


  LexicalReorderingState::ReorderingType PhraseBasedReorderingState::GetOrientationTypeMonotonic(WordsRange currRange) const
  {
    if ((m_first && currRange.GetStartPos() == 0) ||
	(m_prevRange.GetEndPos() == currRange.GetStartPos()-1)) {
      return M;
    }
    return NM;
  }

  LexicalReorderingState::ReorderingType PhraseBasedReorderingState::GetOrientationTypeLeftRight(WordsRange currRange) const
  {
    if (m_first ||
	(m_prevRange.GetEndPos() <= currRange.GetStartPos())) {
      return R;
    }
    return L;
  }

  ///////////////////////////
  //BidirectionalReorderingState

  int BidirectionalReorderingState::Compare(const FFState& o) const
  {
    if (&o == this)
      return 0;

    const BidirectionalReorderingState &other = static_cast<const BidirectionalReorderingState &>(o);
    if(m_backward->Compare(*other.m_backward) < 0)
      return -1;
    else if(m_backward->Compare(*other.m_backward) > 0)
      return 1;
    else
      return m_forward->Compare(*other.m_forward);
  }

  LexicalReorderingState* BidirectionalReorderingState::Expand(const TranslationOption& topt, const InputType& input, ScoreComponentCollection* scores) const
  {
    LexicalReorderingState *newbwd = m_backward->Expand(topt,input, scores);
    LexicalReorderingState *newfwd = m_forward->Expand(topt, input, scores);
    return new BidirectionalReorderingState(m_configuration, newbwd, newfwd, m_offset);
  }

  ///////////////////////////
  //HierarchicalReorderingBackwardState

  HierarchicalReorderingBackwardState::HierarchicalReorderingBackwardState(const HierarchicalReorderingBackwardState *prev,
									   const TranslationOption &topt, ReorderingStack reoStack)
    : LexicalReorderingState(prev, topt),  m_reoStack(reoStack) {}

  HierarchicalReorderingBackwardState::HierarchicalReorderingBackwardState(const LexReoConf &config, size_t offset)
    : LexicalReorderingState(config, LexReoConf::Backward, offset) {}


  int HierarchicalReorderingBackwardState::Compare(const FFState& o) const
  {
    const HierarchicalReorderingBackwardState& other = static_cast<const HierarchicalReorderingBackwardState&>(o);
    return m_reoStack.Compare(other.m_reoStack);
  }

  LexicalReorderingState* HierarchicalReorderingBackwardState::Expand(const TranslationOption& topt, const InputType& input,ScoreComponentCollection*  scores) const
  {

    HierarchicalReorderingBackwardState* nextState = new HierarchicalReorderingBackwardState(this, topt, m_reoStack);
    ReorderingType reoType;
    const LexReoConf::ModelType modelType = m_configuration.GetModelType();

    int reoDistance = nextState->m_reoStack.ShiftReduce(topt.GetSourceWordsRange());

    if (modelType == LexReoConf::MSD) {
      reoType = GetOrientationTypeMSD(reoDistance);
    } else if (modelType == LexReoConf::MSLR) {
      reoType = GetOrientationTypeMSLR(reoDistance);
    } else if (modelType == LexReoConf::LeftRight) {
      reoType = GetOrientationTypeLeftRight(reoDistance);
    } else {
      reoType = GetOrientationTypeMonotonic(reoDistance);
    }

    CopyScores(scores, topt, input, reoType);
    return nextState;
  }

  LexicalReorderingState::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMSD(int reoDistance) const
  {
    if (reoDistance == 1) {
      return M;
    } else if (reoDistance == -1) {
      return S;
    }
    return D;
  }

  LexicalReorderingState::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMSLR(int reoDistance) const
  {
    if (reoDistance == 1) {
      return M;
    } else if (reoDistance == -1) {
      return S;
    } else if (reoDistance > 1) {
      return DR;
    }
    return DL;
  }

  LexicalReorderingState::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeMonotonic(int reoDistance) const
  {
    if (reoDistance == 1) {
      return M;
    }
    return NM;
  }

  LexicalReorderingState::ReorderingType HierarchicalReorderingBackwardState::GetOrientationTypeLeftRight(int reoDistance) const
  {
    if (reoDistance >= 1) {
      return R;
    }
    return L;
  }




  ///////////////////////////
  //HierarchicalReorderingForwardState

  HierarchicalReorderingForwardState::
  HierarchicalReorderingForwardState(const LexReoConf &config, size_t size, size_t offset)
    : LexicalReorderingState(config, LexReoConf::Forward, offset), m_first(true), m_prevRange(NOT_FOUND,NOT_FOUND), m_coverage(size) {}

  HierarchicalReorderingForwardState::HierarchicalReorderingForwardState(const HierarchicalReorderingForwardState *prev, const TranslationOption &topt)
    : LexicalReorderingState(prev, topt), m_first(false), m_prevRange(topt.GetSourceWordsRange()), m_coverage(prev->m_coverage)
  {
    const WordsRange currWordsRange = topt.GetSourceWordsRange();
    m_coverage.SetValue(currWordsRange.GetStartPos(), currWordsRange.GetEndPos(), true);
  }

  int HierarchicalReorderingForwardState::Compare(const FFState& o) const
  {
    if (&o == this)
      return 0;

    const HierarchicalReorderingForwardState* other = static_cast<const HierarchicalReorderingForwardState*>(&o);

    if (m_prevRange == other->m_prevRange) {
      return ComparePrevScores(other->m_prevOption);
    } else if (m_prevRange < other->m_prevRange) {
      return -1;
    }
    return 1;
  }

  // For compatibility with the phrase-based reordering model, scoring is one step delayed.
  // The forward model takes determines orientations heuristically as follows:
  //  mono:   if the next phrase comes after the conditioning phrase and
  //          - there is a gap to the right of the conditioning phrase, or
  //          - the next phrase immediately follows it
  //  swap:   if the next phrase goes before the conditioning phrase and
  //          - there is a gap to the left of the conditioning phrase, or
  //          - the next phrase immediately precedes it
  //  dright: if the next phrase follows the cond. phr. 
  //          and other stuff comes in between
  //  dleft:  if the next phrase precedes the conditioning phrase 
  //          and other stuff comes in between

  LexicalReorderingState* 
  HierarchicalReorderingForwardState::
  Expand(const TranslationOption& topt, const InputType& input, 
	 ScoreComponentCollection* scores) const
  {
    LexReoConf::ModelType const modelType = m_configuration.GetModelType();
    WordsRange const& currRange = topt.GetSourceWordsRange();

    // keep track of the current cov. ourselves so we don't need the hypothesis
    WordsBitmap cov = m_coverage;
    cov.SetValue(currRange.GetStartPos(), currRange.GetEndPos(), true);


    if (!m_first) 
      {
	ReorderingType reoType
	  = ((modelType == LexReoConf::MSD) 
	     ? GetOrientationTypeMSD(currWordsRange, coverage)
	     : (modelType == LexReoConf::MSLR) 
	     ? GetOrientationTypeMSLR(currWordsRange, coverage)
	     : (modelType == LexReoConf::Monotonic) 
	     ? GetOrientationTypeMonotonic(currWordsRange, coverage);
	     : GetOrientationTypeLeftRight(currWordsRange, coverage));
	CopyScores(scores, topt, input, reoType);
      }
    
    return new HierarchicalReorderingForwardState(this, topt);
  }

  LexicalReorderingState::ReorderingType 
  HierarchicalReorderingForwardState::
  GetOrientationTypeMSD(WordsRange currRange, WordsBitmap coverage) const
  {
    return (IsMonotonicStep(m_prevRange,currRange,coverage) ? M 
	    : IsSwap(m_prevRange, currRange, coverage)      ? S : D);
  }

  LexicalReorderingState::ReorderingType 
  HierarchicalReorderingForwardState::
  GetOrientationTypeMSLR(WordsRange currRange, WordsBitmap coverage) const
  {
    return (IsMonotonicStep(m_prevRange,currRange,coverage) ? M 
	    : IsSwap(m_prevRange, currRange, coverage) 	    ? S 
	    : (currRange.GetStartPos() > m_prevRange.GetEndPos()) ? DR : DL);
  }

  LexicalReorderingState::ReorderingType 
  HierarchicalReorderingForwardState::
  GetOrientationTypeMonotonic(WordsRange currRange, WordsBitmap coverage) const
  {
    return IsMonotonicStep(m_prevRange, currRange, coverage) ? M : NM;
  }

  LexicalReorderingState::ReorderingType 
  HierarchicalReorderingForwardState::
  GetOrientationTypeLeftRight(WordsRange currRange, WordsBitmap coverage) const
  {
    return currRange.GetStartPos() > m_prevRange.GetEndPos() ? R : L;
  }


}
