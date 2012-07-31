//
//  fuzzy-match2.h
//  fuzzy-match
//
//  Created by Hieu Hoang on 25/07/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef fuzzy_match_fuzzy_match2_h
#define fuzzy_match_fuzzy_match2_h

#include <string>
#include <sstream>
#include <vector>
#include "Vocabulary.h"
#include "SuffixArray.h"
#include "Util.h"
#include "Match.h"

#define MAX_MATCH_COUNT 10000000

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
map< WORD_ID,vector< int > > single_word_index;
// global cache for word pairs
map< pair< WORD_ID, WORD_ID >, unsigned int > lsed;

void create_extract(int sentenceInd, int cost, const vector< WORD_ID > &sourceSentence, const vector<SentenceAlignment> &targets, const string &inputStr, const string  &path);



/* Letter string edit distance, e.g. sub 'their' to 'there' costs 2 */

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

#endif
