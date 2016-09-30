#include <boost/algorithm/string/predicate.hpp>
#include <boost/thread/tss.hpp>
#include "PhraseTableCompact.h"
#include "PhraseDecoder.h"
#include "../../PhraseBased/InputPath.h"
#include "../../PhraseBased/Manager.h"
#include "../../PhraseBased/TargetPhrases.h"
#include "../../PhraseBased/TargetPhraseImpl.h"
#include "../../PhraseBased/Sentence.h"

using namespace std;
using namespace boost::algorithm;

namespace Moses2
{
bool PhraseTableCompact::s_inMemoryByDefault = false;

PhraseTableCompact::PhraseTableCompact(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
,m_inMemory(s_inMemoryByDefault)
,m_useAlignmentInfo(true)
,m_hash(10, 16)
,m_phraseDecoder(0)
{
  ReadParameters();
}

PhraseTableCompact::~PhraseTableCompact()
{

}

void PhraseTableCompact::Load(System &system)
{
  std::string tFilePath = m_path;

  std::string suffix = ".minphr";
  if (!ends_with(tFilePath, suffix)) tFilePath += suffix;
  if (!FileExists(tFilePath))
    throw runtime_error("Error: File " + tFilePath + " does not exist.");

  m_phraseDecoder
  = new PhraseDecoder(*this, &m_input, &m_output, GetNumScores());

  std::FILE* pFile = std::fopen(tFilePath.c_str() , "r");

  size_t indexSize;
  //if(m_inMemory)
  // Load source phrase index into memory
  indexSize = m_hash.Load(pFile);
  // else
  // Keep source phrase index on disk
  //indexSize = m_hash.LoadIndex(pFile);

  size_t coderSize = m_phraseDecoder->Load(pFile);

  size_t phraseSize;
  if(m_inMemory) {
    // Load target phrase collections into memory
    phraseSize = m_targetPhrasesMemory.load(pFile, false);
  }
  else {
    // Keep target phrase collections on disk
    phraseSize = m_targetPhrasesMapped.load(pFile, true);
  }

  UTIL_THROW_IF2(indexSize == 0 || coderSize == 0 || phraseSize == 0,
                 "Not successfully loaded");
}

void PhraseTableCompact::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "blah") {

  }
  else {
    PhraseTable::SetParameter(key, value);
  }
}

// pb
void PhraseTableCompact::Lookup(const Manager &mgr, InputPathsBase &inputPaths) const
{
  size_t inputSize = static_cast<const Sentence&>(mgr.GetInput()).GetSize();
  InputPaths &inputPathsCast = static_cast<InputPaths&>(inputPaths);

  for (size_t i = 0; i < inputSize; ++i) {
	  for (size_t startPos = 0; startPos < inputSize; ++startPos) {
		  size_t endPos = startPos + i;
		  if (endPos >= inputSize) {
			  break;
		  }
		  InputPath *path = inputPathsCast.GetMatrix().GetValue(startPos, i);
		  cerr << "path=" << path->Debug(mgr.system) << endl;
		  Lookup(mgr, mgr.GetPool(), *path);
	  }
  }
}

TargetPhrases *PhraseTableCompact::Lookup(const Manager &mgr, MemPool &pool,
    InputPath &inputPath) const
{
  TargetPhrases *ret = NULL;

  const Phrase<Word> &sourcePhrase = inputPath.subPhrase;
  cerr << "sourcePhrase=" << sourcePhrase.Debug(mgr.system) << endl;

  // There is no souch source phrase if source phrase is longer than longest
  // observed source phrase during compilation
  if(sourcePhrase.GetSize() > m_phraseDecoder->GetMaxSourcePhraseLength())
    return ret;

  // Retrieve target phrase collection from phrase table
  TargetPhraseVectorPtr decodedPhraseColl
  = m_phraseDecoder->CreateTargetPhraseCollection(mgr, sourcePhrase, true, true);
  cerr << "decodedPhraseColl=" << decodedPhraseColl->size() << endl;

  return NULL;

  if(decodedPhraseColl != NULL && decodedPhraseColl->size()) {
    TargetPhraseVectorPtr tpv(new TargetPhraseVector(*decodedPhraseColl));
    //TargetPhraseCollection::shared_ptr  phraseColl(new TargetPhraseCollection);
    ret = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, decodedPhraseColl->size());
    cerr << "ret=" << ret->GetSize() << endl;

    for (size_t i = 0; i < decodedPhraseColl->size(); ++i) {
      //const TargetPhraseImpl *tp = decodedPhraseColl->at(i);
      //cerr << "tp=" << tp << endl;
      //ret->AddTargetPhrase(*tp);
    }
    ret->SortAndPrune(m_tableLimit);

    /*
    // Cache phrase pair for clean-up or retrieval with PREnc
    const_cast<PhraseDictionaryCompact*>(this)->CacheForCleanup(phraseColl);

    return phraseColl;
    */
  }

  return ret;

}


// scfg
void PhraseTableCompact::InitActiveChart(
    MemPool &pool,
    const SCFG::Manager &mgr,
    SCFG::InputPath &path) const
{
  UTIL_THROW2("Not implemented");
}

void PhraseTableCompact::Lookup(
    MemPool &pool,
    const SCFG::Manager &mgr,
    size_t maxChartSpan,
    const SCFG::Stacks &stacks,
    SCFG::InputPath &path) const
{
  UTIL_THROW2("Not implemented");
}

void PhraseTableCompact::LookupGivenNode(
    MemPool &pool,
    const SCFG::Manager &mgr,
    const SCFG::ActiveChartEntry &prevEntry,
    const SCFG::Word &wordSought,
    const Moses2::Hypotheses *hypos,
    const Moses2::Range &subPhraseRange,
    SCFG::InputPath &outPath) const
{
  UTIL_THROW2("Not implemented");
}

}
