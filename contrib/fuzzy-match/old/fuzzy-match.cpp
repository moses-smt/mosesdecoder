#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <time.h>

#include "Vocabulary.h"
#include "SuffixArray.h"

/** This implementation is explained in
       Koehn and Senellart: "Fast Approximate String Matching 
       with Suffix Arrays and A* Parsing" (AMTA 2010) ***/

using namespace std;

Vocabulary vocabulary;

int basic_flag = false;
int lsed_flag = true;
int refined_flag = true;
int length_filter_flag = true;
int parse_flag = true;
int min_match = 70;
int multiple_flag = false;
int multiple_slack = 0;
int multiple_max = 100;

void load_corpus( char* fileName, vector< vector< WORD_ID > > &corpus )
{
	ifstream fileStream;
	fileStream.open(fileName);
	if (!fileStream) {
		cerr << "file not found: " << fileName << endl;
		exit(1);
	}
	istream *fileStreamP = &fileStream;

	char line[LINE_MAX_LENGTH];
	while(true)
	{
		SAFE_GETLINE((*fileStreamP), line, LINE_MAX_LENGTH, '\n');
		if (fileStreamP->eof()) break;
		corpus.push_back( vocabulary.Tokenize( line ) );
	}
}


/* Letter string edit distance, e.g. sub 'their' to 'there' costs 2 */

// global cache for word pairs
map< pair< WORD_ID, WORD_ID >, unsigned int > lsed;

unsigned int letter_sed( WORD_ID aIdx, WORD_ID bIdx )
{
	// check if already computed -> lookup in cache
	pair< WORD_ID, WORD_ID > pIdx = make_pair( aIdx, bIdx );
	map< pair< WORD_ID, WORD_ID >, unsigned int >::const_iterator lookup = lsed.find( pIdx );
	if (lookup != lsed.end())
	{
		return (lookup->second);
	}

	// get surface strings for word indices
	const string &a = vocabulary.GetWord( aIdx );
	const string &b = vocabulary.GetWord( bIdx );

	// initialize cost matrix
	unsigned int **cost  = (unsigned int**) calloc( sizeof( unsigned int*  ), a.size()+1 );
	for( unsigned int i=0; i<=a.size(); i++ ) {
		cost[i] = (unsigned int*) calloc( sizeof(unsigned int), b.size()+1 );
		cost[i][0] = i;
	}
	for( unsigned int j=0; j<=b.size(); j++ ) {
		cost[0][j] = j;
	}

	// core string edit distance loop
	for( unsigned int i=1; i<=a.size(); i++ ) {
		for( unsigned int j=1; j<=b.size(); j++ ) {

			unsigned int ins = cost[i-1][j] + 1;
			unsigned int del = cost[i][j-1] + 1;
			bool match = (a.substr(i-1,1).compare( b.substr(j-1,1) ) == 0);
			unsigned int diag = cost[i-1][j-1] + (match ? 0 : 1);

			unsigned int min = (ins < del) ? ins : del;
			min = (diag < min) ? diag : min;

			cost[i][j] = min;
		}
	}

	// clear out memory
	unsigned int final = cost[a.size()][b.size()];
	for( unsigned int i=0; i<=a.size(); i++ ) {
		free( cost[i] );
	}
	free( cost );

	// cache and return result
	lsed[ pIdx ] = final;
	return final;
}

/* string edit distance implementation */

