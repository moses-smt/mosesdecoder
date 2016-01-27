/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/
#include "util/exception.hh"
#include "util/tokenize.hh"
#include "util/string_stream.hh"
#include "moses/TranslationModel/PhraseDictionaryMultiModelCounts.h"

using namespace std;

template<typename T>
void OutputVec(const vector<T> &vec)
{
  for (size_t i = 0; i < vec.size(); ++i) {
    cerr << vec[i] << " " << flush;
  }
  cerr << endl;
}

namespace Moses
{

PhraseDictionaryMultiModelCounts::PhraseDictionaryMultiModelCounts(const std::string &line)
  :PhraseDictionaryMultiModel(1, line)
{
  m_mode = "instance_weighting";
  m_combineFunction = InstanceWeighting;
  cerr << "m_args=" << m_args.size() << endl;
  ReadParameters();

  UTIL_THROW_IF2(m_targetTable.size() != m_pdStr.size(),
                 "List of phrase tables and target tables must be equal");

}

void PhraseDictionaryMultiModelCounts::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "mode") {
    m_mode = value;
    if (m_mode == "instance_weighting")
      m_combineFunction = InstanceWeighting;
    else if (m_mode == "interpolate")
      m_combineFunction = LinearInterpolationFromCounts;
    else {
      util::StringStream msg;
      msg << "combination mode unknown: " << m_mode;
      throw runtime_error(msg.str());
    }
  } else if (key == "lex-e2f") {
    m_lexE2FStr = Tokenize(value, ",");
    UTIL_THROW_IF2(m_lexE2FStr.size() != m_pdStr.size(),
                   "Number of scores for lexical probability p(f|e) incorrectly specified");
  } else if (key == "lex-f2e") {
    m_lexF2EStr = Tokenize(value, ",");
    UTIL_THROW_IF2(m_lexF2EStr.size() != m_pdStr.size(),
                   "Number of scores for lexical probability p(e|f) incorrectly specified");
  } else if (key == "target-table") {
    m_targetTable = Tokenize(value, ",");
  } else {
    PhraseDictionaryMultiModel::SetParameter(key, value);
  }
}

PhraseDictionaryMultiModelCounts::~PhraseDictionaryMultiModelCounts()
{
  RemoveAllInColl(m_lexTable_e2f);
  RemoveAllInColl(m_lexTable_f2e);
}


void PhraseDictionaryMultiModelCounts::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();
  for(size_t i = 0; i < m_numModels; ++i) {

    // phrase table
    const string &ptName = m_pdStr[i];

    PhraseDictionary *pt;
    pt = FindPhraseDictionary(ptName);
    UTIL_THROW_IF2(pt == NULL,
                   "Could not find component phrase table " << ptName);
    m_pd.push_back(pt);

    // reverse
    const string &target_table = m_targetTable[i];
    pt = FindPhraseDictionary(target_table);
    UTIL_THROW_IF2(pt == NULL,
                   "Could not find component phrase table " << target_table);
    m_inverse_pd.push_back(pt);

    // lex
    string lex_e2f = m_lexE2FStr[i];
    string lex_f2e = m_lexF2EStr[i];
    lexicalTable* e2f = new lexicalTable;
    LoadLexicalTable(lex_e2f, e2f);
    lexicalTable* f2e = new lexicalTable;
    LoadLexicalTable(lex_f2e, f2e);

    m_lexTable_e2f.push_back(e2f);
    m_lexTable_f2e.push_back(f2e);

  }

}


TargetPhraseCollection::shared_ptr PhraseDictionaryMultiModelCounts::GetTargetPhraseCollectionLEGACY(const Phrase& src) const
{
  vector<vector<float> > multimodelweights;
  bool normalize;
  normalize = (m_mode == "interpolate") ? true : false;
  multimodelweights = getWeights(4,normalize);

  //source phrase frequency is shared among all phrase pairs
  vector<float> fs(m_numModels);

  map<string,multiModelCountsStats*>* allStats = new(map<string,multiModelCountsStats*>);

  CollectSufficientStats(src, fs, allStats);

  TargetPhraseCollection::shared_ptr ret
  = CreateTargetPhraseCollectionCounts(src, fs, allStats, multimodelweights);

  ret->NthElement(m_tableLimit); // sort the phrases for pruning later
  const_cast<PhraseDictionaryMultiModelCounts*>(this)->CacheForCleanup(ret);
  return ret;
}


