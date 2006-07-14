#include <iostream>
#include <limits>
#include <assert.h>
#include "LexicalReordering.h"
#include "InputFileStream.h"

using namespace std;

/*
 * Load the file pointed to by filename; set up the table according to
 * the orientation and condition parameters. Direction will be used
 * later for computing the score.
 */
LexicalReordering::LexicalReordering(const std::string &filename, 
																		 int orientation, int direction,
																		 int condition) :
	m_orientation(orientation), m_direction(direction), m_condition(condition),
	m_filename(filename)
{
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
			if (m_orientation == LexReorderType::Monotone)
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

/*
 * Compute the score for the current hypothesis.
 */
float LexicalReordering::CalcScore(int numSourceWords, 
																	 WordsRange &currTargetRange, 
																	 WordsRange &prevSourceRange, 
																	 WordsRange &currSourceRange)
{
	// First determine if this hypothesis is monotonic, non-monotonic,
	// swap, or discontinuous
	size_t prev_source_start = prevSourceRange.GetStartPos();
	size_t prev_source_end = prevSourceRange.GetEndPos();
	size_t curr_source_start = currSourceRange.GetStartPos();
	size_t curr_source_end = currSourceRange.GetEndPos();
	size_t curr_target_start = currTargetRange.GetStartPos();
	size_t curr_target_end = currTargetRange.GetEndPos();
	
	
}
	
