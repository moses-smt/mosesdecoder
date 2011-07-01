#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include "extract-lex.h"

using namespace std;

float COUNT_INCR = 1;

int main(int argc, char* argv[])
{
  cerr << "Starting...\n";

  char* &filePathTarget = argv[1];
  char* &filePathSource = argv[2];
  char* &filePathAlign  = argv[3];
  char* &filePathLexS2T = argv[4];
  char* &filePathLexT2S = argv[5];

  ifstream streamTarget;
  ifstream streamSource;
  ifstream streamAlign;
  streamTarget.open(filePathTarget);
  streamSource.open(filePathSource);
  streamAlign.open(filePathAlign);

  ofstream streamLexS2T;
  ofstream streamLexT2S;
  streamLexS2T.open(filePathLexS2T);
  streamLexT2S.open(filePathLexT2S);

  ExtractLex extractSingleton;

  string lineTarget, lineSource, lineAlign;
  while (getline(streamTarget, lineTarget))
  {
    istream &isSource = getline(streamSource, lineSource);
    assert(isSource);
    istream &isAlign = getline(streamAlign, lineAlign);
    assert(isAlign);
    
    vector<string> toksTarget, toksSource, toksAlign;
    Tokenize(toksTarget, lineTarget);
    Tokenize(toksSource, lineSource);
    Tokenize(toksAlign, lineAlign);

    cerr  << endl
          << toksTarget.size() << " " << lineTarget << endl
          << toksSource.size() << " " << lineSource << endl 
          << toksAlign.size() << " " << lineAlign << endl;

    extractSingleton.Process(toksTarget, toksSource, toksAlign);
    
  }

  extractSingleton.Output(streamLexS2T, streamLexT2S);

  streamLexS2T.close();
  streamLexT2S.close();

  cerr << "Finished\n";
}

const std::string *Vocab::GetOrAdd(const std::string &word)
{
 	const string *ret = &(*m_coll.insert(word).first);
  return ret;
}

void ExtractLex::Process(vector<string> &toksTarget, vector<string> &toksSource, vector<string> &toksAlign)
{
  vector<string>::const_iterator iterAlign;
  for (iterAlign = toksAlign.begin(); iterAlign != toksAlign.end(); ++iterAlign)
  {
    const string &alignTok = *iterAlign;
    
    vector<size_t> alignPos;
    Tokenize(alignPos, alignTok, "-");
    assert(alignPos.size() == 2);
    assert(alignPos[0] < toksSource.size());
    assert(alignPos[1] < toksTarget.size());

    const string &tmpSource = toksSource[ alignPos[0] ];
    const string &tmpTarget = toksTarget[ alignPos[1] ];
 
    const string *source = m_vocab.GetOrAdd(tmpSource);
    const string *target = m_vocab.GetOrAdd(tmpTarget);

    Process(target, source);
    
  }

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

void ExtractLex::Output(std::ofstream &streamLexS2T, std::ofstream &streamLexT2S)
{
  Output(m_collS2T, streamLexS2T);
  Output(m_collT2S, streamLexT2S);
}

void ExtractLex::Output(const std::map<const std::string*, WordCount> &coll, std::ofstream &outStream)
{
  std::map<const std::string*, WordCount>::const_iterator iterOuter;
  for (iterOuter = coll.begin(); iterOuter != coll.end(); ++iterOuter)
  {
    const string &inStr = *iterOuter->first;
    const WordCount &inWC = iterOuter->second;

    const std::map<const std::string*, WordCount> &outColl = inWC.GetColl();

    std::map<const std::string*, WordCount>::const_iterator iterInner;
    for (iterInner = outColl.begin(); iterInner != outColl.end(); ++iterInner)
    {
      const string &outStr = *iterInner->first;
      const WordCount &outWC = iterInner->second;

      float prob = outWC.GetCount() / inWC.GetCount();
      outStream << inStr << " "  << outStr
              << " " << inWC.GetCount() << " " << outWC.GetCount() << " " << prob
              << endl;
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
  cerr << *this << endl;
}


