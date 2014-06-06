#ifndef moses_FF_LexicalReordering_SparseReordering_h
#define moses_FF_LexicalReordering_SparseReordering_h

/**
 * Sparse reordering features for phrase-based MT, following Cherry (NAACL, 2013)
**/


#include <map>
#include <string>
#include <vector>

#include "moses/ScoreComponentCollection.h"
#include "LexicalReorderingState.h"

/**
 Configuration of sparse reordering:
  
  The sparse reordering feature is configured using sparse-* configs in the lexical reordering line.
  sparse-words-(source|target)-<id>=<filename>  -- Features which fire for the words in the list
  sparse-clusters-(source|target)-<id>=<filename> -- Features which fire for clusters in the list. Format
                                     of cluster file TBD
  sparse-phrase                    -- Add features which depend on the current phrase (backward)
  sparse-stack                     -- Add features which depend on the previous phrase, or
                                      top of stack. (forward)
  sparse-between                   -- Add features which depend on words between previous phrase
                                      (or top of stack) and current phrase.
**/

namespace Moses
{
class SparseReordering
{
public:
  SparseReordering(const std::map<std::string,std::string>& config);
  
  //If direction is backward topt is the current option, otherwise the previous
  void CopyScores(const TranslationOption& topt,
                 LexicalReorderingState::ReorderingType reoType,
                 LexicalReorderingConfiguration::Direction direction,
                 ScoreComponentCollection* scores) const ;

private:
  typedef std::pair<std::string, std::set<std::string> > WordList; //id and list
  std::vector<WordList> m_sourceWordLists;
  std::vector<WordList> m_targetWordLists;
  bool m_usePhrase;
  bool m_useBetween;
  bool m_useStack;

  void ReadWordList(const std::string& filename, const std::string& id, std::vector<WordList>* pWordLists);

};



} //namespace


#endif
