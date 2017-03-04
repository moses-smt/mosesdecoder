#include <iostream>
#include <string>

#include "moses/Phrase.h"
#include "moses/FactorCollection.h"
#include "moses/Timer.h"
#include "moses/InputFileStream.h"
#include "moses/TranslationModel/CompactPT/BlockHashIndex.h"
#include "moses/TranslationModel/CompactPT/CanonicalHuffman.h"
#include "moses/TranslationModel/CompactPT/StringVector.h"

using namespace Moses;
using namespace std;

Timer timer;

FactorList m_factorsF, m_factorsE, m_factorsC;

BlockHashIndex m_hash(10, 16);
size_t m_numScoreComponent;
bool m_multipleScoreTrees;
bool m_inMemory = false;

typedef CanonicalHuffman<float> ScoreTree;
std::vector<ScoreTree*> m_scoreTrees;

StringVector<unsigned char, unsigned long, MmapAllocator>  m_scoresMapped;
StringVector<unsigned char, unsigned long, std::allocator> m_scoresMemory;

////////////////////////////////////////////////////////////////////////////////////
void Load(const string &filePath)
{
  std::FILE* pFile = std::fopen(filePath.c_str(), "r");
  UTIL_THROW_IF2(pFile == NULL, "File " << filePath << " could not be opened");

  //if(m_inMemory)
  m_hash.Load(pFile);
  //else
  //m_hash.LoadIndex(pFile);

  size_t read = 0;
  read += std::fread(&m_numScoreComponent, sizeof(m_numScoreComponent), 1, pFile);
  read += std::fread(&m_multipleScoreTrees,
                     sizeof(m_multipleScoreTrees), 1, pFile);

  if(m_multipleScoreTrees) {
    m_scoreTrees.resize(m_numScoreComponent);
    for(size_t i = 0; i < m_numScoreComponent; i++)
      m_scoreTrees[i] = new CanonicalHuffman<float>(pFile);
  } else {
    m_scoreTrees.resize(1);
    m_scoreTrees[0] = new CanonicalHuffman<float>(pFile);
  }

  if(m_inMemory)
    m_scoresMemory.load(pFile, false);
  else
    m_scoresMapped.load(pFile, true);

}

////////////////////////////////////////////////////////////////////////////////////

std::string
MakeKey(const std::string& f,
        const std::string& e,
        const std::string& c)
{
  std::string key;
  if(!f.empty()) key += f;
  if(!m_factorsE.empty()) {
    if(!key.empty()) key += " ||| ";
    key += e;
  }
  if(!m_factorsC.empty()) {
    if(!key.empty()) key += " ||| ";
    key += c;
  }
  key += " ||| ";
  return key;
}

////////////////////////////////////////////////////////////////////////////////////

std::vector<float>
GetScore(const std::string& f, const std::string& e, const std::string& c)
{
  std::string key;
  std::vector<float> probs;

  key = MakeKey(f, e, c);

  size_t index = m_hash[key];
  if(m_hash.GetSize() != index) {
    std::string scoresString;
    if(m_inMemory)
      scoresString = m_scoresMemory[index].str();
    else
      scoresString = m_scoresMapped[index].str();


    BitWrapper<> bitStream(scoresString);
    for(size_t i = 0; i < m_numScoreComponent; i++) {
      float prob = m_scoreTrees[m_multipleScoreTrees ? i : 0]->Read(bitStream);
      prob = exp(prob);
      probs.push_back(prob);
    }

    return probs;
  } else {
    // return empty vector;
  }

  return probs;
}

////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
  string ptPath(argv[1]);
  string roPath(argv[2]);

  // lex reordering model
  m_factorsF.push_back(0);
  m_factorsE.push_back(0);

  Load(roPath);

  // phrase table
  InputFileStream ptStrm(ptPath);

  string line;
  while (getline(ptStrm, line)) {
    //cerr << line << endl;
    std::vector<std::string> columns(7);
    std::vector<std::string> toks = TokenizeMultiCharSeparator(line, "|||");
    assert(toks.size() >= 2);

    for (size_t i = 0; i < toks.size(); ++i) {
      columns[i] = Trim(toks[i]);
    }

    std::vector<float> scores = GetScore(columns[0], columns[1], "");
    // key-value pairs
    if (scores.size()) {
      if (!columns[6].empty()) {
        columns[6] += " ";
      }
      columns[6] += "{{LexRO ";
      for (size_t i = 0; i < scores.size() - 1; ++i) {
        columns[6] += Moses::SPrint(scores[i]);
        columns[6] += " ";
      }
      columns[6] += Moses::SPrint(scores[scores.size() - 1]);
      columns[6] += "}}";
    }

    // output
    for (size_t i = 0; i < columns.size() - 1; ++i) {
      cout << columns[i] << " ||| ";
    }
    cout << columns[columns.size() - 1] << endl;
  }

}


