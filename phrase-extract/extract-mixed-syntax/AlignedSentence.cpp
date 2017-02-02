/*
 * AlignedSentence.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include <sstream>
#include "moses/Util.h"
#include "AlignedSentence.h"
#include "Parameter.h"

using namespace std;


/////////////////////////////////////////////////////////////////////////////////
AlignedSentence::AlignedSentence(int lineNum,
                                 const std::string &source,
                                 const std::string &target,
                                 const std::string &alignment)
  :m_lineNum(lineNum)
{
  PopulateWordVec(m_source, source);
  PopulateWordVec(m_target, target);
  PopulateAlignment(alignment);
}

AlignedSentence::~AlignedSentence()
{
  Moses::RemoveAllInColl(m_source);
  Moses::RemoveAllInColl(m_target);
}

void AlignedSentence::PopulateWordVec(Phrase &vec, const std::string &line)
{
  std::vector<string> toks;
  Moses::Tokenize(toks, line);

  vec.resize(toks.size());
  for (size_t i = 0; i < vec.size(); ++i) {
    const string &tok = toks[i];
    Word *word = new Word(i, tok);
    vec[i] = word;
  }
}

void AlignedSentence::PopulateAlignment(const std::string &line)
{
  vector<string> alignStr;
  Moses::Tokenize(alignStr, line);

  for (size_t i = 0; i < alignStr.size(); ++i) {
    vector<int> alignPair;
    Moses::Tokenize(alignPair, alignStr[i], "-");
    assert(alignPair.size() == 2);

    int sourcePos = alignPair[0];
    int targetPos = alignPair[1];

    if (sourcePos >= m_source.size()) {
      cerr << "ERROR1:AlignedSentence=" << Debug() << endl;
      cerr << "m_source=" << m_source.size() << endl;
      abort();
    }
    assert(sourcePos < m_source.size());
    assert(targetPos < m_target.size());
    Word *sourceWord = m_source[sourcePos];
    Word *targetWord = m_target[targetPos];

    sourceWord->AddAlignment(targetWord);
    targetWord->AddAlignment(sourceWord);
  }
}

std::string AlignedSentence::Debug() const
{
  stringstream out;
  out << "m_lineNum:";
  out << m_lineNum;
  out << endl;

  out << "m_source:";
  out << m_source.Debug();
  out << endl;

  out << "m_target:";
  out << m_target.Debug();
  out << endl;

  out << "consistent phrases:" << endl;
  out << m_consistentPhrases.Debug();
  out << endl;

  return out.str();
}

std::vector<int> AlignedSentence::GetSourceAlignmentCount() const
{
  vector<int> ret(m_source.size());

  for (size_t i = 0; i < m_source.size(); ++i) {
    const Word &word = *m_source[i];
    ret[i] = word.GetAlignmentIndex().size();
  }
  return ret;
}

void AlignedSentence::Create(const Parameter &params)
{
  CreateConsistentPhrases(params);
  m_consistentPhrases.AddHieroNonTerms(params);
}

void AlignedSentence::CreateConsistentPhrases(const Parameter &params)
{
  int countT = m_target.size();
  int countS = m_source.size();

  m_consistentPhrases.Initialize(countS);

  // check alignments for target phrase startT...endT
  for(int lengthT=1;
      lengthT <= params.maxSpan && lengthT <= countT;
      lengthT++) {
    for(int startT=0; startT < countT-(lengthT-1); startT++) {

      // that's nice to have
      int endT = startT + lengthT - 1;

      // find find aligned source words
      // first: find minimum and maximum source word
      int minS = 9999;
      int maxS = -1;
      vector< int > usedS = GetSourceAlignmentCount();
      for(int ti=startT; ti<=endT; ti++) {
        const Word &word = *m_target[ti];
        const std::set<int> &alignment = word.GetAlignmentIndex();

        std::set<int>::const_iterator iterAlign;
        for(iterAlign = alignment.begin(); iterAlign != alignment.end(); ++iterAlign) {
          int si = *iterAlign;
          if (si<minS) {
            minS = si;
          }
          if (si>maxS) {
            maxS = si;
          }
          usedS[ si ]--;
        }
      }

      // unaligned phrases are not allowed
      if( maxS == -1 )
        continue;

      // source phrase has to be within limits
      size_t width = maxS - minS + 1;

      if( width < params.minSpan )
        continue;

      if( width > params.maxSpan )
        continue;

      // check if source words are aligned to out of bound target words
      bool out_of_bounds = false;
      for(int si=minS; si<=maxS && !out_of_bounds; si++)
        if (usedS[si]>0) {
          out_of_bounds = true;
        }

      // if out of bound, you gotta go
      if (out_of_bounds)
        continue;

      // done with all the checks, lets go over all consistent phrase pairs
      // start point of source phrase may retreat over unaligned
      for(int startS=minS;
          (startS>=0 &&
           startS>maxS - params.maxSpan && // within length limit
           (startS==minS || m_source[startS]->GetAlignment().size()==0)); // unaligned
          startS--) {
        // end point of source phrase may advance over unaligned
        for(int endS=maxS;
            (endS<countS && endS<startS + params.maxSpan && // within length limit
             (endS==maxS || m_source[endS]->GetAlignment().size()==0)); // unaligned
            endS++) {

          // take note that this is a valid phrase alignment
          m_consistentPhrases.Add(startS, endS, startT, endT, params);
        }
      }
    }
  }
}
