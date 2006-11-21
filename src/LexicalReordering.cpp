// $Id$

#include <iostream>
#include <limits>
#include <cassert>
#include <vector>
#include <algorithm>
#include "LexicalReordering.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "Util.h"

using namespace std;

/** Load the file pointed to by filePath; set up the table according to
  * the orientation and condition parameters. Direction will be used
  * later for computing the score.
  * \param filePath file that contains the table
  * \param orientation orientation as defined in DistortionOrientationType (monotone/msd)
  * \param direction direction as defined in LexReorderType (forward/backward/bidirectional)
  * \param condition either conditioned on foreign or foreign+english
  * \param weights weight setting for this model
  * \param input input factors
  * \param output output factors
  */
LexicalReordering::LexicalReordering(const std::string &filePath, 
																		 int orientation, int direction,
																		 int condition, const std::vector<float>& weights,
																		 vector<FactorType> input, vector<FactorType> output) :
	m_orientation(orientation), m_condition(condition), m_numScores(weights.size()), m_filePath(filePath), m_sourceFactors(input), m_targetFactors(output)
{
	//add score producer
	const_cast<ScoreIndexManager&>(StaticData::Instance()->GetScoreIndexManager()).AddScoreProducer(this);
	//manage the weights by SetWeightsForScoreProducer method of static data.
	if(direction == LexReorderType::Bidirectional)
	{
		m_direction.push_back(LexReorderType::Backward); // this order is important
		m_direction.push_back(LexReorderType::Forward);
	}
	else
	{
		m_direction.push_back(direction);
	}
	// set number of orientations
	if( orientation == DistortionOrientationType::Monotone) {
		m_numOrientationTypes = 2;
	}
	else if ( orientation == DistortionOrientationType::Msd) {
		m_numOrientationTypes = 3;
	}
	const_cast<StaticData*>(StaticData::Instance())->SetWeightsForScoreProducer(this, weights);
	// Load the file
	LoadFile();
	//	PrintTable();
}


/** Loads the orientation file into a map 
  */
void LexicalReordering::LoadFile()
{
	InputFileStream inFile(m_filePath);
	string line = "", key = "";
	while (getline(inFile,line))
		{
			vector<string> tokens = TokenizeMultiCharSeparator(line , "|||");
			string f = "", e = "";
			// For storing read-in probabilities.
			vector<float> probs;
			if (m_condition == LexReorderType::Fe)
				{
					f = tokens[FE_FOREIGN];
					e = tokens[FE_ENGLISH];
					// if condition is "fe", then concatenate the first two tokens
					// to make a single token
					key = f + "|||" + e;
					probs = Scan<float>(Tokenize(tokens[FE_PROBS]));
				}
			else
				{
					// otherwise, key is just foreign
					f = tokens[F_FOREIGN];
					key = f;
					probs = Scan<float>(Tokenize(tokens[F_PROBS]));
	
				}
			if (probs.size() != m_direction.size() * m_numOrientationTypes) {
				TRACE_ERR( "found " << probs.size() << " probabilities, expected " 
									<< m_direction.size() * m_numOrientationTypes << endl);
				exit(0);
			}
			std::vector<float> scv(probs.size());
			std::transform(probs.begin(),probs.end(),probs.begin(),TransformScore);
			m_orientation_table[key] = probs;
		}
	inFile.Close();
}

/** print the table in a readable format (not used at this point)
  */
void LexicalReordering::PrintTable()
{
	// iterate over map
	map<string, vector<float> >::iterator table_iter = 
		m_orientation_table.begin(); 
	while (table_iter != m_orientation_table.end())
			{
				// print key
				cout << table_iter->first << " ||| ";
				// print values
				vector<float> val = table_iter->second;
				int i=0, num_probs = val.size();
				while (i<num_probs-1)
					{
						cout << val[i] << " ";
						i++;
					}
				cout << val[i] << endl;
				table_iter++;
		}
}

/** compute the orientation given a hypothesis 
  */