unsigned int sed( const vector< WORD_ID > &a, const vector< WORD_ID > &b, string &best_path, bool use_letter_sed ) {

	// initialize cost and path matrices
	unsigned int **cost  = (unsigned int**) calloc( sizeof( unsigned int* ), a.size()+1 );
	char **path = (char**) calloc( sizeof( char* ), a.size()+1 );
	
	for( unsigned int i=0; i<=a.size(); i++ ) {
		cost[i] = (unsigned int*) calloc( sizeof(unsigned int), b.size()+1 );
		path[i] = (char*) calloc( sizeof(char), b.size()+1 );
		if (i>0)
		{
			cost[i][0] = cost[i-1][0];
			if (use_letter_sed)
			{
				cost[i][0] += vocabulary.GetWord( a[i-1] ).size();
			}
			else
			{
				cost[i][0]++;
			}
		}
		else
		{
			cost[i][0] = 0;
		}
		path[i][0] = 'I';
	}

	for( unsigned int j=0; j<=b.size(); j++ ) {
		if (j>0) 
		{
			cost[0][j] = cost[0][j-1];
			if (use_letter_sed)
			{
				cost[0][j] +=	vocabulary.GetWord( b[j-1] ).size();
			}
			else
			{
				cost[0][j]++;
			}
		}
		else
		{
			cost[0][j] = 0;
		}
		path[0][j] = 'D';
	}

	// core string edit distance algorithm
	for( unsigned int i=1; i<=a.size(); i++ ) {
		for( unsigned int j=1; j<=b.size(); j++ ) {
			unsigned int ins = cost[i-1][j];
			unsigned int del = cost[i][j-1];
			unsigned int match;
			if (use_letter_sed)
			{
				ins += vocabulary.GetWord( a[i-1] ).size();
				del += vocabulary.GetWord( b[j-1] ).size();
				match = letter_sed( a[i-1], b[j-1] );
			}
			else
			{
				ins++;
				del++;
				match = ( a[i-1] == b[j-1] ) ? 0 : 1;
			}
			unsigned int diag = cost[i-1][j-1] + match;

			char action = (ins < del) ? 'I' : 'D';
			unsigned int min = (ins < del) ? ins : del;
			if (diag < min)
			{
				action = (match>0) ? 'S' : 'M';
				min = diag;
			}

			cost[i][j] = min;
			path[i][j] = action;
		}
	}

	// construct string for best path
	unsigned int i = a.size();
	unsigned int j = b.size();
	best_path = "";
	while( i>0 || j>0 )
	{
		best_path = path[i][j] + best_path;
		if (path[i][j] == 'I') 
		{
			i--;
		}
		else if (path[i][j] == 'D') 
		{
			j--;
		}
		else 
		{ 
			i--; 
			j--;
		}
	}
	

	// clear out memory
	unsigned int final = cost[a.size()][b.size()];

	for( unsigned int i=0; i<=a.size(); i++ ) {
		free( cost[i] );
		free( path[i] );
	}
	free( cost );
	free( path );

	// return result
	return final;
}

/* utlility function: compute length of sentence in characters 
   (spaces do not count) */

unsigned int compute_length( const vector< WORD_ID > &sentence )
{
	unsigned int length = 0; for( unsigned int i=0; i<sentence.size(); i++ )
	{
		length += vocabulary.GetWord( sentence[i] ).size();
	}
	return length;
}

/* brute force method: compare input to all corpus sentences */

int basic_fuzzy_match( vector< vector< WORD_ID > > source, 
                       vector< vector< WORD_ID > > input ) 
{
	// go through input set...
	for(unsigned int i=0;i<input.size();i++)
	{
		bool use_letter_sed = false;

		// compute sentence length and worst allowed cost
		unsigned int input_length;
		if (use_letter_sed)
		{
			input_length = compute_length( input[i] );
		}
		else
		{
			input_length = input[i].size();
		}
		unsigned int best_cost = input_length * (100-min_match) / 100 + 2;
		string best_path = "";
		int best_match = -1;

		// go through all corpus sentences
		for(unsigned int s=0;s<source.size();s++)
		{
			int source_length;
			if (use_letter_sed)
			{
				source_length = compute_length( source[s] );
			}
			else
			{
				source_length = source[s].size();
			}
			int diff = abs((int)source_length - (int)input_length);
			if (length_filter_flag && (diff >= best_cost))
			{
				continue;
			}

			// compute string edit distance
			string path;
			unsigned int cost = sed( input[i], source[s], path, use_letter_sed );

			// update if new best
			if (cost < best_cost) 
			{
				best_cost = cost;
				best_path = path;
				best_match = s;
			}
		}
		cout << best_cost << " ||| " << best_match << " ||| " << best_path << endl;
	}
}

#define MAX_MATCH_COUNT 10000000

/* data structure for n-gram match between input and corpus */

class Match {
public:
	int input_start;
	int input_end;
	int tm_start;
	int tm_end;
	int min_cost;
	int max_cost;
	int internal_cost;
	Match( int is, int ie, int ts, int te, int min, int max, int i )
		:input_start(is), input_end(ie), tm_start(ts), tm_end(te), min_cost(min), max_cost(max), internal_cost(i)
		{}
};

map< WORD_ID,vector< int > > single_word_index;

/* definition of short matches
   very short n-gram matches (1-grams) will not be looked up in
   the suffix array, since there are too many matches
   and for longer sentences, at least one 2-gram match must occur */

