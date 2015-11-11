#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include "extract-lex.h"
#include "InputFileStream.h"
#include "moses/Util.h"

using namespace std;
using namespace MosesTraining;

float COUNT_INCR = 1;

void fix(std::ostream& stream)
{
  stream.setf(std::ios::fixed);
  stream.precision(7);
}

int main(int argc, char* argv[])
{
  cerr << "Starting...\n";

  assert(argc == 6);
  char* &filePathTarget = argv[1];
  char* &filePathSource = argv[2];
  char* &filePathAlign  = argv[3];
  char* &filePathLexS2T = argv[4];
  char* &filePathLexT2S = argv[5];

  Moses::InputFileStream streamTarget(filePathTarget);
  Moses::InputFileStream streamSource(filePathSource);
  Moses::InputFileStream streamAlign(filePathAlign);

  ofstream streamLexS2T;
  ofstream streamLexT2S;
  streamLexS2T.open(filePathLexS2T);
  streamLexT2S.open(filePathLexT2S);

  fix(streamLexS2T);
  fix(streamLexT2S);

  ExtractLex extractSingleton;

  size_t lineCount = 0;
  string lineTarget, lineSource, lineAlign;
  while (getline(streamTarget, lineTarget)) {
    if (lineCount % 10000 == 0)
      cerr << lineCount << " ";

    istream &isSource = getline(streamSource, lineSource);
    assert(isSource);
    istream &isAlign = getline(streamAlign, lineAlign);
    assert(isAlign);

    vector<string> toksTarget, toksSource, toksAlign;
    Moses::Tokenize(toksTarget, lineTarget);
    Moses::Tokenize(toksSource, lineSource);
    Moses::Tokenize(toksAlign, lineAlign);

    /*
    cerr  << endl
          << toksTarget.size() << " " << lineTarget << endl
          << toksSource.size() << " " << lineSource << endl
          << toksAlign.size() << " " << lineAlign << endl;
    */

    extractSingleton.Process(toksTarget, toksSource, toksAlign, lineCount);

    ++lineCount;
  }

  extractSingleton.Output(streamLexS2T, streamLexT2S);

  streamTarget.Close();
  streamSource.Close();
  streamAlign.Close();
  streamLexS2T.close();
  streamLexT2S.close();

  cerr << "\nFinished\n";
}

namespace MosesTraining
{

const std::string *Vocab::GetOrAdd(const std::string &word)
{
  const string *ret = &(*m_coll.insert(word).first);
  return ret;
}

void ExtractLex::Process(vector<string> &toksTarget, vector<string> &toksSource, vector<string> &toksAlign, size_t lineCount)
{
  std::vector<bool> m_sourceAligned(toksSource.size(), false)
  , m_targetAligned(toksTarget.size(), false);

  vector<string>::const_iterator iterAlign;
  for (iterAlign = toksAlign.begin(); iterAlign != toksAlign.end(); ++iterAlign) {
    const string &alignTok = *iterAlign;

    vector<size_t> alignPos;
    Moses::Tokenize(alignPos, alignTok, "-");
    assert(alignPos.size() == 2);

    if (alignPos[0] >= toksSource.size()) {
      cerr << "ERROR: alignment over source length. Alignment " << alignPos[0] << " at line " << lineCount << endl;
      continue;
    }
    if (alignPos[1] >= toksTarget.size()) {
      cerr << "ERROR: alignment over target length. Alignment " << alignPos[1] << " at line " << lineCount << endl;
      continue;
    }

    assert(alignPos[0] < toksSource.size());
    assert(alignPos[1] < toksTarget.size());

    m_sourceAligned[ alignPos[0] ] = true;
    m_targetAligned[ alignPos[1] ] = true;

    const string &tmpSource = toksSource[ alignPos[0] ];
    const string &tmpTarget = toksTarget[ alignPos[1] ];

    const string *source = m_vocab.GetOrAdd(tmpSource);
    const string *target = m_vocab.GetOrAdd(tmpTarget);

    Process(target, source);

  }

  ProcessUnaligned(toksTarget, toksSource, m_sourceAligned, m_targetAligned);
}

void ExtractLex::Process(const std::string *target, const std::string *source)
{
  WordCount &wcS2T = m_collS2T[source];
  WordCount &wcT2S = m_collT2S[target];

  wcS2T.AddCount(COUNT_INCR);
  wcT2S.AddCount(COUNT_INCR);

  Process(wcS2T, target);
  Process(wcT2S, source);
}

void ExtractLex::Process(WordCount &wcIn, const std::string *out)
{
  std::map<const std::string*, WordCount> &collOut = wcIn.GetColl();
  WordCount &wcOut = collOut[out];
  wcOut.AddCount(COUNT_INCR);
}

void ExtractLex::ProcessUnaligned(vector<string> &toksTarget, vector<string> &toksSource
                                  , const std::vector<bool> &m_sourceAligned, const std::vector<bool> &m_targetAligned)
{
  const string *nullWord = m_vocab.GetOrAdd("NULL");

  for (size_t pos = 0; pos < m_sourceAligned.size(); ++pos) {
    bool isAlignedCurr = m_sourceAligned[pos];
    if (!isAlignedCurr) {
      const string &tmpWord = toksSource[pos];
      const string *sourceWord = m_vocab.GetOrAdd(tmpWord);

      Process(nullWord, sourceWord);
    }
  }

  for (size_t pos = 0; pos < m_targetAligned.size(); ++pos) {
    bool isAlignedCurr = m_targetAligned[pos];
    if (!isAlignedCurr) {
      const string &tmpWord = toksTarget[pos];
      const string *targetWord = m_vocab.GetOrAdd(tmpWord);

      Process(targetWord, nullWord);
    }
  }

}

void ExtractLex::Output(std::ofstream &streamLexS2T, std::ofstream &streamLexT2S)
{
  Output(m_collS2T, streamLexS2T);
  Output(m_collT2S, streamLexT2S);
}

void ExtractLex::Output(const std::map<const std::string*, WordCount> &coll, std::ofstream &outStream)
{
  std::map<const std::string*, WordCount>::const_iterator iterOuter;
  for (iterOuter = coll.begin(); iterOuter != coll.end(); ++iterOuter) {
    const string &inStr = *iterOuter->first;
    const WordCount &inWC = iterOuter->second;

    const std::map<const std::string*, WordCount> &outColl = inWC.GetColl();

    std::map<const std::string*, WordCount>::const_iterator iterInner;
    for (iterInner = outColl.begin(); iterInner != outColl.end(); ++iterInner) {
      const string &outStr = *iterInner->first;
      const WordCount &outWC = iterInner->second;

      float prob = outWC.GetCount() / inWC.GetCount();
      outStream << outStr << " "  << inStr << " " << prob << endl;
    }
  }
}

std::ostream& operator<<(std::ostream &out, const WordCount &obj)
{
  out << "(" << obj.GetCount() << ")";
  return out;
}

void WordCount::AddCount(float incr)
{
  m_count += incr;
}

} // namespace

