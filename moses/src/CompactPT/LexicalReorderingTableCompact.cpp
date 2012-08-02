#include "LexicalReorderingTableCompact.h"

namespace Moses {

LexicalReorderingTableCompact::LexicalReorderingTableCompact(
  const std::string& filePath,
  const std::vector<FactorType>& f_factors,
  const std::vector<FactorType>& e_factors,
  const std::vector<FactorType>& c_factors)
  : LexicalReorderingTable(f_factors, e_factors, c_factors),
  m_inMemory(StaticData::Instance().UseMinlexrInMemory()),
  m_numScoreComponent(6), m_multipleScoreTrees(true),
  m_hash(10, 16), m_scoreTrees(1, NULL)
{
  Load(filePath);
}

LexicalReorderingTableCompact::LexicalReorderingTableCompact(
  const std::vector<FactorType>& f_factors,
  const std::vector<FactorType>& e_factors,
  const std::vector<FactorType>& c_factors)
  : LexicalReorderingTable(f_factors, e_factors, c_factors),
  m_inMemory(StaticData::Instance().UseMinlexrInMemory()),
  m_numScoreComponent(6), m_multipleScoreTrees(true),
  m_hash(10, 16), m_scoreTrees(1, NULL)
{ }

LexicalReorderingTableCompact::~LexicalReorderingTableCompact() {
  for(size_t i = 0; i < m_scoreTrees.size(); i++)
    delete m_scoreTrees[i];
}

std::vector<float> LexicalReorderingTableCompact::GetScore(const Phrase& f,
    const Phrase& e,
    const Phrase& c)
{
  std::string key;
  Scores scores;
  
  if(0 == c.GetSize())
    key = MakeKey(f, e, c);
  else
    for(size_t i = 0; i <= c.GetSize(); ++i)
    {
      Phrase sub_c(c.GetSubString(WordsRange(i,c.GetSize()-1)));
      key = MakeKey(f,e,sub_c);
    }
    
  size_t index = m_hash[key];
  if(m_hash.GetSize() != index)
  {
    std::string scoresString;
    if(m_inMemory)
      scoresString = m_scoresMemory[index];
    else
      scoresString = m_scoresMapped[index];
      
    BitStream<> bitStream(scoresString);
    for(size_t i = 0; i < m_numScoreComponent; i++)
      scores.push_back(m_scoreTrees[m_multipleScoreTrees ? i : 0]->NextSymbol(bitStream));

    return scores;
  }

  return Scores();
}

std::string  LexicalReorderingTableCompact::MakeKey(const Phrase& f,
    const Phrase& e,
    const Phrase& c) const
{
  return MakeKey(Trim(f.GetStringRep(m_FactorsF)),
                 Trim(e.GetStringRep(m_FactorsE)),
                 Trim(c.GetStringRep(m_FactorsC)));
}

std::string  LexicalReorderingTableCompact::MakeKey(const std::string& f,
    const std::string& e,
    const std::string& c) const
{
  std::string key;
  if(!f.empty())
  {
    key += f;
  }
  if(!m_FactorsE.empty())
  {
    if(!key.empty())
    {
      key += " ||| ";
    }
    key += e;
  }
  if(!m_FactorsC.empty())
  {
    if(!key.empty())
    {
      key += " ||| ";
    }
    key += c;
  }
  key += " ||| ";
  return key;
}

void LexicalReorderingTableCompact::Load(std::string filePath)
{  
  std::FILE* pFile = std::fopen(filePath.c_str(), "r");
  if(m_inMemory)
    m_hash.Load(pFile);
  else
    m_hash.LoadIndex(pFile);
  
  std::fread(&m_numScoreComponent, sizeof(m_numScoreComponent), 1, pFile);
  std::fread(&m_multipleScoreTrees, sizeof(m_multipleScoreTrees), 1, pFile);
  
  if(m_multipleScoreTrees)
  {
    m_scoreTrees.resize(m_numScoreComponent);
    for(size_t i = 0; i < m_numScoreComponent; i++)
      m_scoreTrees[i] = new CanonicalHuffman<float>(pFile);
  }
  else
  {
    m_scoreTrees.resize(1);
    m_scoreTrees[0] = new CanonicalHuffman<float>(pFile);
  }
  
  if(m_inMemory)
    m_scoresMemory.load(pFile, false);
  else
    m_scoresMapped.load(pFile, true);
}

}