void
PhraseDictionaryMultiModelCounts::
CollectSufficientStats(const Phrase& src, vector<float> &fs,
                       map<string,multiModelCountsStats*>* allStats) const
//fill fs and allStats with statistics from models
{
  for(size_t i = 0; i < m_numModels; ++i) {
    const PhraseDictionary &pd = *m_pd[i];

    TargetPhraseCollection::shared_ptr ret_raw
    = pd.GetTargetPhraseCollectionLEGACY(src);
    if (ret_raw != NULL) {

      TargetPhraseCollection::const_iterator iterTargetPhrase;
      for (iterTargetPhrase = ret_raw->begin(); iterTargetPhrase != ret_raw->end();  ++iterTargetPhrase) {

        const TargetPhrase * targetPhrase = *iterTargetPhrase;
        vector<float> raw_scores = targetPhrase->GetScoreBreakdown().GetScoresForProducer(&pd);

        string targetString = targetPhrase->GetStringRep(m_output);
        if (allStats->find(targetString) == allStats->end()) {

          multiModelCountsStats * statistics = new multiModelCountsStats;
          statistics->targetPhrase = new TargetPhrase(*targetPhrase); //make a copy so that we don't overwrite the original phrase table info

          //correct future cost estimates and total score
          statistics->targetPhrase->GetScoreBreakdown().InvertDenseFeatures(&pd);
          vector<FeatureFunction*> pd_feature;
          pd_feature.push_back(m_pd[i]);
          const vector<FeatureFunction*> pd_feature_const(pd_feature);
          statistics->targetPhrase->EvaluateInIsolation(src, pd_feature_const);
          // zero out scores from original phrase table
          statistics->targetPhrase->GetScoreBreakdown().ZeroDenseFeatures(&pd);

          statistics->fst.resize(m_numModels);
          statistics->ft.resize(m_numModels);

          (*allStats)[targetString] = statistics;

        }
        multiModelCountsStats * statistics = (*allStats)[targetString];

        statistics->fst[i] = UntransformScore(raw_scores[0]);
        statistics->ft[i] = UntransformScore(raw_scores[1]);
        fs[i] = UntransformScore(raw_scores[2]);
        (*allStats)[targetString] = statistics;
      }
    }
  }

  // get target phrase frequency for models which have not seen the phrase pair
  for ( map< string, multiModelCountsStats*>::const_iterator iter = allStats->begin(); iter != allStats->end(); ++iter ) {
    multiModelCountsStats * statistics = iter->second;

    for (size_t i = 0; i < m_numModels; ++i) {
      if (!statistics->ft[i]) {
        statistics->ft[i] = GetTargetCount(static_cast<const Phrase&>(*statistics->targetPhrase), i);
      }
    }
  }
}

