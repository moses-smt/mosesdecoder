// $Id$

#ifndef moses_PhraseDictionaryTree_h
#define moses_PhraseDictionaryTree_h

#include <string>
#include <vector>
#include <iostream>

#ifdef WITH_THREADS
#include <boost/thread/mutex.hpp>
#endif

#include "moses/TypeDef.h"
#include "moses/PrefixTree.h"
#include "moses/File.h"
#include "moses/ObjectPool.h"
#include "moses/FF/LexicalReordering/LexicalReorderingTable.h"
#include "moses/LVoc.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"

namespace Moses
{

class Phrase;
class Word;
class ConfusionNet;
class PDTimp;

typedef PrefixTreeF<LabelId,OFF_T> PTF;

//typedef std::pair<std::vector<std::string const*>,Scores > StringTgtCand;
struct StringTgtCand {
  typedef std::vector<std::string const*> Tokens;
  Tokens tokens;
  Scores scores;
  Tokens fnames;
  std::vector<FValue> fvalues;

};

/** A phrase table for phrase-based decoding that is held on disk, rather than in memory
 *  Wrapper around a PDTimp class
 */
class PhraseDictionaryTree
{
  PDTimp *imp; //implementation

  PhraseDictionaryTree(const PhraseDictionaryTree&); //not implemented
  void operator=(const PhraseDictionaryTree&); //not implemented
public:
  PhraseDictionaryTree();

  void NeedAlignmentInfo(bool a);

  void PrintWordAlignment(bool a);
  bool PrintWordAlignment();


  virtual ~PhraseDictionaryTree();

  size_t GetSize() const {
    return 0;
  }

  // convert from ascii phrase table format
  // note: only creates table, does not keep it in memory
  //        -> use Read(outFileNamePrefix);
  int Create(std::istream& in,const std::string& outFileNamePrefix);

  int Read(const std::string& fileNamePrefix);

  // free memory used by the prefix tree etc.
  void FreeMemory() const;


  /**************************************
   *   access with full source phrase   *
   **************************************/
  // print target candidates for a given phrase, mainly for debugging
  void PrintTargetCandidates(const std::vector<std::string>& src,
                             std::ostream& out) const;

  // get the target candidates for a given phrase
  void GetTargetCandidates(const std::vector<std::string>& src,
                           std::vector<StringTgtCand>& rv) const;


  // get the target candidates for a given phrase
  void GetTargetCandidates(const std::vector<std::string>& src,
                           std::vector<StringTgtCand>& rv,
                           std::vector<std::string>& wa) const;

  /*****************************
   *   access to prefix tree   *
   *****************************/

  // 'pointer' into prefix tree
  // the only permitted direct operation is a check for NULL,
  // e.g. PrefixPtr p; if(p) ...
  // other usage only through PhraseDictionaryTree-functions below

  class PrefixPtr
  {
    PPimp* imp;
    friend class PDTimp;
  public:
    PrefixPtr(PPimp* x=0) : imp(x) {}
    operator bool() const;
  };

  // return pointer to root node
  PrefixPtr GetRoot() const;
  // extend pointer with a word/Factorstring and return the resulting successor
  // pointer. If there is no such successor node, the result will evaluate to
  // false. Requirement: the input pointer p evaluates to true.
  PrefixPtr Extend(PrefixPtr p,const std::string& s) const;

  // get the target candidates for a given prefix pointer
  // requirement: the pointer has to evaluate to true
  void GetTargetCandidates(PrefixPtr p,
                           std::vector<StringTgtCand>& rv) const;
  void GetTargetCandidates(PrefixPtr p,
                           std::vector<StringTgtCand>& rv,
                           std::vector<std::string>& wa) const;

  // print target candidates for a given prefix pointer to a stream, mainly
  // for debugging
  void PrintTargetCandidates(PrefixPtr p,std::ostream& out) const;

};


}

#endif
