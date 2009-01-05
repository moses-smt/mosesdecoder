// $Id$

#include <cassert>
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "WordsRange.h"

std::vector<float> LexicalDistortionCost::CalcScore(Hypothesis *h)
{
	std::vector<float> scores;
	std::string key;

	if(m_direction == Forward || m_direction == Bidirectional) {
		if(m_condition == Word)
			key = h->GetSourcePhrase()->GetWord(0).ToString();
		else if(m_condition == SourcePhrase)
			key = h->GetSourcePhraseStringRep();
		else if(m_condition == PhrasePair)
			key = h->GetSourcePhraseStringRep() + " ||| " + h->GetTargetPhraseStringRep();

		std::vector<float> params_cur = StaticData::Instance().GetDistortionParameters(key, Forward);
		scores.push_back(CalculateDistortionScore(
			h->GetPrevHypo()->GetCurrSourceWordsRange(),
			this->GetCurrSourceWordsRange(), params_cur);
	}

	if(m_direction == Backward || m_direction == Bidirectional) {
		if(h->GetPrevHypo()->GetPrevHypo() == NULL)
			scores.push_back(.0f);
		else {
			if(m_condition == Word) {
				const Phrase* prevSrcPhrase = h->GetPrevHypo()->GetSourcePhrase();
				key = prevSrcPhrase->GetWord(prevSrcPhrase->GetSize() - 1).ToString();
			} else if(m_condition == SourcePhrase)
				key = h->GetPrevHypo()->GetSourcePhraseStringRep();
			else if(m_condition == PhrasePair)
				key = h->GetPrevHypo()->GetSourcePhraseStringRep() + " ||| " +
					h->GetPrevHypo()->GetTargetPhraseStringRep());

			std::vector<float> params_prev = StaticData::Instance().GetDistortionParameters(key, Backward);
			scores.push_back(CalculateDistortionScore(
					h->GetPrevHypo()->GetCurrSourceWordsRange(),
					this->GetCurrSourceWordsRange(), params_prev);
		}
	}

	return scores;
}

bool LexicalDistortionCost::LoadTable()
{
  if(!FileExists(fileName) && FileExists(fileName+".gz")){
        fileName += ".gz";
  }
  InputFileStream file(fileName);
  std::string line("");
  std::cerr << "Loading distortion table into memory...";
  while(!getline(file, line).eof()){
    std::vector<std::string> tokens = TokenizeMultiCharSeparator(line, "|||");
#if !defined(CONDITION_ON_WORDS) && !defined(CONDITION_ON_SRCPHRASE)
    std::pair< std::string,std::string > e = std::make_pair(tokens.at(0), tokens.at(1));

    //last token are the probs
    std::vector<float> p = Scan<float>(Tokenize(tokens.at(2)));
#else
    std::pair< std::string,std::string > e = std::make_pair(tokens.at(0), "");
    std::vector<float> p = Scan<float>(Tokenize(tokens.at(1)));
#endif
    //sanity check: all lines must have equall number of probs
    if((int)p.size() != 4){
      TRACE_ERR( "found inconsistent number of probabilities... found " << p.size() << " expected 4" << std::endl);
      exit(0);
    }
    //save it all into our map
    float *entry = new float[4];
    entry[0] = p[0]; entry[1] = p[1]; entry[2] = p[2]; entry[3] = p[3];
    m_distortionTable[e] = entry;
  }
  std::cerr << "done.\n";
  return true;
}

float LDCBetaBinomial::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr, std::vector<float> params) const
{
        // float p = px + 2.23;
        float p = params[0] + 2;
        float q = params[1] + 2;

        int x = StaticData::Instance().GetInput()->ComputeDistortionDistance(prev, curr);
        if(x < -m_distortionRange) x = -m_distortionRange;
        if(x >  m_distortionRange) x =  m_distortionRange;
        x += m_distortionRange;

        return beta_binomial(p, q, x);
}

float LDCBetaBinomial::beta_binomial(float p, float q, int x) const
{
        const long double n = (long double) 2 * m_distortionRange;

        long double n1 = lgammal(n + 1);
        long double n2 = lgammal(q + n - x);
        long double n3 = lgammal(x + p);
        long double n4 = lgammal(p + q);
        long double d1 = lgammal(x + 1);
        long double d2 = lgammal(n - x + 1);
        long double d3 = lgammal(p + q + n);
        long double d4 = lgammal(p);
        long double d5 = lgammal(q);

        // std::cerr << n1 << " + " << n2 << " + " << n3 << " + " << n4 << std::endl;
        // std::cerr << d1 << " + " << d2 << " + " << d3 << " + " << d4 << " + " << d5 << std::endl;

        long double part1 = n1 + n4 - d3 - d4 - d5;
        // std::cerr << part1 << std::endl;
        long double part2 = n2 + n3 - d1 - d2;
        // std::cerr << part2 << std::endl;

        long double score = part1 + part2;
        // std::cerr << "x: " << x << "   score: " << score << std::endl;

        if(!finite(score)) {
                std::cerr << p << " ; " << q << std::endl;
                std::cerr << n1 << " + " << n2 << " + " << n3 << " + " << n4 << std::endl;
                std::cerr << d1 << " + " << d2 << " + " << d3 << " + " << d4 << " + " << d5 << std::endl;
                std::cerr << part1 << std::endl;
                std::cerr << part2 << std::endl;
                std::cerr << "x: " << x << "   score: " << score << std::endl;
                assert(false);
        }

        return FloorScore((float) score);
}
