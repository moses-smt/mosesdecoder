#include <iostream>
#include <limits>
#include <assert.h>
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
																		 int condition, const std::vector<float> weights) :
	m_orientation(orientation), m_direction(direction), m_condition(condition), m_weights(weights),
	m_filename(filename)
{
	const_cast<ScoreIndexManager&>(StaticData::Instance()->GetScoreIndexManager()).AddScoreProducer(this);
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

float LexicalReordering::CalcScore(Hypothesis *hypothesis, int direction)
{
	if(m_direction==LexReorderType::Bidirectional || m_direction==direction){
		vector<float> val;
		//this phrase declaration is to get around const mumbo jumbo and let me call a
		//"convert to a string" method 
		Phrase myphrase = hypothesis->GetPhrase();
		int orientation = DistortionOrientation::GetOrientation(hypothesis, direction);
		if(m_condition==LexReorderType::Fe)
		{
		//this key string is be F+'|||'+E from the hypothesis	
		val=m_orientation_table[myphrase.GetStringRep(hypothesis->GetCurrSourceWordsRange())
														+"|||"
														+myphrase.GetStringRep(hypothesis->GetCurrTargetWordsRange())];
	
		}
		else
		{
			//this key string is F from the hypothesis
			val=m_orientation_table[ myphrase.GetStringRep(hypothesis->GetCurrTargetWordsRange())];
		}
		//will tell us where to look in the table for the probability we need
		int index = 0;
		//the weight will tell us what to multiply the probability we fetch by
		float weight = 1;
		int forward_offset = 0;
		//the weight vector will be longer if this LexicalReordering is bidirectional, 
		//containing backward weights then forward weights. 
		//by probing its size we can see if the LexicalReordering is bidirectional, 
		//(meaning we need to access the forward weights midvector) and
		//if it is Monotone or MSD (which changes where midvector is)
		if(m_weights.size()==5){
			forward_offset = 2;
		}
		else if(m_weights.size()==7){
			forward_offset = 3;
		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
		
		if(m_orientation==DistortionOrientationType::Msd)
		{
			if(direction==LexReorderType::Backward)
			{
				if(orientation==DistortionOrientationType::MONO)
				{
					index=BACK_M;
					weight=m_weights[1];
				}
				else if(orientation==DistortionOrientationType::SWAP)
				{
					index=BACK_S;
					weight=m_weights[2];
				}
				else
				{
					index=BACK_D;
					weight=m_weights[3];
				}
			
			}
			else
			{
				if(orientation==DistortionOrientationType::MONO)
				{
					index=FOR_M;
					weight=m_weights[1+forward_offset];					
				}
				else if(orientation==DistortionOrientationType::SWAP)
				{
					index=FOR_S;
					weight=m_weights[2+forward_offset];					
				}
				else
				{
					index=FOR_D;
					weight=m_weights[3+forward_offset];					
				}
			}
		}
		else
		{
			if(direction==LexReorderType::Backward)
			{
				if(orientation==DistortionOrientationType::MONO)
				{
					index=BACK_MONO;
					weight=m_weights[1];
				}
				else
				{
					index=BACK_NONMONO;
					weight=m_weights[2];					
				}
			}
			else
			{
				if(orientation==DistortionOrientationType::MONO)
				{
					index=FOR_MONO;
					weight=m_weights[1+forward_offset];					
				}
				else
				{
					index=FOR_NONMONO;
					weight=m_weights[2+forward_offset];				
				}
			}
		}
		return val[index] * weight;
	}
	else
	{
		return 0;
	}
}


unsigned int LexicalReordering::GetNumScoreComponents() const
{
	return 1;
}

const std::string  LexicalReordering::GetScoreProducerDescription() const
{
	return "Lexicalized reordering score, file=";
}

