#ifndef moses_PrefixTreeMap_h
#define moses_PrefixTreeMap_h

#include<vector>
#include<climits>
#include<iostream>
#include <map>


#include "PrefixTree.h"
#include "File.h"
#include "LVoc.h"
#include "ObjectPool.h"

namespace Moses
{


typedef PrefixTreeF<LabelId,OFF_T> PTF;
typedef FilePtr<PTF>               CPT;
typedef std::vector<CPT>           Data;
typedef LVoc<std::string>          WordVoc;

/** @todo How is this used in the pb binary phrase table?
 */
class GenericCandidate
{
public:
  typedef std::vector<IPhrase>              PhraseList;
  typedef std::vector< std::vector<float> > ScoreList;
public:
  GenericCandidate() {
  };
  GenericCandidate(const GenericCandidate& other)
    : m_PhraseList(other.m_PhraseList), m_ScoreList(other.m_ScoreList) {
  };
  GenericCandidate(const PhraseList& p, const ScoreList& s)
    : m_PhraseList(p), m_ScoreList(s) {
  };
  ~GenericCandidate() {
  };
public:
  size_t NumPhrases() const {
    return m_PhraseList.size();
  };
  size_t NumScores()  const {
    return m_ScoreList.size();
  };
  const IPhrase& GetPhrase(unsigned int i) const {
    return m_PhraseList.at(i);
  }
  const std::vector<float>& GetScore(unsigned int i) const {
    return m_ScoreList.at(i);
  }
  void readBin(FILE* f);
  void writeBin(FILE* f) const;
private:
  PhraseList m_PhraseList;
  ScoreList  m_ScoreList;
};


/** @todo How is this used in the pb binary phrase table?
 */
struct PPimp {
  PTF const*p;
  unsigned idx;
  bool root;

  PPimp(PTF const* x,unsigned i,bool b) : p(x),idx(i),root(b) {}
  bool isValid() const {
    return root || (p && idx<p->size());
  }

  bool isRoot() const {
    return root;
  }
  PTF const* ptr() const {
    return p;
  }
};


/** @todo How is this used in the pb binary phrase table?
 */
class Candidates : public std::vector<GenericCandidate>
{
  typedef std::vector<GenericCandidate> MyBase;
public:
  Candidates() : MyBase() {
  };
  void writeBin(FILE* f) const;
  void readBin(FILE* f);
};

class PrefixTreeMap
{
public:
  PrefixTreeMap() : m_FileSrc(0), m_FileTgt(0) {
    PTF::setDefault(InvalidOffT);
  }
  ~PrefixTreeMap();

public:
  static const LabelId MagicWord;

  void FreeMemory();

  int Read(const std::string& fileNameStem, int numVocs = -1);

  void GetCandidates(const IPhrase& key, Candidates* cands);
  void GetCandidates(const PPimp& p, Candidates* cands);

  std::vector< std::string const * > ConvertPhrase(const IPhrase& p, unsigned int voc) const;
  IPhrase ConvertPhrase(const std::vector< std::string >& p, unsigned int voc) const;
  LabelId ConvertWord(const std::string& w, unsigned int voc) const;
  std::string ConvertWord(LabelId w, unsigned int voc) const;
public: //low level
  PPimp* GetRoot();
  PPimp* Extend(PPimp* p, LabelId wi);
  PPimp* Extend(PPimp* p, const std::string w, size_t voc) {
    return Extend(p, ConvertWord(w,voc));
  }
private:
  Data  m_Data;
  FILE* m_FileSrc;
  FILE* m_FileTgt;

  std::vector<WordVoc*> m_Voc;
  ObjectPool<PPimp>     m_PtrPool;
  std::map<std::string,WordVoc> m_vocs;
};

}

#endif