TargetPhraseCollection::shared_ptr
PhraseDictionaryMultiModelCounts::
CreateTargetPhraseCollectionCounts(const Phrase &src, vector<float> &fs, map<string,multiModelCountsStats*>* allStats, vector<vector<float> > &multimodelweights) const
{
  TargetPhraseCollection::shared_ptr ret(new TargetPhraseCollection);
  for ( map< string, multiModelCountsStats*>::const_iterator iter = allStats->begin(); iter != allStats->end(); ++iter ) {

    multiModelCountsStats * statistics = iter->second;

    if (statistics->targetPhrase->GetAlignTerm().GetSize() == 0) {
      UTIL_THROW(util::Exception, " alignment information empty\ncount-tables need to include alignment information for computation of lexical weights.\nUse --phrase-word-alignment during training; for on-disk tables, also set -alignment-info when creating on-disk tables.");
    }

    try {
      pair<vector< set<size_t> >, vector< set<size_t> > > alignment = GetAlignmentsForLexWeights(src, static_cast<const Phrase&>(*statistics->targetPhrase), statistics->targetPhrase->GetAlignTerm());
      vector< set<size_t> > alignedToT = alignment.first;
      vector< set<size_t> > alignedToS = alignment.second;
      double lexst = ComputeWeightedLexicalTranslation(static_cast<const Phrase&>(*statistics->targetPhrase), src, alignedToS, m_lexTable_e2f, multimodelweights[1], false );
      double lexts = ComputeWeightedLexicalTranslation(src, static_cast<const Phrase&>(*statistics->targetPhrase), alignedToT, m_lexTable_f2e, multimodelweights[3], true );

      Scores scoreVector(4);
      scoreVector[0] = FloorScore(TransformScore(m_combineFunction(statistics->fst, statistics->ft, multimodelweights[0])));
      scoreVector[1] = FloorScore(TransformScore(lexst));
      scoreVector[2] = FloorScore(TransformScore(m_combineFunction(statistics->fst, fs, multimodelweights[2])));
      scoreVector[3] = FloorScore(TransformScore(lexts));

      statistics->targetPhrase->GetScoreBreakdown().Assign(this, scoreVector);

      //correct future cost estimates and total score
      vector<FeatureFunction*> pd_feature;
      pd_feature.push_back(const_cast<PhraseDictionaryMultiModelCounts*>(this));
      const vector<FeatureFunction*> pd_feature_const(pd_feature);
      statistics->targetPhrase->EvaluateInIsolation(src, pd_feature_const);
    } catch (AlignmentException& e) {
      continue;
    }

    ret->Add(new TargetPhrase(*statistics->targetPhrase));
  }

  RemoveAllInMap(*allStats);
  delete allStats;
  return ret;
}


float PhraseDictionaryMultiModelCounts::GetTargetCount(const Phrase &target, size_t modelIndex) const
{

  const PhraseDictionary &pd = *m_inverse_pd[modelIndex];
  TargetPhraseCollection::shared_ptr ret_raw = pd.GetTargetPhraseCollectionLEGACY(target);

  // in inverse mode, we want the first score of the first phrase pair (note: if we were to work with truly symmetric models, it would be the third score)
  if (ret_raw && ret_raw->GetSize() > 0) {
    const TargetPhrase * targetPhrase = *(ret_raw->begin());
    return UntransformScore(targetPhrase->GetScoreBreakdown().GetScoresForProducer(&pd)[0]);
  }

  // target phrase unknown
  else return 0;
}


pair<PhraseDictionaryMultiModelCounts::AlignVector,PhraseDictionaryMultiModelCounts::AlignVector> PhraseDictionaryMultiModelCounts::GetAlignmentsForLexWeights(const Phrase &phraseS, const Phrase &phraseT, const AlignmentInfo &alignment) const
{

  size_t tsize = phraseT.GetSize();
  size_t ssize = phraseS.GetSize();
  AlignVector alignedToT (tsize);
  AlignVector alignedToS (ssize);
  AlignmentInfo::const_iterator iter;

  for (iter = alignment.begin(); iter != alignment.end(); ++iter) {
    const pair<size_t,size_t> &alignPair = *iter;
    size_t s = alignPair.first;
    size_t t = alignPair.second;
    if (s >= ssize || t >= tsize) {
      cerr << "Error: inconsistent alignment for phrase pair: " << phraseS << " - " << phraseT << endl;
      cerr << "phrase pair will be discarded" << endl;
      throw AlignmentException();
    }
    alignedToT[t].insert( s );
    alignedToS[s].insert( t );
  }
  return make_pair(alignedToT,alignedToS);
}


double PhraseDictionaryMultiModelCounts::ComputeWeightedLexicalTranslation( const Phrase &phraseS, const Phrase &phraseT, AlignVector &alignment, const vector<lexicalTable*> &tables, vector<float> &multimodelweights, bool is_input) const
{
  // lexical translation probability

  double lexScore = 1.0;
  Word null;
  if (is_input) {
    null.CreateFromString(Input, m_input, "NULL", false);
  } else {
    null.CreateFromString(Output, m_output, "NULL", false);
  }

  // all target words have to be explained
  for(size_t ti=0; ti<alignment.size(); ti++) {
    const set< size_t > & srcIndices = alignment[ ti ];
    Word t_word = phraseT.GetWord(ti);

    if (srcIndices.empty()) {
      // explain unaligned word by NULL
      lexScore *= GetLexicalProbability( null, t_word, tables, multimodelweights );
    } else {
      // go through all the aligned words to compute average
      double thisWordScore = 0;
      for (set< size_t >::const_iterator si(srcIndices.begin()); si != srcIndices.end(); ++si) {
        Word s_word = phraseS.GetWord(*si);
        thisWordScore += GetLexicalProbability( s_word, t_word, tables, multimodelweights );
      }
      lexScore *= thisWordScore / srcIndices.size();
    }
  }
  return lexScore;
}


