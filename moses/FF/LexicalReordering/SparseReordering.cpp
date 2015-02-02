#include <fstream>

#include "moses/FactorCollection.h"
#include "moses/InputPath.h"
#include "moses/Util.h"

#include "util/exception.hh"

#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include "LexicalReordering.h"
#include "SparseReordering.h"


using namespace std;

namespace Moses
{

const std::string& SparseReorderingFeatureKey::Name (const string& wordListId)
{
  static string kSep = "-";
  static string name;
  ostringstream buf;
  // type side position id word reotype
  if (type == Phrase) {
    buf << "phr";
  } else if (type == Stack) {
    buf << "stk";
  } else if (type == Between) {
    buf << "btn";
  }
  buf << kSep;
  if (side == Source) {
    buf << "src";
  } else if (side == Target) {
    buf << "tgt";
  }
  buf << kSep;
  if (position == First) {
    buf << "first";
  } else if (position == Last) {
    buf << "last";
  }
  buf << kSep;
  buf << wordListId;
  buf << kSep;
  if (isCluster) buf << "cluster_";
  buf << word->GetString();
  buf << kSep;
  buf << reoType;
  name = buf.str();
  return name;
}

SparseReordering::SparseReordering(const map<string,string>& config, const LexicalReordering* producer)
  : m_producer(producer)
{
  static const string kSource= "source";
  static const string kTarget = "target";
  for (map<string,string>::const_iterator i = config.begin(); i != config.end(); ++i) {
    vector<string> fields = Tokenize(i->first, "-");
    if (fields[0] == "words") {
      UTIL_THROW_IF(!(fields.size() == 3), util::Exception, "Sparse reordering word list name should be sparse-words-(source|target)-<id>");
      if (fields[1] == kSource) {
        ReadWordList(i->second,fields[2], SparseReorderingFeatureKey::Source, &m_sourceWordLists);
      } else if (fields[1] == kTarget) {
        ReadWordList(i->second,fields[2],SparseReorderingFeatureKey::Target, &m_targetWordLists);
      } else {
        UTIL_THROW(util::Exception, "Sparse reordering requires source or target, not " << fields[1]);
      }
    } else if (fields[0] == "clusters") {
      UTIL_THROW_IF(!(fields.size() == 3), util::Exception, "Sparse reordering cluster name should be sparse-clusters-(source|target)-<id>");
      if (fields[1] == kSource) {
        ReadClusterMap(i->second,fields[2], SparseReorderingFeatureKey::Source, &m_sourceClusterMaps);
      } else if (fields[1] == kTarget) {
        ReadClusterMap(i->second,fields[2],SparseReorderingFeatureKey::Target, &m_targetClusterMaps);
      } else {
        UTIL_THROW(util::Exception, "Sparse reordering requires source or target, not " << fields[1]);
      }

    } else if (fields[0] == "phrase") {
      m_usePhrase = true;
    } else if (fields[0] == "stack") {
      m_useStack = true;
    } else if (fields[0] == "between") {
      m_useBetween = true;
    } else {
      UTIL_THROW(util::Exception, "Unable to parse sparse reordering option: " << i->first);
    }
  }

}

void SparseReordering::PreCalculateFeatureNames(size_t index, const string& id, SparseReorderingFeatureKey::Side side, const Factor* factor, bool isCluster)
{
  for (size_t type = SparseReorderingFeatureKey::Stack;
       type <= SparseReorderingFeatureKey::Between; ++type) {
    for (size_t position = SparseReorderingFeatureKey::First;
         position <= SparseReorderingFeatureKey::Last; ++position) {
      for (int reoType = 0; reoType <= LexicalReorderingState::MAX; ++reoType) {
        SparseReorderingFeatureKey key(
          index, static_cast<SparseReorderingFeatureKey::Type>(type), factor, isCluster,
          static_cast<SparseReorderingFeatureKey::Position>(position), side, reoType);
        m_featureMap.insert(pair<SparseReorderingFeatureKey, FName>(key,m_producer->GetFeatureName(key.Name(id))));
      }
    }
  }
}

void SparseReordering::ReadWordList(const string& filename, const string& id, SparseReorderingFeatureKey::Side side, vector<WordList>* pWordLists)
{
  ifstream fh(filename.c_str());
  UTIL_THROW_IF(!fh, util::Exception, "Unable to open: " << filename);
  string line;
  pWordLists->push_back(WordList());
  pWordLists->back().first = id;
  while (getline(fh,line)) {
    //TODO: StringPiece
    const Factor* factor = FactorCollection::Instance().AddFactor(line);
    pWordLists->back().second.insert(factor);
    PreCalculateFeatureNames(pWordLists->size()-1, id, side, factor, false);

  }
}

void SparseReordering::ReadClusterMap(const string& filename, const string& id, SparseReorderingFeatureKey::Side side, vector<ClusterMap>* pClusterMaps)
{
  pClusterMaps->push_back(ClusterMap());
  pClusterMaps->back().first = id;
  util::FilePiece file(filename.c_str());
  StringPiece line;
  while (true) {
    try {
      line = file.ReadLine();
    } catch (const util::EndOfFileException &e) {
      break;
    }
    util::TokenIter<util::SingleCharacter, true> lineIter(line,util::SingleCharacter('\t'));
    if (!lineIter) UTIL_THROW(util::Exception, "Malformed cluster line (missing word): '" << line << "'");
    const Factor* wordFactor = FactorCollection::Instance().AddFactor(*lineIter);
    ++lineIter;
    if (!lineIter) UTIL_THROW(util::Exception, "Malformed cluster line (missing cluster id): '" << line << "'");
    const Factor* idFactor = FactorCollection::Instance().AddFactor(*lineIter);
    pClusterMaps->back().second[wordFactor] = idFactor;
    PreCalculateFeatureNames(pClusterMaps->size()-1, id, side, idFactor, true);
  }
}

void SparseReordering::AddFeatures(
  SparseReorderingFeatureKey::Type type, SparseReorderingFeatureKey::Side side,
  const Word& word, SparseReorderingFeatureKey::Position position,
  LexicalReorderingState::ReorderingType reoType,
  ScoreComponentCollection* scores) const
{

  const Factor*  wordFactor = word.GetFactor(0);

  const vector<WordList>* wordLists;
  const vector<ClusterMap>* clusterMaps;
  if (side == SparseReorderingFeatureKey::Source) {
    wordLists = &m_sourceWordLists;
    clusterMaps = &m_sourceClusterMaps;
  } else {
    wordLists = &m_targetWordLists;
    clusterMaps = &m_targetClusterMaps;
  }

  for (size_t id = 0; id < wordLists->size(); ++id) {
    if ((*wordLists)[id].second.find(wordFactor) == (*wordLists)[id].second.end()) continue;
    SparseReorderingFeatureKey key(id, type, wordFactor, false, position, side, reoType);
    FeatureMap::const_iterator fmi = m_featureMap.find(key);
    assert(fmi != m_featureMap.end());
    scores->SparsePlusEquals(fmi->second, 1.0);
  }

  for (size_t id = 0; id < clusterMaps->size(); ++id) {
    const ClusterMap& clusterMap = (*clusterMaps)[id];
    boost::unordered_map<const Factor*, const Factor*>::const_iterator clusterIter
    = clusterMap.second.find(wordFactor);
    if (clusterIter != clusterMap.second.end()) {
      SparseReorderingFeatureKey key(id, type, clusterIter->second, true, position, side, reoType);
      FeatureMap::const_iterator fmi = m_featureMap.find(key);
      assert(fmi != m_featureMap.end());
      scores->SparsePlusEquals(fmi->second, 1.0);
    }
  }

}

void SparseReordering::CopyScores(
  const TranslationOption& currentOpt,
  const TranslationOption* previousOpt,
  const InputType& input,
  LexicalReorderingState::ReorderingType reoType,
  LexicalReorderingConfiguration::Direction direction,
  ScoreComponentCollection* scores) const
{
  if (m_useBetween && direction == LexicalReorderingConfiguration::Backward &&
      (reoType == LexicalReorderingState::D || reoType == LexicalReorderingState::DL ||
       reoType == LexicalReorderingState::DR)) {
    size_t gapStart, gapEnd;
    //NB: Using a static cast for speed, but could be nasty if
    //using non-sentence input
    const Sentence& sentence = static_cast<const Sentence&>(input);
    const WordsRange& currentRange = currentOpt.GetSourceWordsRange();
    if (previousOpt) {
      const WordsRange& previousRange = previousOpt->GetSourceWordsRange();
      if (previousRange < currentRange) {
        gapStart = previousRange.GetEndPos() + 1;
        gapEnd = currentRange.GetStartPos();
      } else {
        gapStart = currentRange.GetEndPos() + 1;
        gapEnd = previousRange.GetStartPos();
      }
    } else {
      //start of sentence
      gapStart = 0;
      gapEnd  = currentRange.GetStartPos();
    }
    assert(gapStart < gapEnd);
    for (size_t i = gapStart; i < gapEnd; ++i) {
      AddFeatures(SparseReorderingFeatureKey::Between,
                  SparseReorderingFeatureKey::Source, sentence.GetWord(i),
                  SparseReorderingFeatureKey::First, reoType, scores);
    }
  }
  //std::cerr << "SR " << topt << " " << reoType << " " << direction << std::endl;
  //phrase (backward)
  //stack (forward)
  SparseReorderingFeatureKey::Type type;
  if (direction == LexicalReorderingConfiguration::Forward) {
    if (!m_useStack) return;
    type = SparseReorderingFeatureKey::Stack;
  } else if (direction == LexicalReorderingConfiguration::Backward) {
    if (!m_usePhrase) return;
    type = SparseReorderingFeatureKey::Phrase;
  } else {
    //Shouldn't be called for bidirectional
    //keep compiler happy
    type = SparseReorderingFeatureKey::Phrase;
    assert(!"Shouldn't call CopyScores() with bidirectional direction");
  }
  const Phrase& sourcePhrase = currentOpt.GetInputPath().GetPhrase();
  AddFeatures(type, SparseReorderingFeatureKey::Source, sourcePhrase.GetWord(0),
              SparseReorderingFeatureKey::First, reoType, scores);
  AddFeatures(type, SparseReorderingFeatureKey::Source, sourcePhrase.GetWord(sourcePhrase.GetSize()-1), SparseReorderingFeatureKey::Last, reoType, scores);
  const Phrase& targetPhrase = currentOpt.GetTargetPhrase();
  AddFeatures(type, SparseReorderingFeatureKey::Target, targetPhrase.GetWord(0),
              SparseReorderingFeatureKey::First, reoType, scores);
  AddFeatures(type, SparseReorderingFeatureKey::Target, targetPhrase.GetWord(targetPhrase.GetSize()-1), SparseReorderingFeatureKey::Last, reoType, scores);


}

} //namespace