inline int short_match_max_length( int input_length )
{
	if ( ! refined_flag ) 
		return 0;
	if ( input_length >= 5 )
		return 1;
	return 0;	
}

/* if we have non-short matches in a sentence, we need to
   take a closer look at it. 
	 this function creates a hash map for all input words and their positions
   (to be used by the next function) 
   (done here, because this has be done only once for an input sentence) */

void init_short_matches( const vector< WORD_ID > &input )
{
	int max_length = short_match_max_length( input.size() );
	if (max_length == 0)
		return;

	single_word_index.clear();
	
	// store input words and their positions in hash map
	for(int i=0; i<input.size(); i++)
	{
		if (single_word_index.find( input[i] ) == single_word_index.end())
		{
			vector< int > position_vector;
			single_word_index[ input[i] ] = position_vector;
		}
		single_word_index[ input[i] ].push_back( i );
	}	
}

/* add all short matches to list of matches for a sentence */

void add_short_matches( vector< Match > &match, const vector< WORD_ID > &tm, int input_length, int best_cost )
{	
	int max_length = short_match_max_length( input_length );
	if (max_length == 0)
		return;

	int tm_length = tm.size();
	map< WORD_ID,vector< int > >::iterator input_word_hit;
	for(int t_pos=0; t_pos<tm.size(); t_pos++)
	{
		input_word_hit = single_word_index.find( tm[t_pos] );
		if (input_word_hit != single_word_index.end())
		{
			vector< int > &position_vector = input_word_hit->second;
			for(int j=0; j<position_vector.size(); j++)
			{
				int &i_pos = position_vector[j];

				// before match
				int max_cost = max( i_pos , t_pos );
				int min_cost = abs( i_pos - t_pos );
				if ( i_pos>0 && i_pos == t_pos ) 
					min_cost++;
				
				// after match
				max_cost += max( (input_length-i_pos) , (tm_length-t_pos));
				min_cost += abs( (input_length-i_pos) - (tm_length-t_pos));
				if ( i_pos != input_length-1 && (input_length-i_pos) == (tm_length-t_pos))
					min_cost++;
				
				if (min_cost <= best_cost)
				{
					Match new_match( i_pos,i_pos, t_pos,t_pos, min_cost,max_cost,0 );
					match.push_back( new_match );
				}
			}
		} 
	}
}

/* remove matches that are subsumed by a larger match */

vector< Match > prune_matches( const vector< Match > &match, int best_cost )
{
	//cerr << "\tpruning";
	vector< Match > pruned;
	for(int i=match.size()-1; i>=0; i--)
	{
		//cerr << " (" << match[i].input_start << "," << match[i].input_end 
		//		 << " ; " << match[i].tm_start << "," << match[i].tm_end 
		//		 << " * " << match[i].min_cost << ")";

		//if (match[i].min_cost > best_cost)
		//	continue;

		bool subsumed = false;
		for(int j=match.size()-1; j>=0; j--)
		{
			if (i!=j // do not compare match with itself
					&& ( match[i].input_end - match[i].input_start <= 
							 match[j].input_end - match[j].input_start ) // i shorter than j
					&& ((match[i].input_start == match[j].input_start &&
							 match[i].tm_start    == match[j].tm_start	) ||
							(match[i].input_end   == match[j].input_end &&
							 match[i].tm_end      == match[j].tm_end) ) )
			{
				subsumed = true;
			}
		}
		if (! subsumed && match[i].min_cost <= best_cost)
		{
			//cerr << "*";
			pruned.push_back( match[i] );
		}
	}
	//cerr << endl;
	return pruned;
}

/* A* parsing method to compute string edit distance */

