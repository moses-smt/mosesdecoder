//
//  FuzzyMatchWrapper.cpp
//  moses
//
//  Created by Hieu Hoang on 26/07/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "FuzzyMatchWrapper.h"
#include "SentenceAlignment.h"
#include "Match.h"
#include "create_xml.h"
#include "moses/Util.h"
#include "moses/StaticData.h"
#include "util/file.hh"

using namespace std;

namespace tmmt
{

FuzzyMatchWrapper::FuzzyMatchWrapper(const std::string &sourcePath, const std::string &targetPath, const std::string &alignmentPath)
  :basic_flag(false)
  ,lsed_flag(true)
  ,refined_flag(true)
  ,length_filter_flag(true)
  ,parse_flag(true)
  ,min_match(70)
  ,multiple_flag(true)
  ,multiple_slack(0)
  ,multiple_max(100)
{
  cerr << "creating suffix array" << endl;
  suffixArray = new tmmt::SuffixArray( sourcePath );

  //cerr << "loading source data" << endl;
  //load_corpus(sourcePath, source);

  cerr << "loading target data" << endl;
  load_target(targetPath, targetAndAlignment);

  cerr << "loading alignment" << endl;
  load_alignment(alignmentPath, targetAndAlignment);

  // create suffix array
  //load_corpus(m_config[0], input);

  cerr << "loading completed" << endl;
}

string FuzzyMatchWrapper::Extract(long translationId, const string &dirNameStr)
{
  const Moses::StaticData &staticData = Moses::StaticData::Instance();

  WordIndex wordIndex;

  string fuzzyMatchFile = ExtractTM(wordIndex, translationId, dirNameStr);

  // create extrac files
  create_xml(fuzzyMatchFile);

  // create phrase table with usual Moses scoring and consolidate programs
  string cmd;
  cmd = "LC_ALL=C sort " + fuzzyMatchFile + ".extract | gzip -c > "
        + fuzzyMatchFile + ".extract.sorted.gz";
  system(cmd.c_str());
  cmd = "LC_ALL=C sort " + fuzzyMatchFile + ".extract.inv | gzip -c > "
        + fuzzyMatchFile + ".extract.inv.sorted.gz";
  system(cmd.c_str());

#ifdef IS_XCODE
  cmd = "/Users/hieuhoang/unison/workspace/github/moses-smt/bin";
#elif IS_ECLIPSE
  cmd = "/home/hieu/workspace/github/moses-smt/bin";
#else
  cmd = staticData.GetBinDirectory();
#endif

  cmd += string("/../scripts/training/train-model.perl -dont-zip -first-step 6 -last-step 6 -f en -e fr -hierarchical ")
         + " -extract-file " + fuzzyMatchFile + ".extract -lexical-file - -score-options \"--NoLex\" "
         + " -phrase-translation-table " + fuzzyMatchFile + ".pt";
  system(cmd.c_str());


  return fuzzyMatchFile + ".pt.gz";
}

string FuzzyMatchWrapper::ExtractTM(WordIndex &wordIndex, long translationId, const string &dirNameStr)
{
  const std::vector< std::vector< WORD_ID > > &source = suffixArray->GetCorpus();

  string inputPath = dirNameStr + "/in";
  string fuzzyMatchFile = dirNameStr + "/fuzzyMatchFile";
  ofstream fuzzyMatchStream(fuzzyMatchFile.c_str());

  vector< vector< WORD_ID > > input;
  load_corpus(inputPath, input);

  assert(input.size() == 1);
  size_t sentenceInd = 0;

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
  for(size_t start=0; start<input[sentenceInd].size(); start++) {
    SuffixArray::INDEX prior_first_match = 0;
    SuffixArray::INDEX prior_last_match = suffixArray->GetSize()-1;
    vector< string > substring;
    bool stillMatched = true;
    vector< pair< SuffixArray::INDEX, SuffixArray::INDEX > > matchedAtThisStart;
    //cerr << "start: " << start;
    for(int word=start; stillMatched && word<input[sentenceInd].size(); word++) {
      substring.push_back( GetVocabulary().GetWord( input[sentenceInd][word] ) );

      // only look up, if needed (i.e. no unnecessary short gram lookups)
      //				if (! word-start+1 <= short_match_max_length( input_length ) )
      //			{
      SuffixArray::INDEX first_match, last_match;
      stillMatched = false;
      if (suffixArray->FindMatches( substring, first_match, last_match, prior_first_match, prior_last_match ) ) {
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
  for(int length = input[sentenceInd].size(); length >= 1; length--) {
    // do not create matches, if these are handled by the short match function
    if (length <= short_match_max_length( input_length ) ) {
      continue;
    }

    unsigned int count = 0;
    for(int start = 0; start <= input[sentenceInd].size() - length; start++) {
      if (match_range[start].size() >= length) {
        pair< SuffixArray::INDEX, SuffixArray::INDEX > &range = match_range[start][length-1];
        // cerr << " (" << range.first << "," << range.second << ")";
        count += range.second - range.first + 1;

        for(SuffixArray::INDEX i=range.first; i<=range.second; i++) {
          int position = suffixArray->GetPosition( i );

          // sentence length mismatch
          size_t sentence_id = suffixArray->GetSentence( position );
          int sentence_length = suffixArray->GetSentenceLength( sentence_id );
          int diff = abs( (int)sentence_length - (int)input_length );
          // cerr << endl << i << "\tsentence " << sentence_id << ", length " << sentence_length;
          //if (length <= 2 && input_length>=5 &&
          //		sentence_match.find( sentence_id ) == sentence_match.end())
          //	continue;

          if (diff > best_cost)
            continue;

          // compute minimal cost
          int start_pos = suffixArray->GetWordInSentence( position );
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

          if (max_cost < best_cost) {
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
  if (short_match_max_length( input_length )) {
    init_short_matches(wordIndex, translationId, input[sentenceInd] );
  }
  vector< int > best_tm;
  typedef map< int, vector< Match > >::iterator I;

  clock_t clock_validation_sum = 0;

  for(I tm=sentence_match.begin(); tm!=sentence_match.end(); tm++) {
    int tmID = tm->first;
    int tm_length = suffixArray->GetSentenceLength(tmID);
    vector< Match > &match = tm->second;
    add_short_matches(wordIndex, translationId, match, source[tmID], input_length, best_cost );

    //cerr << "match in sentence " << tmID << ": " << match.size() << " [" << tm_length << "]" << endl;

    // quick look: how many words are matched
    int words_matched = 0;
    for(int m=0; m<match.size(); m++) {

      if (match[m].min_cost <= best_cost) // makes no difference
        words_matched += match[m].input_end - match[m].input_start + 1;
    }
    if (max(input_length,tm_length) - words_matched > best_cost) {
      if (length_filter_flag) continue;
    }
    tm_count_word_match++;

    // prune, check again how many words are matched
    vector< Match > pruned = prune_matches( match, best_cost );
    words_matched = 0;
    for(int p=0; p<pruned.size(); p++) {
      words_matched += pruned[p].input_end - pruned[p].input_start + 1;
    }
    if (max(input_length,tm_length) - words_matched > best_cost) {
      if (length_filter_flag) continue;
    }
    tm_count_word_match2++;

    pruned_match_count += pruned.size();
    int prior_best_cost = best_cost;
    int cost;

    clock_t clock_validation_start = clock();
    if (! parse_flag ||
        pruned.size()>=10) { // to prevent worst cases
      string path;
      cost = sed( input[sentenceInd], source[tmID], path, false );
      if (cost <  best_cost) {
        best_cost = cost;
      }
    }

    else {
      cost = parse_matches( pruned, input_length, tm_length, best_cost );
      if (prior_best_cost != best_cost) {
        best_tm.clear();
      }
    }
    clock_validation_sum += clock() - clock_validation_start;
    if (cost == best_cost) {
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
    inputStr += GetVocabulary().GetWord(input[sentenceInd][pos]) + " ";
  }

  // do not try to find the best ... report multiple matches
  if (multiple_flag) {
    int input_letter_length = compute_length( input[sentenceInd] );
    for(int si=0; si<best_tm.size(); si++) {
      int s = best_tm[si];
      string path;
      unsigned int letter_cost = sed( input[sentenceInd], source[s], path, true );
      // do not report multiple identical sentences, but just their count
      //cout << sentenceInd << " "; // sentence number
      //cout << letter_cost << "/" << input_letter_length << " ";
      //cout << "(" << best_cost <<"/" << input_length <<") ";
      //cout << "||| " << s << " ||| " << path << endl;

      const vector<WORD_ID> &sourceSentence = source[s];
      vector<SentenceAlignment> &targets = targetAndAlignment[s];
      create_extract(sentenceInd, best_cost, sourceSentence, targets, inputStr, path, fuzzyMatchStream);

    }
  } // if (multiple_flag)
  else {

    // find the best matches according to letter sed
    string best_path = "";
    int best_match = -1;
    int best_letter_cost;
    if (lsed_flag) {
      best_letter_cost = compute_length( input[sentenceInd] ) * min_match / 100 + 1;
      for(int si=0; si<best_tm.size(); si++) {
        int s = best_tm[si];
        string path;
        unsigned int letter_cost = sed( input[sentenceInd], source[s], path, true );
        if (letter_cost < best_letter_cost) {
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
      //cout << best_letter_cost << "/" << compute_length( input[sentenceInd] ) << " (";
    }
    //cout << best_cost <<"/" << input_length;
    if (lsed_flag) {
      //cout << ")";
    }
    //cout << " ||| " << best_match << " ||| " << best_path << endl;

    if (best_match == -1) {
      UTIL_THROW_IF2(source.size() == 0, "Empty source phrase");
      best_match = 0;
    }

    // creat xml & extracts
    const vector<WORD_ID> &sourceSentence = source[best_match];
    vector<SentenceAlignment> &targets = targetAndAlignment[best_match];
    create_extract(sentenceInd, best_cost, sourceSentence, targets, inputStr, best_path, fuzzyMatchStream);

  } // else if (multiple_flag)

  fuzzyMatchStream.close();

  return fuzzyMatchFile;
}

void FuzzyMatchWrapper::load_corpus( const std::string &fileName, vector< vector< WORD_ID > > &corpus )
{
  // source
  ifstream fileStream;
  fileStream.open(fileName.c_str());
  if (!fileStream) {
    cerr << "file not found: " << fileName << endl;
    exit(1);
  }
  cerr << "loading " << fileName << endl;

  istream *fileStreamP = &fileStream;

  char line[LINE_MAX_LENGTH];
  while(true) {
    SAFE_GETLINE((*fileStreamP), line, LINE_MAX_LENGTH, '\n');
    if (fileStreamP->eof()) break;
    corpus.push_back( GetVocabulary().Tokenize( line ) );
  }
}

void FuzzyMatchWrapper::load_target(const std::string &fileName, vector< vector< SentenceAlignment > > &corpus)
{
  ifstream fileStream;
  fileStream.open(fileName.c_str());
  if (!fileStream) {
    cerr << "file not found: " << fileName << endl;
    exit(1);
  }
  cerr << "loading " << fileName << endl;

  istream *fileStreamP = &fileStream;

  WORD_ID delimiter = GetVocabulary().StoreIfNew("|||");

  int lineNum = 0;
  char line[LINE_MAX_LENGTH];
  while(true) {
    SAFE_GETLINE((*fileStreamP), line, LINE_MAX_LENGTH, '\n');
    if (fileStreamP->eof()) break;

    vector<WORD_ID> toks = GetVocabulary().Tokenize( line );

    corpus.push_back(vector< SentenceAlignment >());
    vector< SentenceAlignment > &vec = corpus.back();

    vec.push_back(SentenceAlignment());
    SentenceAlignment *sentence = &vec.back();

    const WORD &countStr = GetVocabulary().GetWord(toks[0]);
    sentence->count = atoi(countStr.c_str());

    for (size_t i = 1; i < toks.size(); ++i) {
      WORD_ID wordId = toks[i];

      if (wordId == delimiter) {
        // target and alignments can have multiple sentences.
        vec.push_back(SentenceAlignment());
        sentence = &vec.back();

        // count
        ++i;

        const WORD &countStr = GetVocabulary().GetWord(toks[i]);
        sentence->count = atoi(countStr.c_str());
      } else {
        // just a normal word, add
        sentence->target.push_back(wordId);
      }
    }

    ++lineNum;

  }

}


void FuzzyMatchWrapper::load_alignment(const std::string &fileName, vector< vector< SentenceAlignment > > &corpus )
{
  ifstream fileStream;
  fileStream.open(fileName.c_str());
  if (!fileStream) {
    cerr << "file not found: " << fileName << endl;
    exit(1);
  }
  cerr << "loading " << fileName << endl;

  istream *fileStreamP = &fileStream;

  string delimiter = "|||";

  int lineNum = 0;
  char line[LINE_MAX_LENGTH];
  while(true) {
    SAFE_GETLINE((*fileStreamP), line, LINE_MAX_LENGTH, '\n');
    if (fileStreamP->eof()) break;

    vector< SentenceAlignment > &vec = corpus[lineNum];
    size_t targetInd = 0;
    SentenceAlignment *sentence = &vec[targetInd];

    vector<string> toks = Moses::Tokenize(line);

    for (size_t i = 0; i < toks.size(); ++i) {
      string &tok = toks[i];

      if (tok == delimiter) {
        // target and alignments can have multiple sentences.
        ++targetInd;
        sentence = &vec[targetInd];

        ++i;
      } else {
        // just a normal alignment, add
        vector<int> alignPoint = Moses::Tokenize<int>(tok, "-");
        assert(alignPoint.size() == 2);
        sentence->alignment.push_back(pair<int,int>(alignPoint[0], alignPoint[1]));
      }
    }

    ++lineNum;

  }
}

bool FuzzyMatchWrapper::GetLSEDCache(const std::pair< WORD_ID, WORD_ID > &key, unsigned int &value) const
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif
  map< pair< WORD_ID, WORD_ID >, unsigned int >::const_iterator lookup = m_lsed.find( key );
  if (lookup != m_lsed.end()) {
    value = lookup->second;
    return true;
  }

  return false;
}

void FuzzyMatchWrapper::SetLSEDCache(const std::pair< WORD_ID, WORD_ID > &key, const unsigned int &value)
{
#ifdef WITH_THREADS
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
  m_lsed[ key ] = value;
}

/* Letter string edit distance, e.g. sub 'their' to 'there' costs 2 */

unsigned int FuzzyMatchWrapper::letter_sed( WORD_ID aIdx, WORD_ID bIdx )
{
  // check if already computed -> lookup in cache
  pair< WORD_ID, WORD_ID > pIdx = make_pair( aIdx, bIdx );
  unsigned int value;
  bool ret = GetLSEDCache(pIdx, value);
  if (ret) {
    return value;
  }

  // get surface strings for word indices
  const string &a = GetVocabulary().GetWord( aIdx );
  const string &b = GetVocabulary().GetWord( bIdx );

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
  SetLSEDCache(pIdx, final);
  return final;
}

/* string edit distance implementation */

unsigned int FuzzyMatchWrapper::sed( const vector< WORD_ID > &a, const vector< WORD_ID > &b, string &best_path, bool use_letter_sed )
{

  // initialize cost and path matrices
  unsigned int **cost  = (unsigned int**) calloc( sizeof( unsigned int* ), a.size()+1 );
  char **path = (char**) calloc( sizeof( char* ), a.size()+1 );

  for( unsigned int i=0; i<=a.size(); i++ ) {
    cost[i] = (unsigned int*) calloc( sizeof(unsigned int), b.size()+1 );
    path[i] = (char*) calloc( sizeof(char), b.size()+1 );
    if (i>0) {
      cost[i][0] = cost[i-1][0];
      if (use_letter_sed) {
        cost[i][0] += GetVocabulary().GetWord( a[i-1] ).size();
      } else {
        cost[i][0]++;
      }
    } else {
      cost[i][0] = 0;
    }
    path[i][0] = 'I';
  }

  for( unsigned int j=0; j<=b.size(); j++ ) {
    if (j>0) {
      cost[0][j] = cost[0][j-1];
      if (use_letter_sed) {
        cost[0][j] +=	GetVocabulary().GetWord( b[j-1] ).size();
      } else {
        cost[0][j]++;
      }
    } else {
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
      if (use_letter_sed) {
        ins += GetVocabulary().GetWord( a[i-1] ).size();
        del += GetVocabulary().GetWord( b[j-1] ).size();
        match = letter_sed( a[i-1], b[j-1] );
      } else {
        ins++;
        del++;
        match = ( a[i-1] == b[j-1] ) ? 0 : 1;
      }
      unsigned int diag = cost[i-1][j-1] + match;

      char action = (ins < del) ? 'I' : 'D';
      unsigned int min = (ins < del) ? ins : del;
      if (diag < min) {
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
  while( i>0 || j>0 ) {
    best_path = path[i][j] + best_path;
    if (path[i][j] == 'I') {
      i--;
    } else if (path[i][j] == 'D') {
      j--;
    } else {
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

unsigned int FuzzyMatchWrapper::compute_length( const vector< WORD_ID > &sentence )
{
  unsigned int length = 0;
  for( unsigned int i=0; i<sentence.size(); i++ ) {
    length += GetVocabulary().GetWord( sentence[i] ).size();
  }
  return length;
}

/* brute force method: compare input to all corpus sentences */

int FuzzyMatchWrapper::basic_fuzzy_match( vector< vector< WORD_ID > > source,
    vector< vector< WORD_ID > > input )
{
  // go through input set...
  for(unsigned int i=0; i<input.size(); i++) {
    bool use_letter_sed = false;

    // compute sentence length and worst allowed cost
    unsigned int input_length;
    if (use_letter_sed) {
      input_length = compute_length( input[i] );
    } else {
      input_length = input[i].size();
    }
    unsigned int best_cost = input_length * (100-min_match) / 100 + 2;
    string best_path = "";
    int best_match = -1;

    // go through all corpus sentences
    for(unsigned int s=0; s<source.size(); s++) {
      int source_length;
      if (use_letter_sed) {
        source_length = compute_length( source[s] );
      } else {
        source_length = source[s].size();
      }
      int diff = abs((int)source_length - (int)input_length);
      if (length_filter_flag && (diff >= best_cost)) {
        continue;
      }

      // compute string edit distance
      string path;
      unsigned int cost = sed( input[i], source[s], path, use_letter_sed );

      // update if new best
      if (cost < best_cost) {
        best_cost = cost;
        best_path = path;
        best_match = s;
      }
    }
    //cout << best_cost << " ||| " << best_match << " ||| " << best_path << endl;
  }
}

/* definition of short matches
 very short n-gram matches (1-grams) will not be looked up in
 the suffix array, since there are too many matches
 and for longer sentences, at least one 2-gram match must occur */

int FuzzyMatchWrapper::short_match_max_length( int input_length )
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

void FuzzyMatchWrapper::init_short_matches(WordIndex &wordIndex, long translationId, const vector< WORD_ID > &input )
{
  int max_length = short_match_max_length( input.size() );
  if (max_length == 0)
    return;

  wordIndex.clear();

  // store input words and their positions in hash map
  for(int i=0; i<input.size(); i++) {
    if (wordIndex.find( input[i] ) == wordIndex.end()) {
      vector< int > position_vector;
      wordIndex[ input[i] ] = position_vector;
    }
    wordIndex[ input[i] ].push_back( i );
  }
}

/* add all short matches to list of matches for a sentence */

void FuzzyMatchWrapper::add_short_matches(WordIndex &wordIndex, long translationId, vector< Match > &match, const vector< WORD_ID > &tm, int input_length, int best_cost )
{
  int max_length = short_match_max_length( input_length );
  if (max_length == 0)
    return;

  int tm_length = tm.size();
  map< WORD_ID,vector< int > >::iterator input_word_hit;
  for(int t_pos=0; t_pos<tm.size(); t_pos++) {
    input_word_hit = wordIndex.find( tm[t_pos] );
    if (input_word_hit != wordIndex.end()) {
      vector< int > &position_vector = input_word_hit->second;
      for(int j=0; j<position_vector.size(); j++) {
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

        if (min_cost <= best_cost) {
          Match new_match( i_pos,i_pos, t_pos,t_pos, min_cost,max_cost,0 );
          match.push_back( new_match );
        }
      }
    }
  }
}

/* remove matches that are subsumed by a larger match */

vector< Match > FuzzyMatchWrapper::prune_matches( const vector< Match > &match, int best_cost )
{
  //cerr << "\tpruning";
  vector< Match > pruned;
  for(int i=match.size()-1; i>=0; i--) {
    //cerr << " (" << match[i].input_start << "," << match[i].input_end
    //		 << " ; " << match[i].tm_start << "," << match[i].tm_end
    //		 << " * " << match[i].min_cost << ")";

    //if (match[i].min_cost > best_cost)
    //	continue;

    bool subsumed = false;
    for(int j=match.size()-1; j>=0; j--) {
      if (i!=j // do not compare match with itself
          && ( match[i].input_end - match[i].input_start <=
               match[j].input_end - match[j].input_start ) // i shorter than j
          && ((match[i].input_start == match[j].input_start &&
               match[i].tm_start    == match[j].tm_start	) ||
              (match[i].input_end   == match[j].input_end &&
               match[i].tm_end      == match[j].tm_end) ) ) {
        subsumed = true;
      }
    }
    if (! subsumed && match[i].min_cost <= best_cost) {
      //cerr << "*";
      pruned.push_back( match[i] );
    }
  }
  //cerr << endl;
  return pruned;
}

/* A* parsing method to compute string edit distance */

int FuzzyMatchWrapper::parse_matches( vector< Match > &match, int input_length, int tm_length, int &best_cost )
{
  // cerr << "sentence has " << match.size() << " matches, best cost: " << best_cost << ", lengths input: " << input_length << " tm: " << tm_length << endl;

  if (match.size() == 1)
    return match[0].max_cost;
  if (match.size() == 0)
    return input_length+tm_length;

  int this_best_cost = input_length + tm_length;
  for(int i=0; i<match.size(); i++) {
    this_best_cost = min( this_best_cost, match[i].max_cost );
  }
  // cerr << "\tthis best cost: " << this_best_cost << endl;

  // bottom up combination of spans
  vector< vector< Match > > multi_match;
  multi_match.push_back( match );

  int match_level = 1;
  while(multi_match[ match_level-1 ].size()>0) {
    // init vector
    vector< Match > empty;
    multi_match.push_back( empty );

    for(int first_level = 0; first_level <= (match_level-1)/2; first_level++) {
      int second_level = match_level - first_level -1;
      //cerr << "\tcombining level " << first_level << " and " << second_level << endl;

      vector< Match > &first_match  = multi_match[ first_level ];
      vector< Match > &second_match = multi_match[ second_level ];

      for(int i1 = 0; i1 < first_match.size(); i1++) {
        for(int i2 = 0; i2 < second_match.size(); i2++) {

          // do not combine the same pair twice
          if (first_level == second_level && i2 <= i1) {
            continue;
          }

          // get sorted matches (first is before second)
          Match *first, *second;
          if (first_match[i1].input_start < second_match[i2].input_start ) {
            first = &first_match[i1];
            second = &second_match[i2];
          } else {
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
          if (first->input_end >= second->input_start) {
            continue;
          }

          // no overlap / mismatch in tm
          if (first->tm_end >= second->tm_start) {
            continue;
          }

          // compute cost
          int min_cost = 0;
          int max_cost = 0;

          // initial
          min_cost += abs( first->input_start - first->tm_start );
          max_cost += max( first->input_start, first->tm_start );

          // same number of words, but not sent. start -> cost is at least 1
          if (first->input_start == first->tm_start && first->input_start > 0) {
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
               && input_length-1 != second->input_end ) {
            min_cost++;
          }

          // cerr << "\tcost: " << min_cost << "-" << max_cost << endl;

          // if worst than best cost, forget it
          if (min_cost > best_cost) {
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
          if (max_cost < this_best_cost) {
            // cerr << "\tupdating this best cost to " << max_cost << "\n";
            this_best_cost = max_cost;

            // possibly updating best_cost
            if (max_cost < best_cost) {
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


void FuzzyMatchWrapper::create_extract(int sentenceInd, int cost, const vector< WORD_ID > &sourceSentence, const vector<SentenceAlignment> &targets, const string &inputStr, const string  &path, ofstream &outputFile)
{
  string sourceStr;
  for (size_t pos = 0; pos < sourceSentence.size(); ++pos) {
    WORD_ID wordId = sourceSentence[pos];
    sourceStr += GetVocabulary().GetWord(wordId) + " ";
  }

  for (size_t targetInd = 0; targetInd < targets.size(); ++targetInd) {
    const SentenceAlignment &sentenceAlignment = targets[targetInd];
    string targetStr = sentenceAlignment.getTargetString(GetVocabulary());
    string alignStr = sentenceAlignment.getAlignmentString();

    outputFile
        << sentenceInd << endl
        << cost << endl
        << sourceStr << endl
        << inputStr << endl
        << targetStr << endl
        << alignStr << endl
        << path << endl
        << sentenceAlignment.count << endl;

  }
}

} // namespace