int LexicalReordering::GetOrientation(const Hypothesis *curr_hypothesis) 
{
	const Hypothesis *prevHypo = curr_hypothesis->GetPrevHypo();

	const WordsRange &currSourceRange = curr_hypothesis->GetCurrSourceWordsRange();
	size_t curr_source_start = currSourceRange.GetStartPos();
	size_t curr_source_end = currSourceRange.GetEndPos();

	//if there's no previous source...
	if(prevHypo->GetId() == 0){
		if (curr_source_start == 0) 
		{
			return ORIENTATION_MONOTONE;
		}
		else {
			return ORIENTATION_DISCONTINUOUS;
		}
	}


	const WordsRange &prevSourceRange = prevHypo->GetCurrSourceWordsRange();
	size_t prev_source_start = prevSourceRange.GetStartPos();
	size_t prev_source_end = prevSourceRange.GetEndPos();		
	if(prev_source_end==curr_source_start-1)
	{
		return ORIENTATION_MONOTONE;
	}
	// distinguish between monotone, swap, discontinuous
	else if(m_orientation==DistortionOrientationType::Msd) 
	{
		if(prev_source_start==curr_source_end+1)
		{
			return ORIENTATION_SWAP;
		}
		else
		{
			return ORIENTATION_DISCONTINUOUS;
		}
	}
	// only distinguish between monotone, non monotone
	else
	{
		return ORIENTATION_NON_MONOTONE;
	}
}

/** calculate the score(s) for a hypothesis 
  */
std::vector<float> LexicalReordering::CalcScore(Hypothesis *hypothesis)
{
	std::vector<float> score(m_numScores, 0);
	for(unsigned int i=0; i < m_direction.size(); i++) // backward, forward, or both 
	{
	  vector<float> val; // we will score the matching probability here
		
		// FIRST, get probability distribution

	  int direction = m_direction[i]; // either backward or forward

		// no score, if we would have to compute the forward score from the initial hypothesis
	  if (direction == LexReorderType::Backward || hypothesis->GetPrevHypo()->GetId() != 0) {

			if (direction == LexReorderType::Backward) { 
				// conditioned on both foreign and English
				if(m_condition==LexReorderType::Fe)
					{
						//this key string is F+'|||'+E from the hypothesis
						val=m_orientation_table[hypothesis->GetSourcePhraseStringRep(m_sourceFactors)
																		+"||| "
																		+hypothesis->GetTargetPhraseStringRep(m_targetFactors)];
					}
				// only conditioned on foreign
				else 
					{
						//this key string is F from the hypothesis
						val=m_orientation_table[hypothesis->GetTargetPhraseStringRep(m_sourceFactors)];
					}
			}

			// if forward looking, condition on previous phrase
			else {
				// conditioned on both foreign and English
				if(m_condition==LexReorderType::Fe)
					{
						//this key string is F+'|||'+E from the hypothesis
						val=m_orientation_table[hypothesis->GetPrevHypo()->GetSourcePhraseStringRep(m_sourceFactors)
																		+"||| "
																		+hypothesis->GetPrevHypo()->GetTargetPhraseStringRep(m_targetFactors)];
					}
				// only conditioned on foreign
				else 
					{
						//this key string is F from the hypothesis
						val=m_orientation_table[hypothesis->GetPrevHypo()->GetTargetPhraseStringRep(m_sourceFactors)];
					}
			}
	  }

		// SECOND, look up score

	  if(val.size()> 0) // valid entry
		{
			int orientation = GetOrientation(hypothesis);
			float value = val[ orientation + i * m_numOrientationTypes ];
			// one weight per direction
			if ( m_numScores < m_numOrientationTypes ) { 
				score[i] = value;
			}
			// one weight per direction and type
			else {
				score[ orientation + i * m_numOrientationTypes ] = value;
			}

			//			IFVERBOSE(3) {
			//				TRACE_ERR( "\tdistortion type " << orientation << " =>");
			//				for(unsigned int j=0;j<score.size();j++) {
			//					TRACE_ERR( " " << score[j]);
			//				}
			//				TRACE_ERR( endl);
			//			}
		}
	}
	return score;
}

/** return the number of scores produced by this model */
size_t LexicalReordering::GetNumScoreComponents() const
{
	return m_numScores;
}

/** returns description of the model */
const std::string  LexicalReordering::GetScoreProducerDescription(int idx) const
{
	return "Lexicalized reordering score, file=" + m_filePath;
}