lexicalCache PhraseDictionaryMultiModelCounts::CacheLexicalStats( const Phrase &phraseS, const Phrase &phraseT, AlignVector &alignment, const vector<lexicalTable*> &tables, bool is_input )
{
//do all the necessary lexical table lookups and get counts, but don't apply weights yet

  Word null;
  if (is_input) {
    null.CreateFromString(Input, m_input, "NULL", false);
  } else {
    null.CreateFromString(Output, m_output, "NULL", false);
  }

  lexicalCache ret;

  // all target words have to be explained
  for(size_t ti=0; ti<alignment.size(); ti++) {
    const set< size_t > & srcIndices = alignment[ ti ];
    Word t_word = phraseT.GetWord(ti);

    vector<lexicalPair> ti_vector;
    if (srcIndices.empty()) {
      // explain unaligned word by NULL
      vector<float> joint_count (m_numModels);
      vector<float> marginals (m_numModels);

      FillLexicalCountsJoint(null, t_word, joint_count, tables);
      FillLexicalCountsMarginal(null, marginals, tables);

      ti_vector.push_back(make_pair(joint_count, marginals));

    } else {
      for (set< size_t >::const_iterator si(srcIndices.begin()); si != srcIndices.end(); ++si) {
        Word s_word = phraseS.GetWord(*si);
        vector<float> joint_count (m_numModels);
        vector<float> marginals (m_numModels);

        FillLexicalCountsJoint(s_word, t_word, joint_count, tables);
        FillLexicalCountsMarginal(s_word, marginals, tables);

        ti_vector.push_back(make_pair(joint_count, marginals));
      }
    }
    ret.push_back(ti_vector);
  }
  return ret;
}


double PhraseDictionaryMultiModelCounts::ComputeWeightedLexicalTranslationFromCache( lexicalCache &cache, vector<float> &weights ) const
{
  // lexical translation probability

  double lexScore = 1.0;

  for (lexicalCache::const_iterator iter = cache.begin();  iter != cache.end(); ++iter) {
    vector<lexicalPair> t_vector = *iter;
    double thisWordScore = 0;
    for ( vector<lexicalPair>::const_iterator iter2 = t_vector.begin();  iter2 != t_vector.end(); ++iter2) {
      vector<float> joint_count = iter2->first;
      vector<float> marginal = iter2->second;
      thisWordScore += m_combineFunction(joint_count, marginal, weights);
    }
    lexScore *= thisWordScore / t_vector.size();
  }
  return lexScore;
}

// get lexical probability for single word alignment pair
double PhraseDictionaryMultiModelCounts::GetLexicalProbability( Word &wordS, Word &wordT, const vector<lexicalTable*> &tables, vector<float> &multimodelweights ) const
{
  vector<float> joint_count (m_numModels);
  vector<float> marginals (m_numModels);

  FillLexicalCountsJoint(wordS, wordT, joint_count, tables);
  FillLexicalCountsMarginal(wordS, marginals, tables);

  double lexProb = m_combineFunction(joint_count, marginals, multimodelweights);

  return lexProb;
}


void PhraseDictionaryMultiModelCounts::FillLexicalCountsJoint(Word &wordS, Word &wordT, vector<float> &count, const vector<lexicalTable*> &tables) const
{
  for (size_t i=0; i < m_numModels; i++) {
    lexicalMapJoint::iterator joint_s = tables[i]->joint.find( wordS );
    if (joint_s == tables[i]->joint.end()) count[i] = 0.0;
    else {
      lexicalMap::iterator joint_t = joint_s->second.find( wordT );
      if (joint_t == joint_s->second.end()) count[i] = 0.0;
      else count[i] = joint_t->second;
    }
  }
}

