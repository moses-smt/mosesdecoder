
#include <vector>
#include <assert.h>
#include "ConcatenatedPhrase.h"
#include "TargetPhrase.h"
#include "TargetPhraseCollection.h"
#include "StaticData.h"

using namespace std;

ConcatenatedPhrase::~ConcatenatedPhrase()
{
  delete m_cacheTargetPhraseColl;
}

void ConcatenatedPhrase::Set(size_t i
                           , const WordsRange &sourceRange
                           , const TargetPhrase *phrase)
{
  assert(m_coll[i] == NULL);

  m_phraseSize += phrase->GetSize();
  m_sourceRange[i] = sourceRange;
  m_coll[i] = phrase;
}

// hellper fn. create permutations
void Permutations(vector< vector<size_t> > &output, vector<size_t> &value, int N, int k)
{
  static int level = -1;
  level = level+1; value[k] = level;

  if (level == N)
		output.push_back(value);
  else
    for (int i = 0; i < N; i++)
      if (value[i] == 0)
        Permutations(output, value, N, i);

  level = level-1; value[k] = 0;
}

void Permutations(vector< vector<size_t> > &output, size_t N)
{
	vector<size_t> value(N, 0);

	Permutations(output, value, (int)N, 0);
}

const TargetPhraseCollection& ConcatenatedPhrase::CreateTargetPhrases() const
{		
  if (m_cacheTargetPhraseColl == NULL)
  {
    m_cacheTargetPhraseColl = new TargetPhraseCollection();

	  vector< vector<size_t> > permIndex;
    size_t size = GetSize();
	  Permutations(permIndex, size);

	  for (size_t indPerm = 0 ; indPerm < permIndex.size() ; ++indPerm)
	  {
		  vector<size_t> &perm = permIndex[indPerm];
  	  TargetPhrase *targetPhrase = new TargetPhrase(Output);

      for (size_t vecInd = 0; vecInd < size; ++vecInd)
		  {
        const WordsRange &sourceRange = GetSourceRange(perm[vecInd] - 1);
        const TargetPhrase &subRangePhrase = GetTargetPhrase(perm[vecInd] - 1);
				targetPhrase->Append(sourceRange, subRangePhrase);        
		  }

			// recalc lm scores
			float weightWP = StaticData::Instance().GetWeightWordPenalty();
			const LMList &lmList = StaticData::Instance().GetAllLM();
			targetPhrase->RecalcLMScore(weightWP, lmList);
		  m_cacheTargetPhraseColl->Add(targetPhrase);
	  }
  }

  return *m_cacheTargetPhraseColl;
}
