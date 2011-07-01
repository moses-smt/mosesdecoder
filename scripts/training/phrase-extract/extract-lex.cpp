#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include "extract-lex.h"

using namespace std;

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

float COUNT_INCR = 1;

void ExtractLex::Process(const std::string *target, const std::string *source)
{
  WordCount tmpWCTarget(target, COUNT_INCR);
  WordCount tmpWCSource(source, COUNT_INCR);

  Process(tmpWCSource, tmpWCTarget, m_collS2T);
  Process(tmpWCTarget, tmpWCSource, m_collT2S);
}

void ExtractLex::Process(const WordCount &in, const WordCount &out, std::map<WordCount, WordCountColl> &coll)
{
  std::map<WordCount, WordCountColl>::iterator iterMap;
  // s2t
  WordCountColl *wcColl = NULL;
  iterMap = coll.find(in);
  if (iterMap == coll.end())
  {
    wcColl = &coll[in];
  }
  else
  {
    const WordCount &wcIn = iterMap->first;

    //cerr << wcIn << endl;
    wcIn.AddCount(COUNT_INCR);
    //cerr << wcIn << endl;

    wcColl = &iterMap->second;
  }
  
  assert(in.GetCount() == COUNT_INCR);
  assert(out.GetCount() == COUNT_INCR);
  assert(wcColl);

  pair<WordCountColl::iterator, bool> iterSet = wcColl->insert(out);
  const WordCount &outWC = *iterSet.first;
  outWC.AddCount(COUNT_INCR);
}

void ExtractLex::Output(std::ofstream &streamLexS2T, std::ofstream &streamLexT2S)
{
  Output(m_collS2T, streamLexS2T);
  Output(m_collT2S, streamLexT2S);
}

void ExtractLex::Output(const std::map<WordCount, WordCountColl> &coll, std::ofstream &outStream)
{
  std::map<WordCount, WordCountColl>::const_iterator iterOuter;
  for (iterOuter = coll.begin(); iterOuter != coll.end(); ++iterOuter)
  {
    const WordCount &in = iterOuter->first;
    const WordCountColl &outColl = iterOuter->second;

    WordCountColl::const_iterator iterInner;
    for (iterInner = outColl.begin(); iterInner != outColl.end(); ++iterInner)
    {
      const WordCount &out = *iterInner;
      outStream << in.GetString() << " " << out.GetString() 
              << " " << in.GetCount() << " " << out.GetCount()
              << endl;
    }
  }
}

std::ostream& operator<<(std::ostream &out, const WordCount &obj)
{
  out << obj.GetString() << "(" << obj.GetCount() << ")";
  return out;
}

void WordCount::AddCount(float incr) const
{
  m_count += incr;
  cerr << *this << endl;
}