void PhraseDictionaryMultiModelCounts::FillLexicalCountsMarginal(Word &wordS, vector<float> &count, const vector<lexicalTable*> &tables) const
{
  for (size_t i=0; i < m_numModels; i++) {
    lexicalMap::iterator marginal_s = tables[i]->marginal.find( wordS );
    if (marginal_s == tables[i]->marginal.end()) count[i] = 0.0;
    else count[i] = marginal_s->second;
  }
}


void PhraseDictionaryMultiModelCounts::LoadLexicalTable( string &fileName, lexicalTable* ltable)
{

  cerr << "Loading lexical translation table from " << fileName;
  ifstream inFile;
  inFile.open(fileName.c_str());
  if (inFile.fail()) {
    cerr << " - ERROR: could not open file\n";
    exit(1);
  }
  istream *inFileP = &inFile;

  int i=0;
  string line;

  while(getline(*inFileP, line)) {
    i++;
    if (i%100000 == 0) cerr << "." << flush;

    const vector<string> token = util::tokenize( line );
    if (token.size() != 4) {
      cerr << "line " << i << " in " << fileName
           << " has wrong number of tokens, skipping:\n"
           << token.size() << " " << token[0] << " " << line << endl;
      continue;
    }

    double joint = atof( token[2].c_str() );
    double marginal = atof( token[3].c_str() );
    Word wordT, wordS;
    wordT.CreateFromString(Output, m_output, token[0], false);
    wordS.CreateFromString(Input, m_input, token[1], false);
    ltable->joint[ wordS ][ wordT ] = joint;
    ltable->marginal[ wordS ] = marginal;
  }
  cerr << endl;

}


#ifdef WITH_DLIB
vector<float> PhraseDictionaryMultiModelCounts::MinimizePerplexity(vector<pair<string, string> > &phrase_pair_vector)
{

  map<pair<string, string>, size_t> phrase_pair_map;

  for ( vector<pair<string, string> >::const_iterator iter = phrase_pair_vector.begin(); iter != phrase_pair_vector.end(); ++iter ) {
    phrase_pair_map[*iter] += 1;
  }

  vector<multiModelCountsStatsOptimization*> optimizerStats;

  for ( map<pair<string, string>, size_t>::iterator iter = phrase_pair_map.begin(); iter != phrase_pair_map.end(); ++iter ) {

    pair<string, string> phrase_pair = iter->first;
    string source_string = phrase_pair.first;
    string target_string = phrase_pair.second;

    vector<float> fs(m_numModels);
    map<string,multiModelCountsStats*>* allStats = new(map<string,multiModelCountsStats*>);

    Phrase sourcePhrase(0);
    sourcePhrase.CreateFromString(Input, m_input, source_string, NULL);

    CollectSufficientStats(sourcePhrase, fs, allStats); //optimization potential: only call this once per source phrase

    //phrase pair not found; leave cache empty
    if (allStats->find(target_string) == allStats->end()) {
      RemoveAllInMap(*allStats);
      delete allStats;
      continue;
    }

    multiModelCountsStatsOptimization * targetStats = new multiModelCountsStatsOptimization();
    targetStats->targetPhrase = new TargetPhrase(*(*allStats)[target_string]->targetPhrase);
    targetStats->fs = fs;
    targetStats->fst = (*allStats)[target_string]->fst;
    targetStats->ft = (*allStats)[target_string]->ft;
    targetStats->f = iter->second;

    try {
      pair<vector< set<size_t> >, vector< set<size_t> > > alignment = GetAlignmentsForLexWeights(sourcePhrase, static_cast<const Phrase&>(*targetStats->targetPhrase), targetStats->targetPhrase->GetAlignTerm());
      targetStats->lexCachee2f = CacheLexicalStats(static_cast<const Phrase&>(*targetStats->targetPhrase), sourcePhrase, alignment.second, m_lexTable_e2f, false );
      targetStats->lexCachef2e = CacheLexicalStats(sourcePhrase, static_cast<const Phrase&>(*targetStats->targetPhrase), alignment.first, m_lexTable_f2e, true );

      optimizerStats.push_back(targetStats);
    } catch (AlignmentException& e) {}

    RemoveAllInMap(*allStats);
    delete allStats;
  }

  Sentence sentence;
  CleanUpAfterSentenceProcessing(sentence); // free memory used by compact phrase tables

  vector<float> ret (m_numModels*4);
  for (size_t iFeature=0; iFeature < 4; iFeature++) {

    CrossEntropyCounts * ObjectiveFunction = new CrossEntropyCounts(optimizerStats, this, iFeature);

    vector<float> weight_vector = Optimize(ObjectiveFunction, m_numModels);

    if (m_mode == "interpolate") {
      weight_vector = normalizeWeights(weight_vector);
    } else if (m_mode == "instance_weighting") {
      float first_value = weight_vector[0];
      for (size_t i=0; i < m_numModels; i++) {
        weight_vector[i] = weight_vector[i]/first_value;
      }
    }
    cerr << "Weight vector for feature " << iFeature << ": ";
    for (size_t i=0; i < m_numModels; i++) {
      ret[(iFeature*m_numModels)+i] = weight_vector[i];
      cerr << weight_vector[i] << " ";
    }
    cerr << endl;
    delete ObjectiveFunction;
  }

  RemoveAllInColl(optimizerStats);
  return ret;

}

