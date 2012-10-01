#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <time.h>
#include <fstream>
#include "SentenceAlignment.h"
#include "fuzzy-match2.h"
#include "SuffixArray.h"

/** This implementation is explained in
       Koehn and Senellart: "Fast Approximate String Matching 
       with Suffix Arrays and A* Parsing" (AMTA 2010) ***/

using namespace std;

int main(int argc, char* argv[]) 
{
	vector< vector< WORD_ID > > source, input;
	vector< vector< SentenceAlignment > > targetAndAlignment;
	
	
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


	if (optind+4 != argc) {
		cerr << "syntax: ./fuzzy-match input source target alignment [--basic] [--word] [--minmatch 1..100]\n";
		exit(1);
	}
	
	load_corpus(argv[optind], input);
	load_corpus(argv[optind+1], source);
	load_target(argv[optind+2], targetAndAlignment);
	load_alignment(argv[optind+3], targetAndAlignment);

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
	for(unsigned int sentenceInd = 0; sentenceInd < input.size(); sentenceInd++)
	{
		clock_t start_clock = clock();
		// if (i % 10 == 0) cerr << ".";

		// establish some basic statistics

		// int input_length = compute_length( input[i] );
		int input_length = input[sentenceInd].size();
		int best_cost = input_length * (100-min_match) / 100 + 1;

		int match_count = 0; // how many substring matches to be considered
		//cerr << endl << "sentence " << i << ", length " << input_length << ", best_cost " << best_cost << endl;

		// find match ranges in suffix array
		vector< vector< pair< SuffixArray::INDEX, SuffixArray::INDEX > > > match_range;
		for(size_t start=0;start<input[sentenceInd].size();start++) 
		{
			SuffixArray::INDEX prior_first_match = 0;
			SuffixArray::INDEX prior_last_match = suffixArray.GetSize()-1;
			vector< string > substring;
			bool stillMatched = true;
			vector< pair< SuffixArray::INDEX, SuffixArray::INDEX > > matchedAtThisStart;
			//cerr << "start: " << start;
			for(int word=start; stillMatched && word<input[sentenceInd].size(); word++)
			{
				substring.push_back( vocabulary.GetWord( input[sentenceInd][word] ) );

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
		for(int length = input[sentenceInd].size(); length >= 1; length--)
		{
			// do not create matches, if these are handled by the short match function
			if (length <= short_match_max_length( input_length ) )
			{
				continue;
			}

			unsigned int count = 0;
			for(int start = 0; start <= input[sentenceInd].size() - length; start++)
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
			init_short_matches( input[sentenceInd] );
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
				cost = sed( input[sentenceInd], source[tmID], path, false );
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

    // create xml and extract files
    string inputStr, sourceStr;
    for (size_t pos = 0; pos < input_length; ++pos) {
      inputStr += vocabulary.GetWord(input[sentenceInd][pos]) + " ";
    }
    
		// do not try to find the best ... report multiple matches
		if (multiple_flag) {
			int input_letter_length = compute_length( input[sentenceInd] );
			for(int si=0; si<best_tm.size(); si++) {
				int s = best_tm[si];
				string path;
				unsigned int letter_cost = sed( input[sentenceInd], source[s], path, true );
				// do not report multiple identical sentences, but just their count
				cout << sentenceInd << " "; // sentence number
				cout << letter_cost << "/" << input_letter_length << " ";
				cout << "(" << best_cost <<"/" << input_length <<") ";
				cout << "||| " << s << " ||| " << path << endl;
        
        vector<WORD_ID> &sourceSentence = source[s];
        vector<SentenceAlignment> &targets = targetAndAlignment[s];
        create_extract(sentenceInd, best_cost, sourceSentence, targets, inputStr, path);

			}
		} // if (multiple_flag)
    else {

      // find the best matches according to letter sed
      string best_path = "";
      int best_match = -1;
      int best_letter_cost;
      if (lsed_flag) {
        best_letter_cost = compute_length( input[sentenceInd] ) * min_match / 100 + 1;
        for(int si=0; si<best_tm.size(); si++)
        {
          int s = best_tm[si];
          string path;
          unsigned int letter_cost = sed( input[sentenceInd], source[s], path, true );
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
          sed( input[sentenceInd], source[best_tm[0]], path, false );
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
        cout << best_letter_cost << "/" << compute_length( input[sentenceInd] ) << " (";
      }
      cout << best_cost <<"/" << input_length;
      if (lsed_flag) 	cout << ")";
      cout << " ||| " << best_match << " ||| " << best_path << endl;

      // creat xml & extracts
      vector<WORD_ID> &sourceSentence = source[best_match];
      vector<SentenceAlignment> &targets = targetAndAlignment[best_match];
      create_extract(sentenceInd, best_cost, sourceSentence, targets, inputStr, best_path);

    } // else if (multiple_flag)
    
    
  }
	cerr << "total: " << (1000 * (clock()-start_main_clock) / CLOCKS_PER_SEC) << endl;
	
}

void create_extract(int sentenceInd, int cost, const vector< WORD_ID > &sourceSentence, const vector<SentenceAlignment> &targets, const string &inputStr, const string  &path)
{
  string sourceStr;
  for (size_t pos = 0; pos < sourceSentence.size(); ++pos) {
    WORD_ID wordId = sourceSentence[pos];
    sourceStr += vocabulary.GetWord(wordId) + " ";
  }
    
  char *inputFileName = tmpnam(NULL);
  ofstream inputFile(inputFileName);

  for (size_t targetInd = 0; targetInd < targets.size(); ++targetInd) {
    const SentenceAlignment &sentenceAlignment = targets[targetInd]; 
    string targetStr = sentenceAlignment.getTargetString();
    string alignStr = sentenceAlignment.getAlignmentString();
    
    inputFile 
      << sentenceInd << endl
      << cost << endl
      << sourceStr << endl 
      << inputStr << endl
      << targetStr << endl
      << alignStr << endl
      << path << endl
      << sentenceAlignment.count << endl;

  }
  
  string cmd = string("perl create_xml.perl < ") + inputFileName;
  cerr << cmd << endl;
  inputFile.close();
  
}