int parse_matches( vector< Match > &match, int input_length, int tm_length, int &best_cost )
{	
	// cerr << "sentence has " << match.size() << " matches, best cost: " << best_cost << ", lengths input: " << input_length << " tm: " << tm_length << endl;

	if (match.size() == 1)
		return match[0].max_cost;
	if (match.size() == 0)
		return input_length+tm_length;
	
	int this_best_cost = input_length + tm_length;
	for(int i=0;i<match.size();i++)
	{
		this_best_cost = min( this_best_cost, match[i].max_cost );
	}
	// cerr << "\tthis best cost: " << this_best_cost << endl;
	
	// bottom up combination of spans
	vector< vector< Match > > multi_match;
	multi_match.push_back( match );
	
	int match_level = 1;
	while(multi_match[ match_level-1 ].size()>0)
	{
		// init vector
		vector< Match > empty;
		multi_match.push_back( empty );

		for(int first_level = 0; first_level <= (match_level-1)/2; first_level++)
		{
			int second_level = match_level - first_level -1;
			//cerr << "\tcombining level " << first_level << " and " << second_level << endl;
			
			vector< Match > &first_match  = multi_match[ first_level ];
			vector< Match > &second_match = multi_match[ second_level ];

			for(int i1 = 0; i1 < first_match.size(); i1++) {
				for(int i2 = 0; i2 < second_match.size(); i2++) {

					// do not combine the same pair twice
					if (first_level == second_level && i2 <= i1) 
					{
						continue;
					}

					// get sorted matches (first is before second)
					Match *first, *second;
					if (first_match[i1].input_start < second_match[i2].input_start )
					{
						first = &first_match[i1];
						second = &second_match[i2];
					}
					else
					{
						second = &first_match[i1];
						first = &second_match[i2];
					}

					//cerr << "\tcombining " 
					//		 << "(" << first->input_start << "," << first->input_end << "), "
					//		 << first->tm_start << " [" << first->internal_cost << "]"
					//		 << " with "
					//		 << "(" << second->input_start << "," << second->input_end << "), "
					//		 << second->tm_start<< " [" << second->internal_cost << "]"
					//		 << endl;

					// do not process overlapping matches
					if (first->input_end >= second->input_start) 
					{
						continue;
					}

					// no overlap / mismatch in tm
					if (first->tm_end >= second->tm_start)
					{
						continue;
					}

					// compute cost
					int min_cost = 0;
					int max_cost = 0;

					// initial
					min_cost += abs( first->input_start - first->tm_start );
					max_cost += max( first->input_start, first->tm_start );				 

					// same number of words, but not sent. start -> cost is at least 1 
					if (first->input_start == first->tm_start && first->input_start > 0)
					{
						min_cost++;
					}

					// in-between
					int skipped_words = second->input_start - first->input_end -1;
					int skipped_words_tm = second->tm_start - first->tm_end -1;
					int internal_cost = max( skipped_words, skipped_words_tm );
					internal_cost += first->internal_cost + second->internal_cost;
					min_cost += internal_cost;
					max_cost += internal_cost;
					
					// final
					min_cost += abs( (tm_length-1 - second->tm_end) -
													 (input_length-1 - second->input_end) );
					max_cost += max( (tm_length-1 - second->tm_end),
													 (input_length-1 - second->input_end) );

					// same number of words, but not sent. end -> cost is at least 1
					if ( ( input_length-1 - second->input_end 
								 == tm_length-1 - second->tm_end )
							 && input_length-1 != second->input_end )
					{
						min_cost++;
					}

					// cerr << "\tcost: " << min_cost << "-" << max_cost << endl;

					// if worst than best cost, forget it
					if (min_cost > best_cost)					
					{
						continue;
					}
					
					// add match
					Match new_match( first->input_start,
													 second->input_end,
													 first->tm_start,
													 second->tm_end,
													 min_cost,
													 max_cost,
													 internal_cost);
					multi_match[ match_level ].push_back( new_match );
					// cerr << "\tstored\n";
					
					// possibly updating this_best_cost
					if (max_cost < this_best_cost)
					{
						// cerr << "\tupdating this best cost to " << max_cost << "\n";
						this_best_cost = max_cost;

						// possibly updating best_cost
						if (max_cost < best_cost)
						{
							// cerr << "\tupdating best cost to " << max_cost << "\n";
							best_cost = max_cost;
						}					
					}
				}
			}
		}
		match_level++;
	}
	return this_best_cost;
}

