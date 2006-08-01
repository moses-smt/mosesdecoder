// $Id$

#include <iostream>
#include <limits>
#include <assert.h>
#include <vector>
#include "LexicalReordering.h"
#include "InputFileStream.h"
#include "DistortionOrientation.h"
#include "StaticData.h"

using namespace std;

/*
 * Load the file pointed to by filename; set up the table according to
 * the orientation and condition parameters. Direction will be used
 * later for computing the score.
 */
LexicalReordering::LexicalReordering(const std::string &filename, 
																		 int orientation, int direction,
																		 int condition, const std::vector<float>& weights) :
	m_orientation(orientation), m_condition(condition), m_filename(filename), m_numberscores(weights.size())
{
	//add score producer
	const_cast<ScoreIndexManager&>(StaticData::Instance()->GetScoreIndexManager()).AddScoreProducer(this);
	//manage the weights by SetWeightsForScoreProducer method of static data.
	if(direction == LexReorderType::Bidirectional)
	{
		m_direction.push_back(LexReorderType::Forward);
		m_direction.push_back(LexReorderType::Backward);
	}
	else
	{
		m_direction.push_back(direction);
	}
	const_cast<StaticData*>(StaticData::Instance())->SetWeightsForScoreProducer(this, weights);
	// Load the file
	LoadFile();
	PrintTable();
}


/*
 * Loads the file into a map.
 */
void LexicalReordering::LoadFile()
{
	InputFileStream inFile(m_filename);
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
			if (m_orientation == DistortionOrientationType::Monotone)
				{
					assert(probs.size() == MONO_NUM_PROBS); // 2 backward, 2 forward
				}
			else
				{
					assert(probs.size() == MSD_NUM_PROBS); // 3 backward, 3 forward
				}
			m_orientation_table[key] = probs;
		}
	inFile.Close();
}

/*
 * Print the table in a readable format.
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

std::vector<float> LexicalReordering::CalcScore(Hypothesis *hypothesis)
{
	std::vector<float> score(m_numberscores, 0);
	vector<float> val;
	for(int i=0; i < m_direction.size(); i++)
	{
		int direction = m_direction[i];
		int orientation = DistortionOrientation::GetOrientation(hypothesis, direction);
		if(m_condition==LexReorderType::Fe)
		{
		//this key string is be F+'|||'+E from the hypothesis
		val=m_orientation_table[hypothesis->GetSourcePhrase().GetStringRep(hypothesis->GetCurrSourceWordsRange())
														+"|||"
														+hypothesis->GetTargetPhrase().GetStringRep(hypothesis->GetCurrTargetWordsRange())];
	
		}
		else
		{
			//this key string is F from the hypothesis
			val=m_orientation_table[hypothesis->GetTargetPhrase().GetStringRep(hypothesis->GetCurrTargetWordsRange())];
		}
		//the forward_offset is only applicable if we have a bidirectional model
		//as the forward weights/scores come after the backward in this model, we need to offset by this amount.
		int forward_offset = 0;
		//we know we have a bidirectional model if the number of scores is 4 or 6, not 2 or 3.
		if(m_numberscores==4)
		{
			forward_offset=2;
		}
		else if(m_numberscores==6)
		{
			forward_offset=3;
		}
		if(m_orientation==DistortionOrientationType::Msd)
		{
			if(direction==LexReorderType::Backward)
			{
				if(orientation==DistortionOrientationType::MONO)
				{
					score[BACK_M] = val[BACK_M];
				}
				else if(orientation==DistortionOrientationType::SWAP)
				{
					score[BACK_S] = val[BACK_S];
				}
				else
				{
					score[BACK_D] = val[BACK_D];
				}
			
			}
			else
			{
				if(orientation==DistortionOrientationType::MONO)
				{
					score[FOR_M+forward_offset] = val[FOR_M+forward_offset];
				}
				else if(orientation==DistortionOrientationType::SWAP)
				{
					score[FOR_S+forward_offset] = val[FOR_S+forward_offset];
				}
				else
				{
					score[FOR_D+forward_offset] = val[FOR_D+forward_offset];
				}
			}
		}
		else
		{
			if(direction==LexReorderType::Backward)
			{
				if(orientation==DistortionOrientationType::MONO)
				{
					score[BACK_MONO] = val[BACK_MONO];
				}
				else
				{
					score[BACK_NONMONO] = val[BACK_NONMONO];
				}
			}
			else
			{
				if(orientation==DistortionOrientationType::MONO)
				{
					score[FOR_MONO+forward_offset] = val[FOR_MONO+forward_offset];					
				}
				else
				{
					score[FOR_NONMONO+forward_offset] = val[FOR_NONMONO+forward_offset];					
				}
			}
		}

	}
	return score;
}


unsigned int LexicalReordering::GetNumScoreComponents() const
{
	return m_numberscores;
}

const std::string  LexicalReordering::GetScoreProducerDescription() const
{
	return "Lexicalized reordering score, file=" + m_filename;
}

