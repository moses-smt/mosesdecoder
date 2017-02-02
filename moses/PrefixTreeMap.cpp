#include "PrefixTreeMap.h"
#include "TypeDef.h"

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif

using namespace std;

namespace Moses
{
void GenericCandidate::readBin(FILE* f)
{
  m_PhraseList.clear();
  m_ScoreList.clear();
  uint32_t num_phrases;  // on older compilers, <stdint.h> may need to be included
  fRead(f, num_phrases);
  for(unsigned int i = 0; i < num_phrases; ++i) {
    IPhrase phrase;
    fReadVector(f, phrase);
    m_PhraseList.push_back(phrase);
  };
  uint32_t num_scores;
  fRead(f, num_scores);
  for(unsigned int j = 0; j < num_scores; ++j) {
    std::vector<float> score;
    fReadVector(f, score);
    m_ScoreList.push_back(score);
  };
};

void GenericCandidate::writeBin(FILE* f) const
{
  // cast is necessary to ensure compatibility between 32- and 64-bit platforms
  fWrite(f, static_cast<uint32_t>(m_PhraseList.size()));
  for(size_t i = 0; i < m_PhraseList.size(); ++i) {
    fWriteVector(f, m_PhraseList[i]);
  }
  fWrite(f, static_cast<uint32_t>(m_ScoreList.size()));
  for(size_t j = 0; j < m_ScoreList.size(); ++j) {
    fWriteVector(f, m_ScoreList[j]);
  }
};


void Candidates::writeBin(FILE* f) const
{
  uint32_t s = this->size();
  fWrite(f,s);
  for(size_t i = 0; i < s; ++i) {
    MyBase::operator[](i).writeBin(f);
  }
}

void Candidates::readBin(FILE* f)
{
  uint32_t s;
  fRead(f,s);
  this->resize(s);
  for(size_t i = 0; i<s; ++i) {
    MyBase::operator[](i).readBin(f);
  }
}

const LabelId PrefixTreeMap::MagicWord = std::numeric_limits<LabelId>::max() - 1;

//////////////////////////////////////////////////////////////////
PrefixTreeMap::~PrefixTreeMap()
{
  if(m_FileSrc) {
    fClose(m_FileSrc);
  }
  if(m_FileTgt) {
    fClose(m_FileTgt);
  }
  FreeMemory();
}


void PrefixTreeMap::FreeMemory()
{
  for(Data::iterator i = m_Data.begin(); i != m_Data.end(); ++i) {
    i->free();
  }
  /*for(size_t i = 0; i < m_Voc.size(); ++i){
  delete m_Voc[i];
  m_Voc[i] = 0;
  }*/
  m_PtrPool.reset();
}

WordVoc &ReadVoc(std::map<std::string,WordVoc> &vocs, const std::string& filename)
{
#ifdef WITH_THREADS
  boost::mutex mutex;
  boost::mutex::scoped_lock lock(mutex);
#endif
  std::map<std::string,WordVoc>::iterator vi = vocs.find(filename);
  if (vi == vocs.end()) {
    WordVoc &voc = vocs[filename];
    voc.Read(filename);
    return voc;
  } else {
    return vi->second;
  }
}

int PrefixTreeMap::Read(const std::string& fileNameStem, int numVocs)
{
  std::string ifs(fileNameStem + ".srctree"),
      ift(fileNameStem + ".tgtdata"),
      ifi(fileNameStem + ".idx"),
      ifv(fileNameStem + ".voc");

  std::vector<OFF_T> srcOffsets;
  FILE *ii=fOpen(ifi.c_str(),"rb");
  fReadVector(ii,srcOffsets);
  fClose(ii);

  if (m_FileSrc) {
    fClose(m_FileSrc);
  }
  m_FileSrc = fOpen(ifs.c_str(),"rb");
  if (m_FileTgt) {
    fClose(m_FileTgt);
  }
  m_FileTgt = fOpen(ift.c_str(),"rb");

  m_Data.resize(srcOffsets.size());

  for(size_t i = 0; i < m_Data.size(); ++i) {
    m_Data[i] = CPT(m_FileSrc, srcOffsets[i]);
  }

  if(-1 == numVocs) {
    char num[5];
    numVocs = 0;
    sprintf(num, "%d", numVocs);
    while(FileExists(ifv + num)) {
      ++numVocs;
      sprintf(num, "%d", numVocs);
    }
  }
  char num[5];
  m_Voc.resize(numVocs);
  for(int i = 0; i < numVocs; ++i) {
    sprintf(num, "%d", i);
    //m_Voc[i] = new WordVoc();
    //m_Voc[i]->Read(ifv + num);
    m_Voc[i] = &ReadVoc(m_vocs, ifv + num);
  }

  TRACE_ERR("binary file loaded, default OFF_T: "<< PTF::getDefault()<<"\n");
  return 1;
};


void PrefixTreeMap::GetCandidates(const IPhrase& key, Candidates* cands)
{
  //check if key is valid
  if(key.empty() || key[0] >= m_Data.size() || !m_Data[key[0]]) {
    return;
  }
  UTIL_THROW_IF2(m_Data[key[0]]->findKey(key[0]) >= m_Data[key[0]]->size(),
                 "Key not found: " << key[0]);

  OFF_T candOffset = m_Data[key[0]]->find(key);
  if(candOffset == InvalidOffT) {
    return;
  }
  fSeek(m_FileTgt,candOffset);
  cands->readBin(m_FileTgt);
}

void PrefixTreeMap::GetCandidates(const PPimp& p, Candidates* cands)
{
  UTIL_THROW_IF2(!p.isValid(), "Not a valid PPimp...");
  if(p.isRoot()) {
    return;
  };
  OFF_T candOffset = p.ptr()->getData(p.idx);
  if(candOffset == InvalidOffT) {
    return;
  }
  fSeek(m_FileTgt,candOffset);
  cands->readBin(m_FileTgt);
}

std::vector< std::string const * > PrefixTreeMap::ConvertPhrase(const IPhrase& p, unsigned int voc) const
{
  UTIL_THROW_IF2(voc >= m_Voc.size() || m_Voc[voc] == 0,
                 "Invalid vocab id: " << voc);
  std::vector< std::string const * > result;
  result.reserve(p.size());
  for(IPhrase::const_iterator i = p.begin(); i != p.end(); ++i) {
    result.push_back(&(m_Voc[voc]->symbol(*i)));
  }
  return result;
}

IPhrase PrefixTreeMap::ConvertPhrase(const std::vector< std::string >& p, unsigned int voc) const
{
  UTIL_THROW_IF2(voc >= m_Voc.size() || m_Voc[voc] == 0,
                 "Invalid vocab id: " << voc);
  IPhrase result;
  result.reserve(p.size());
  for(size_t i = 0; i < p.size(); ++i) {
    result.push_back(m_Voc[voc]->index(p[i]));
  }
  return result;
}

LabelId PrefixTreeMap::ConvertWord(const std::string& w, unsigned int voc) const
{
  UTIL_THROW_IF2(voc >= m_Voc.size() || m_Voc[voc] == 0,
                 "Invalid vocab id: " << voc);
  return m_Voc[voc]->index(w);
}

std::string PrefixTreeMap::ConvertWord(LabelId w, unsigned int voc) const
{
  UTIL_THROW_IF2(voc >= m_Voc.size() || m_Voc[voc] == 0,
                 "Invalid vocab id: " << voc);
  if(w == PrefixTreeMap::MagicWord) {
    return "|||";
  } else if (w == InvalidLabelId) {
    return "<invalid>";
  } else {
    return m_Voc[voc]->symbol(w);
  }
}

PPimp* PrefixTreeMap::GetRoot()
{
  return m_PtrPool.get(PPimp(0,0,1));
}

PPimp* PrefixTreeMap::Extend(PPimp* p, LabelId wi)
{
  UTIL_THROW_IF2(!p->isValid(), "Not a valid PPimp...");

  if(wi == InvalidLabelId) {
    return 0; // unknown word, return invalid pointer

  } else if(p->isRoot()) {
    if(wi < m_Data.size() && m_Data[wi]) {
      const void* ptr = m_Data[wi]->findKeyPtr(wi);
      UTIL_THROW_IF2(ptr == NULL, "Null pointer");
      return m_PtrPool.get(PPimp(m_Data[wi],m_Data[wi]->findKey(wi),0));
    }
  } else if(PTF const* nextP = p->ptr()->getPtr(p->idx)) {
    return m_PtrPool.get(PPimp(nextP, nextP->findKey(wi),0));
  }
  return 0; // should never get here, return invalid pointer

}

}