double CrossEntropyCounts::operator() ( const dlib::matrix<double,0,1>& arg) const
{
  double total = 0.0;
  double n = 0.0;
  std::vector<float> weight_vector (m_model->m_numModels);

  for (int i=0; i < arg.nr(); i++) {
    weight_vector[i] = arg(i);
  }
  if (m_model->m_mode == "interpolate") {
    weight_vector = m_model->normalizeWeights(weight_vector);
  }

  for ( std::vector<multiModelCountsStatsOptimization*>::const_iterator iter = m_optimizerStats.begin(); iter != m_optimizerStats.end(); ++iter ) {
    multiModelCountsStatsOptimization* statistics = *iter;
    size_t f = statistics->f;

    double score;
    if (m_iFeature == 0) {
      score = m_model->m_combineFunction(statistics->fst, statistics->ft, weight_vector);
    } else if (m_iFeature == 1) {
      score = m_model->ComputeWeightedLexicalTranslationFromCache(statistics->lexCachee2f, weight_vector);
    } else if (m_iFeature == 2) {
      score = m_model->m_combineFunction(statistics->fst, statistics->fs, weight_vector);
    } else if (m_iFeature == 3) {
      score = m_model->ComputeWeightedLexicalTranslationFromCache(statistics->lexCachef2e, weight_vector);
    } else {
      score = 0;
      UTIL_THROW(util::Exception, "Trying to optimize feature that I don't know. Aborting");
    }
    total -= (FloorScore(TransformScore(score))/TransformScore(2))*f;
    n += f;
  }
  return total/n;
}

#endif

// calculate weighted probability based on instance weighting of joint counts and marginal counts
double InstanceWeighting(vector<float> &joint_counts, vector<float> &marginals, vector<float> &multimodelweights)
{

  double joint_counts_weighted =  inner_product(joint_counts.begin(), joint_counts.end(), multimodelweights.begin(), 0.0);
  double marginals_weighted = inner_product(marginals.begin(), marginals.end(), multimodelweights.begin(), 0.0);

  if (marginals_weighted == 0) {
    return 0;
  } else {
    return joint_counts_weighted/marginals_weighted;
  }
}


// calculate linear interpolation of relative frequency estimates based on joint count and marginal counts
//unused for now; enable in config?
double LinearInterpolationFromCounts(vector<float> &joint_counts, vector<float> &marginals, vector<float> &multimodelweights)
{

  vector<float> p(marginals.size());

  for (size_t i=0; i < marginals.size(); i++) {
    if (marginals[i] != 0) {
      p[i] = joint_counts[i]/marginals[i];
    }
  }

  double p_weighted = inner_product(p.begin(), p.end(), multimodelweights.begin(), 0.0);

  return p_weighted;
}

} //namespace
