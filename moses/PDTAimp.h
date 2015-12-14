// -*- c++ -*-
// $Id$
// vim:tabstop=2

#ifndef moses_PDTAimp_h
#define moses_PDTAimp_h

#include "StaticData.h"  // needed for factor splitter
#include "moses/TranslationModel/PhraseDictionaryTree.h"
#include "UniqueObject.h"
#include "InputFileStream.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "Util.h"
#include "util/tokenize_piece.hh"
#include "util/exception.hh"
#include "moses/FF/InputFeature.h"
#include "util/exception.hh"

namespace Moses
{

inline double addLogScale(double x,double y)
{
  if(x>y) return addLogScale(y,x);
  else return x+log(1.0+exp(y-x));
}

inline double Exp(double x)
{
  return exp(x);
}

/** implementation of the binary phrase table for the phrase-based decoder. Used by PhraseDictionaryTreeAdaptor
 */
class PDTAimp
{
  // only these classes are allowed to instantiate this class
  friend class PhraseDictionaryTreeAdaptor;

protected:
  PDTAimp(PhraseDictionaryTreeAdaptor *p);

public:
  std::vector<FactorType> m_input,m_output;
  PhraseDictionaryTree *m_dict;
  const InputFeature *m_inputFeature;
  typedef std::vector<TargetPhraseCollectionWithSourcePhrase::shared_ptr> vTPC;
  mutable vTPC m_tgtColls;

  typedef std::map<Phrase,TargetPhraseCollectionWithSourcePhrase::shared_ptr> MapSrc2Tgt;
  mutable MapSrc2Tgt m_cache;
  PhraseDictionaryTreeAdaptor *m_obj;
  int useCache;

  std::vector<vTPC> m_rangeCache;
  unsigned m_numInputScores;

  UniqueObjectManager<Phrase> uniqSrcPhr;

  size_t totalE,distinctE;
  std::vector<size_t> path1Best,pathExplored;
  std::vector<double> pathCN;

  ~PDTAimp();

  void Factors2String(Word const& w,std::string& s) const {
    s=w.GetString(m_input,false);
  }

  void CleanUp();

  TargetPhraseCollectionWithSourcePhrase::shared_ptr
  GetTargetPhraseCollection(Phrase const &src) const;

  void Create(const std::vector<FactorType> &input
              , const std::vector<FactorType> &output
              , const std::string &filePath
              , const std::vector<float> &weight);


  typedef PhraseDictionaryTree::PrefixPtr PPtr;
  typedef unsigned short Position;
  typedef std::pair<Position,Position> Range;
  struct State {
    PPtr ptr;
    Range range;
    std::vector<float> scores;
    Phrase src;

    State() : range(0,0),scores(0),src(ARRAY_SIZE_INCR) {}
    State(Position b,Position e,const PPtr& v,const std::vector<float>& sv=std::vector<float>(0))
      : ptr(v),range(b,e),scores(sv),src(ARRAY_SIZE_INCR) {}
    State(Range const& r,const PPtr& v,const std::vector<float>& sv=std::vector<float>(0))
      : ptr(v),range(r),scores(sv),src(ARRAY_SIZE_INCR) {}

    Position begin() const {
      return range.first;
    }
    Position end() const {
      return range.second;
    }
    std::vector<float> GetScores() const {
      return scores;
    }

    friend std::ostream& operator<<(std::ostream& out,State const& s) {
      out<<" R=("<<s.begin()<<","<<s.end()<<"),";
      for(std::vector<float>::const_iterator scoreIterator = s.GetScores().begin(); scoreIterator<s.GetScores().end(); scoreIterator++) {
        out<<", "<<*scoreIterator;
      }
      out<<")";
      return out;
    }

  };

  void CreateTargetPhrase(TargetPhrase& targetPhrase,
                          StringTgtCand::Tokens const& factorStrings,
                          std::string const& factorDelimiter,
                          Scores const& transVector,
                          Scores const& inputVector,
                          const std::string *alignmentString,
                          Phrase const* srcPtr=0) const;

  TargetPhraseCollectionWithSourcePhrase::shared_ptr PruneTargetCandidates
  (const std::vector<TargetPhrase> & tCands,
   std::vector<std::pair<float,size_t> >& costs,
   const std::vector<Phrase> &sourcePhrases) const;


  // POD for target phrase scores
  struct TScores {
    float total;
    Scores transScore, inputScores;
    Phrase const* src;

    TScores() : total(0.0),src(0) {}
  };

  void CacheSource(ConfusionNet const& src);

  size_t GetNumInputScores() const {
    return m_numInputScores;
  }
};

}
#endif