int main(int argc, char* argv[]) 
{
	vector< vector< WORD_ID > > source, input;

	while(1) {
		static struct option long_options[] = {
			{"basic", no_argument, &basic_flag, 1},
			{"word", no_argument, &lsed_flag, 0},
			{"unrefined", no_argument, &refined_flag, 0},
			{"nolengthfilter", no_argument, &length_filter_flag, 0},
			{"noparse", no_argument, &parse_flag, 0},
			{"multiple", no_argument, &multiple_flag, 1},
			{"minmatch", required_argument, 0, 'm'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		int c = getopt_long (argc, argv, "m:", long_options, &option_index);
		if (c == -1) break;
		switch (c) {
			case 0:
//				if (long_options[option_index].flag != 0)
//					break;
//				printf ("option %s", long_options[option_index].name);
//				if (optarg)
//					printf (" with arg %s", optarg);
//				printf ("\n");
				break;
			case 'm':
				min_match = atoi(optarg);
				if (min_match < 1 || min_match > 100) {
					cerr << "error: --minmatch must have value in range 1..100\n";
					exit(1);
				}
				cerr << "setting min match to " << min_match << endl;
				break;
			default:
				cerr << "usage: syntax: ./fuzzy-match input corpus [--basic] [--word] [--minmatch 1..100]\n";
				exit(1);
		}
	}
	if (lsed_flag) { cerr << "lsed\n"; }
	if (basic_flag) { cerr << "basic\n"; }
	if (refined_flag) { cerr << "refined\n"; }
	if (length_filter_flag) { cerr << "length filter\n"; }
	if (parse_flag) { cerr << "parse\n"; }
//	exit(1);


	if (optind+2 != argc) {
		cerr << "syntax: ./fuzzy-match input corpus [--basic] [--word] [--minmatch 1..100]\n";
		exit(1);
	}
	
	cerr << "loading corpus...\n";

	load_corpus(argv[optind], input);
	load_corpus(argv[optind+1], source);

  // ./fuzzy-match input corpus [-basic] 
	
//	load_corpus("../corpus/tm.truecased.4.en", source);
//	load_corpus("../corpus/tm.truecased.4.it", target);
//	load_corpus("../evaluation/test.input.tc.4", input);

//	load_corpus("../../acquis-truecase/corpus/acquis.truecased.190.en", source);
//	load_corpus("../../acquis-truecase/evaluation/ac-test.input.tc.190", input);

//	load_corpus("../corpus/tm.truecased.16.en", source);
//	load_corpus("../evaluation/test.input.tc.16", input);

	if (basic_flag) {
		cerr << "using basic method\n";
		clock_t start_main_clock2 = clock();
		basic_fuzzy_match( source, input );
		cerr << "total: " << (1000 * (clock()-start_main_clock2) / CLOCKS_PER_SEC) << endl;
		exit(1);
	}

	cerr << "number of input sentences " << input.size() << endl;

	cerr << "creating suffix array...\n";
//	SuffixArray suffixArray( "../corpus/tm.truecased.4.en" );
//	SuffixArray suffixArray( "../../acquis-truecase/corpus/acquis.truecased.190.en" );
	SuffixArray suffixArray( argv[optind+1] );
	
	clock_t start_main_clock = clock();

	// looping through all input sentences...
	cerr << "looping...\n";
	for(unsigned int i=0;i<input.size();i++)
	{
		clock_t start_clock = clock();
		// if (i % 10 == 0) cerr << ".";
		int input_id = i; // clean up this mess!

		// establish some basic statistics

		// int input_length = compute_length( input[i] );
		int input_length = input[i].size();
		int best_cost = input_length * (100-min_match) / 100 + 1;

		int match_count = 0; // how many substring matches to be considered
		//cerr << endl << "sentence " << i << ", length " << input_length << ", best_cost " << best_cost << endl;

		// find match ranges in suffix array
		vector< vector< pair< SuffixArray::INDEX, SuffixArray::INDEX > > > match_range;
		for(size_t start=0;start<input[i].size();start++) 
		{
			SuffixArray::INDEX prior_first_match = 0;
			SuffixArray::INDEX prior_last_match = suffixArray.GetSize()-1;
			vector< string > substring;
			bool stillMatched = true;
			vector< pair< SuffixArray::INDEX, SuffixArray::INDEX > > matchedAtThisStart;
			//cerr << "start: " << start;
			for(int word=start; stillMatched && word<input[i].size(); word++)
			{
				substring.push_back( vocabulary.GetWord( input[i][word] ) );

				// only look up, if needed (i.e. no unnecessary short gram lookups)
//				if (! word-start+1 <= short_match_max_length( input_length ) )
				//			{
				SuffixArray::INDEX first_match, last_match;
				stillMatched = false;
				if (suffixArray.FindMatches( substring, first_match, last_match, prior_first_match, prior_last_match ) )
				{
					stillMatched = true;
					matchedAtThisStart.push_back( make_pair( first_match, last_match ) );
					//cerr << " (" << first_match << "," << last_match << ")";
					//cerr << " " << ( last_match - first_match + 1 );
					prior_first_match = first_match;
					prior_last_match = last_match;
				}
					//}
			}
			//cerr << endl;
			match_range.push_back( matchedAtThisStart );
		}

		clock_t clock_range = clock();

		map< int, vector< Match > > sentence_match;
		map< int, int > sentence_match_word_count;

		// go through all matches, longest first
		for(int length = input[i].size(); length >= 1; length--)
		{
			// do not create matches, if these are handled by the short match function
			if (length <= short_match_max_length( input_length ) )
			{
				continue;
			}

			unsigned int count = 0;
			for(int start = 0; start <= input[i].size() - length; start++)
			{
				if (match_range[start].size() >= length)
				{
					pair< SuffixArray::INDEX, SuffixArray::INDEX > &range = match_range[start][length-1];
					// cerr << " (" << range.first << "," << range.second << ")";
					count += range.second - range.first + 1;

					for(SuffixArray::INDEX i=range.first; i<=range.second; i++)
					{
						int position = suffixArray.GetPosition( i );

						// sentence length mismatch
						size_t sentence_id = suffixArray.GetSentence( position );
						int sentence_length = suffixArray.GetSentenceLength( sentence_id );
						int diff = abs( (int)sentence_length - (int)input_length );
						// cerr << endl << i << "\tsentence " << sentence_id << ", length " << sentence_length;
						//if (length <= 2 && input_length>=5 &&
						//		sentence_match.find( sentence_id ) == sentence_match.end())
						//	continue;

						if (diff > best_cost)
							continue;

						// compute minimal cost
						int start_pos = suffixArray.GetWordInSentence( position );
						int end_pos = start_pos + length-1;
						// cerr << endl << "\t" << start_pos << "-" << end_pos << " (" << sentence_length << ") vs. " 
						// << start << "-" << (start+length-1) << " (" << input_length << ")"; 
						// different number of prior words -> cost is at least diff
						int min_cost = abs( start - start_pos );
						
						// same number of words, but not sent. start -> cost is at least 1 
						if (start == start_pos && start>0)
							min_cost++;

						// different number of remaining words -> cost is at least diff
						min_cost += abs( ( sentence_length-1 - end_pos ) -
														 ( input_length-1 - (start+length-1) ) );

						// same number of words, but not sent. end -> cost is at least 1
						if ( sentence_length-1 - end_pos ==
								 input_length-1 - (start+length-1)
								 && end_pos != sentence_length-1 )
							min_cost++;

						// cerr << " -> min_cost " << min_cost;
						if (min_cost > best_cost)
							continue;

						// valid match
						match_count++;

						// compute maximal cost
						int max_cost = max( start, start_pos )
							+ max( sentence_length-1 - end_pos,
										 input_length-1 - (start+length-1) );
						// cerr << ", max_cost " << max_cost;
						
						Match m = Match( start, start+length-1, 
														 start_pos, start_pos+length-1, 
														 min_cost, max_cost, 0);
						sentence_match[ sentence_id ].push_back( m );
						sentence_match_word_count[ sentence_id ] += length;

						if (max_cost < best_cost)
						{
							best_cost = max_cost;
							if (best_cost == 0) break;
						}
						//if (match_count >= MAX_MATCH_COUNT) break;
					}
				}
				// cerr << endl;
				if (best_cost == 0) break;
				//if (match_count >= MAX_MATCH_COUNT) break;
			}
			// cerr << count << " matches at length " << length << " in " << sentence_match.size() << " tm." << endl;

			if (best_cost == 0) break;
			//if (match_count >= MAX_MATCH_COUNT) break;
		}
		cerr << match_count << " matches in " << sentence_match.size() << " sentences." << endl;

		clock_t clock_matches = clock();

		// consider each sentence for which we have matches
		int old_best_cost = best_cost;
		int tm_count_word_match = 0;
		int tm_count_word_match2 = 0;
		int pruned_match_count = 0;
		if (short_match_max_length( input_length ))
		{
			init_short_matches( input[i] );
		}
		vector< int > best_tm;
		typedef map< int, vector< Match > >::iterator I;

		clock_t clock_validation_sum = 0;

		for(I tm=sentence_match.begin(); tm!=sentence_match.end(); tm++)
		{
			int tmID = tm->first;
			int tm_length = suffixArray.GetSentenceLength(tmID);
			vector< Match > &match = tm->second;
			add_short_matches( match, source[tmID], input_length, best_cost );

			//cerr << "match in sentence " << tmID << ": " << match.size() << " [" << tm_length << "]" << endl;

			// quick look: how many words are matched
			int words_matched = 0;
			for(int m=0;m<match.size();m++) {

				if (match[m].min_cost <= best_cost) // makes no difference
					words_matched += match[m].input_end - match[m].input_start + 1;
			}
			if (max(input_length,tm_length) - words_matched > best_cost)
			{
				if (length_filter_flag) continue;
			}
			tm_count_word_match++;

			// prune, check again how many words are matched
			vector< Match > pruned = prune_matches( match, best_cost );
			words_matched = 0;
			for(int p=0;p<pruned.size();p++) {
				words_matched += pruned[p].input_end - pruned[p].input_start + 1;
			}
			if (max(input_length,tm_length) - words_matched > best_cost)
			{
				if (length_filter_flag) continue;
			}
			tm_count_word_match2++;

			pruned_match_count += pruned.size();
			int prior_best_cost = best_cost;
			int cost;

			clock_t clock_validation_start = clock();
			if (! parse_flag ||
			    pruned.size()>=10) // to prevent worst cases
			{
				string path;
				cost = sed( input[input_id], source[tmID], path, false );
				if (cost <  best_cost) 
				{
					best_cost = cost;
				}
			}

			else
			{
				cost = parse_matches( pruned, input_length, tm_length, best_cost );
				if (prior_best_cost != best_cost)
				{
					best_tm.clear();
				}
			}
			clock_validation_sum += clock() - clock_validation_start;
			if (cost == best_cost)
			{
				best_tm.push_back( tmID );
			}
		}
		cerr << "reduced best cost from " << old_best_cost << " to " << best_cost << endl;
		cerr << "tm considered: " << sentence_match.size()
				 << " word-matched: " << tm_count_word_match 
				 << " word-matched2: " << tm_count_word_match2 
				 << " best: " << best_tm.size() << endl;

		cerr << "pruned matches: " << ((float)pruned_match_count/(float)tm_count_word_match2) << endl;

		// do not try to find the best ... report multiple matches
		if (multiple_flag) {
			int input_letter_length = compute_length( input[input_id] );
			for(int si=0; si<best_tm.size(); si++) {
				int s = best_tm[si];
				string path;
				unsigned int letter_cost = sed( input[input_id], source[s], path, true );
				// do not report multiple identical sentences, but just their count
				cout << i << " "; // sentence number
				cout << letter_cost << "/" << input_letter_length << " ";
				cout << "(" << best_cost <<"/" << input_length <<") ";
				cout << "||| " << s << " ||| " << path << endl;
			}
			continue;
		}

		// find the best matches according to letter sed
		string best_path = "";
		int best_match = -1;
		int best_letter_cost;
		if (lsed_flag) {
			best_letter_cost = compute_length( input[input_id] ) * min_match / 100 + 1;
			for(int si=0; si<best_tm.size(); si++)
			{
				int s = best_tm[si];
				string path;
				unsigned int letter_cost = sed( input[input_id], source[s], path, true );
				if (letter_cost < best_letter_cost)
				{
					best_letter_cost = letter_cost;
					best_path = path;
					best_match = s;
				}
			}
		}
		// if letter sed turned off, just compute path for first match
		else {
			if (best_tm.size() > 0) {
				string path;
				sed( input[input_id], source[best_tm[0]], path, false );
				best_path = path;
				best_match = best_tm[0];
			}
		}
		cerr << "elapsed: " << (1000 * (clock()-start_clock) / CLOCKS_PER_SEC)
				 << " ( range: " << (1000 * (clock_range-start_clock) / CLOCKS_PER_SEC)
				 << " match: " << (1000 * (clock_matches-clock_range) / CLOCKS_PER_SEC)
				 << " tm: " << (1000 * (clock()-clock_matches) / CLOCKS_PER_SEC)
				 << " (validation: " << (1000 * (clock_validation_sum) / CLOCKS_PER_SEC) << ")"
				 << " )" << endl;
		if (lsed_flag) {
			cout << best_letter_cost << "/" << compute_length( input[input_id] ) << " (";
		}
		cout << best_cost <<"/" << input_length;
		if (lsed_flag) 	cout << ")";
		cout << " ||| " << best_match << " ||| " << best_path << endl;
	}
	cerr << "total: " << (1000 * (clock()-start_main_clock) / CLOCKS_PER_SEC) << endl;
	

}
