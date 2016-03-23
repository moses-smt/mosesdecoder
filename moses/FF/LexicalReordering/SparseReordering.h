#ifndef moses_FF_LexicalReordering_SparseReordering_h
#define moses_FF_LexicalReordering_SparseReordering_h

/**
 * Sparse reordering features for phrase-based MT, following Cherry (NAACL, 2013)
**/


#include <functional>
#include <map>
#include <string>
#include <vector>

#include <boost/unordered_set.hpp>

#include "util/murmur_hash.hh"
#include "util/pool.hh"
#include "util/string_piece.hh"

#include "moses/FeatureVector.h"
#include "moses/ScoreComponentCollection.h"
#include "LRState.h"

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

/**
 * Used to store pre-calculated feature names.
**/
struct SparseReorderingFeatureKey {
  size_t id;
  enum Type {Stack, Phrase, Between} type;
  const Factor* word;
  bool isCluster;
  enum Position {First, Last} position;
  enum Side {Source, Target} side;
  LRState::ReorderingType reoType;

  SparseReorderingFeatureKey(size_t id_, Type type_, const Factor* word_, bool isCluster_,
                             Position position_, Side side_, LRState::ReorderingType reoType_)
    : id(id_), type(type_), word(word_), isCluster(isCluster_),
      position(position_), side(side_), reoType(reoType_) {
  }

  const std::string& Name(const std::string& wordListId) ;
};

struct HashSparseReorderingFeatureKey : public std::unary_function<SparseReorderingFeatureKey, std::size_t> {
  std::size_t operator()(const SparseReorderingFeatureKey& key) const {
    //TODO: can we just hash the memory?
    //not sure, there could be random padding
    std::size_t seed = 0;
    seed = util::MurmurHashNative(&key.id, sizeof(key.id), seed);
    seed = util::MurmurHashNative(&key.type, sizeof(key.type), seed);
    seed = util::MurmurHashNative(&key.word, sizeof(key.word), seed);
    seed = util::MurmurHashNative(&key.isCluster, sizeof(key.isCluster), seed);
    seed = util::MurmurHashNative(&key.position, sizeof(key.position), seed);
    seed = util::MurmurHashNative(&key.side, sizeof(key.side), seed);
    seed = util::MurmurHashNative(&key.reoType, sizeof(key.reoType), seed);
    return seed;
  }
};

struct EqualsSparseReorderingFeatureKey :
  public std::binary_function<SparseReorderingFeatureKey, SparseReorderingFeatureKey, bool> {
  bool operator()(const SparseReorderingFeatureKey& left, const SparseReorderingFeatureKey& right) const {
    //TODO: Can we just compare the memory?
    return left.id == right.id &&  left.type == right.type && left.word == right.word &&
           left.position == right.position && left.side == right.side &&
           left.reoType == right.reoType;
  }
};

class SparseReordering
{
public:
  SparseReordering(const std::map<std::string,std::string>& config, const LexicalReordering* producer);

  //If direction is backward the options will be different, for forward they will be the same
  void CopyScores(const TranslationOption& currentOpt,
                  const TranslationOption* previousOpt,
                  const InputType& input,
                  LRModel::ReorderingType reoType,
                  LRModel::Direction direction,
                  ScoreComponentCollection* scores) const ;

private:
  const LexicalReordering* m_producer;
  typedef std::pair<std::string, boost::unordered_set<const Factor*> > WordList; //id and list
  std::vector<WordList> m_sourceWordLists;
  std::vector<WordList> m_targetWordLists;
  typedef std::pair<std::string, boost::unordered_map<const Factor*, const Factor*> > ClusterMap; //id and map
  std::vector<ClusterMap> m_sourceClusterMaps;
  std::vector<ClusterMap> m_targetClusterMaps;
  bool m_usePhrase;
  bool m_useBetween;
  bool m_useStack;
  typedef boost::unordered_map<SparseReorderingFeatureKey, FName, HashSparseReorderingFeatureKey, EqualsSparseReorderingFeatureKey> FeatureMap;
  FeatureMap m_featureMap;

  typedef boost::unordered_map<std::string, float> WeightMap;
  WeightMap m_weightMap;
  bool m_useWeightMap;
  std::vector<FName> m_featureMap2;

  void ReadWordList(const std::string& filename, const std::string& id,
                    SparseReorderingFeatureKey::Side side, std::vector<WordList>* pWordLists);
  void ReadClusterMap(const std::string& filename, const std::string& id, SparseReorderingFeatureKey::Side side, std::vector<ClusterMap>* pClusterMaps);
  void PreCalculateFeatureNames(size_t index, const std::string& id, SparseReorderingFeatureKey::Side side, const Factor* factor, bool isCluster);
  void ReadWeightMap(const std::string& filename);

  void AddFeatures(
    SparseReorderingFeatureKey::Type type, SparseReorderingFeatureKey::Side side,
    const Word& word, SparseReorderingFeatureKey::Position position,
    LRModel::ReorderingType reoType,
    ScoreComponentCollection* scores) const;

};



} //namespace


#endif
