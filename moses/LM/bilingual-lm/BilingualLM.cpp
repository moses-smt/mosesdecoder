#include <vector>
#include "BilingualLM.h"
#include "moses/ScoreComponentCollection.h"

using namespace std;

namespace Moses
{
int BilingualLMState::Compare(const FFState& other) const
{
  const BilingualLMState &otherState = static_cast<const BilingualLMState&>(other);

  if (m_hash == otherState.m_hash)
    return 0;
  return (m_hash < otherState.m_hash) ? -1 : +1;
}

////////////////////////////////////////////////////////////////
BilingualLM::BilingualLM(const std::string &line)
  :StatefulFeatureFunction(1, line)
  ,premultiply(true)
  ,factored(false)
  ,neuralLM_cache(1000000)
  ,word_factortype(0)
  ,BOS_word(BOS_word_actual)
  ,EOS_word(EOS_word_actual)
{
  ReadParameters();
  FactorCollection& factorFactory = FactorCollection::Instance(); //Factor Factory to use for BOS_ and EOS_
  BOS_factor = factorFactory.AddFactor(BOS_);
  BOS_word_actual.SetFactor(0, BOS_factor);
  EOS_factor = factorFactory.AddFactor(EOS_);
  EOS_word_actual.SetFactor(0, EOS_factor);
}

void BilingualLM::Load(){
  loadModel();
  initSharedPointer();

  //Get unknown word ID
  unknown_word_id = LookUpNeuralLMWord("<unk>");

}

//Cache for NeuralLMids
int BilingualLM::getNeuralLMId(const Word& word) const{

  initSharedPointer();
  
  const Factor* factor = word.GetFactor(word_factortype);

  std::map<const Factor *, int>::iterator it;

  boost::upgrade_lock< boost::shared_mutex > read_lock(neuralLMids_lock);
  it = neuralLMids.find(factor);

  if (it != neuralLMids.end()) {
    if (!factored){
      return it->second; //Lock is released here automatically
    } else {
      //See if word is unknown
      if (it->second == unknown_word_id){
        const Factor* pos_factor = word.GetFactor(pos_factortype); //Get POS tag
        //Look up the POS tag in the cache
        it = neuralLMids.find(pos_factor);
        if (it != neuralLMids.end()){
          return it->second; //We have our pos tag in the cache.
        } else {
          //We have to lookup the pos_tag
          const std::string posstring = pos_factor->GetString().as_string();
          int neuralLM_wordID = LookUpNeuralLMWord(posstring);

          boost::upgrade_to_unique_lock< boost::shared_mutex > uniqueLock(read_lock);
          neuralLMids.insert(std::pair<const Factor *, int>(pos_factor, neuralLM_wordID));

          return neuralLM_wordID; //We return the ID of the pos TAG
        }
      } else {
        return it->second; //We return the neuralLMid of the word
      }
      
    }
    
  } else {
    //We have to lookup the word
    const std::string string = factor->GetString().as_string();
    int neuralLM_wordID = LookUpNeuralLMWord(string);

    boost::upgrade_to_unique_lock< boost::shared_mutex > uniqueLock(read_lock);
    neuralLMids.insert(std::pair<const Factor *, int>(factor, neuralLM_wordID));

    if (!factored) {
      return neuralLM_wordID; //Lock is released here
    } else {
      if (neuralLM_wordID == unknown_word_id){
        const Factor* pos_factor = word.GetFactor(pos_factortype);
        const std::string factorstring = pos_factor->GetString().as_string();
        neuralLM_wordID = LookUpNeuralLMWord(string);
        neuralLMids.insert(std::pair<const Factor *, int>(pos_factor, neuralLM_wordID));
      }
      return neuralLM_wordID; //If a POS tag is needed, neuralLM_wordID is going to be updated.
    }
    
  }


}

//Populates words with amount words from the targetPhrase from the previous hypothesis where
//words[0] is the last word of the previous hypothesis, words[1] is the second last etc...
void BilingualLM::requestPrevTargetNgrams(const Hypothesis &cur_hypo, int amount, std::vector<int> &words) const {
  const Hypothesis * prev_hyp = cur_hypo.GetPrevHypo();
  int found = 0;

  while (found != amount){
    if (prev_hyp){
      const TargetPhrase& currTargetPhrase = prev_hyp->GetCurrTargetPhrase();
      for (int i = currTargetPhrase.GetSize() - 1; i> -1; i--){
        if (found != amount){
          const Word& word = currTargetPhrase.GetWord(i);
          words[found] = getNeuralLMId(word);
          found++;
        } else {
          return; //We have gotten everything needed
        }
      }
    } else {
      break; //We have reached the beginning of the hypothesis
    }
    prev_hyp = prev_hyp->GetPrevHypo();
  }

  int neuralLM_wordID = getNeuralLMId(BOS_word);
  for (int i = found; i < amount; i++){
    words[i] = neuralLM_wordID;
  }

}

//Populates the words vector with target_ngrams sized that also contains the current word we are looking at. 
//(in effect target_ngrams + 1)
void BilingualLM::getTargetWords(const Hypothesis &cur_hypo
                , const TargetPhrase &targetPhrase
                , int current_word_index
                , std::vector<int> &words) const {

  //Check if we need to look at previous target phrases
  int additional_needed = current_word_index - target_ngrams;
  if (additional_needed < 0) {
    additional_needed = -additional_needed;
    std::vector<int> prev_words(additional_needed);
    requestPrevTargetNgrams(cur_hypo, additional_needed, prev_words);
    for (int i=additional_needed -1 ; i>-1; i--){
      words.push_back(prev_words[i]);
    }
  }

  if (words.size()!=source_ngrams){
    //We have added some words from previous phrases
    //Just add until we reach current_word_index
    for (int i = 0; i<current_word_index + 1; i++){
      const Word& word = targetPhrase.GetWord(i);
      words.push_back(getNeuralLMId(word));
    }

  } else {
    //We haven't added any words, proceed as before
    for (int i = current_word_index - target_ngrams; i < current_word_index + 1; i++){
      const Word& word = targetPhrase.GetWord(i);
      words.push_back(getNeuralLMId(word));
    }
  }

}

//Returns target_ngrams sized word vector that contains the current word we are looking at. (in effect target_ngrams + 1)
/*
void BilingualLM::getTargetWords(Phrase &whole_phrase
                , int current_word_index
                , std::vector<int> &words) const {

  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
  }
  int j = source_ngrams; //Initial index for appending to the vector
  for (int i = target_ngrams; i > -1; i--){

    int neuralLM_wordID;
    int indexToRead = current_word_index - i;
    //std::cout << "Whole phrase size: " << whole_phrase.GetSize() << std::endl;

    if (indexToRead < 0) {
      neuralLM_wordID = getNeuralLMId(BOS_factor);
    } else {
      const Word& word = whole_phrase.GetWord(indexToRead);
      const Factor* factor = word.GetFactor(0); //Parameter here is m_factorType, hard coded to 0
      neuralLM_wordID = getNeuralLMId(factor);
    }
    words[j] = neuralLM_wordID;
    j++;
  }

}
*/
//Returns source words in the way NeuralLM expects them.

void BilingualLM::getSourceWords(const TargetPhrase &targetPhrase
                , int targetWordIdx
                , const Sentence &source_sent
                , const WordsRange &sourceWordRange
                , std::vector<int> &words) const {
  //Get source context

  //Get alignment for the word we require
  const AlignmentInfo& alignments = targetPhrase.GetAlignTerm();

  //We are getting word alignment for targetPhrase.GetWord(i + target_ngrams -1) according to the paper.
  //Try to get some alignment, because the word we desire might be unaligned.
  std::set<size_t> last_word_al;
  for (int j = 0; j < targetPhrase.GetSize(); j++){
    //Sometimes our word will not be aligned, so find the nearest aligned word right
    if ((targetWordIdx + j) < targetPhrase.GetSize()){
      last_word_al = alignments.GetAlignmentsForTarget(targetWordIdx + j);
      if (!last_word_al.empty()){
        break;
      }
    } else if ((targetWordIdx - j) > 0) {
      //We couldn't find word on the right, try the left.
      last_word_al = alignments.GetAlignmentsForTarget(targetWordIdx - j);
      if (!last_word_al.empty()){
        break;
      }

    }
    
  }

  //Assume we have gotten some alignment here. If we couldn't get an alignment from the above routine it means
  //that none of the words in the target phrase aligned to any word in the source phrase

  //Now we get the source words.
  size_t source_center_index;
  if (last_word_al.size() == 1) {
    //We have only one word aligned
    source_center_index = *last_word_al.begin();
  } else { //We have more than one alignments, take the middle one
    int tempidx = 0; //Temporary index to track where the iterator is.
    for (std::set<size_t>::iterator it = last_word_al.begin(); it != last_word_al.end(); it++){
      if (tempidx == last_word_al.size()/2){
        source_center_index = *(it);
        break;
      }
    }
  }

  //We have found the alignment. Now determine how much to shift by to get the actual source word index.
  size_t phrase_start_pos = sourceWordRange.GetStartPos();
  size_t source_word_mid_idx = phrase_start_pos + targetWordIdx; //Account for how far the current word is from the start of the phrase.

  
  appendSourceWordsToVector(source_sent, words, source_word_mid_idx);

}

size_t BilingualLM::getState(const Hypothesis& cur_hypo) const {

  const TargetPhrase &targetPhrase = cur_hypo.GetCurrTargetPhrase();

  size_t hashCode = 0;

  //Check if we need to look at previous target phrases
  int additional_needed = targetPhrase.GetSize() - target_ngrams;
  if (additional_needed < 0) {
    additional_needed = -additional_needed;
    std::vector<int> prev_words(additional_needed);
    requestPrevTargetNgrams(cur_hypo, additional_needed, prev_words);
    for (int i=additional_needed - 1; i>-1; i--) {
      boost::hash_combine(hashCode, prev_words[i]);
    }
    //Get the rest of the phrases needed
    for (int i = 0; i < targetPhrase.GetSize(); i++) {
      int neuralLM_wordID;

      const Word& word = targetPhrase.GetWord(i);
      neuralLM_wordID = getNeuralLMId(word);

      boost::hash_combine(hashCode, neuralLM_wordID);
    }

  } else {
    for (int i = targetPhrase.GetSize() - target_ngrams; i < targetPhrase.GetSize(); i++) {
      int neuralLM_wordID;

      const Word& word = targetPhrase.GetWord(i);
      neuralLM_wordID = getNeuralLMId(word);

      boost::hash_combine(hashCode, neuralLM_wordID);
    }
  }

  return hashCode;
}
/*
size_t BilingualLM::getState(Phrase &whole_phrase) const {
  
  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
  }

  //Get last n-1 target
  size_t hashCode = 0;
  for (int i = whole_phrase.GetSize() - target_ngrams; i < whole_phrase.GetSize(); i++) {
    int neuralLM_wordID;
    if (i < 0) {
      neuralLM_wordID = getNeuralLMId(BOS_factor);
    } else {
      const Word& word = whole_phrase.GetWord(i);
      const Factor* factor = word.GetFactor(0); //Parameter here is m_factorType, hard coded to 0
      neuralLM_wordID = getNeuralLMId(factor);
    }
    boost::hash_combine(hashCode, neuralLM_wordID);
  }
  return hashCode;
}
*/
void BilingualLM::EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const {}

void BilingualLM::EvaluateWithSourceContext(const InputType &input
                                  , const InputPath &inputPath
                                  , const TargetPhrase &targetPhrase
                                  , const StackVec *stackVec
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection *estimatedFutureScore) const
{
  /*
  float value = 0;
  if (target_ngrams > targetPhrase.GetSize()) {
      //We have too small of a phrase.
      return;
  }
  for (size_t i = 0; i < targetPhrase.GetSize() - target_ngrams; ++i) {
    //Get source word indexes

    //Insert n target phrase words.
    std::vector<int> words(target_ngrams + source_ngrams);

    //Taken from NeuralLM wrapper more or less
    for (int j = target_ngrams - 1; j < -1; j--){
      const Word& word = targetPhrase.GetWord(i + j); //Target phrase is actually Phrase
      const Factor* factor = word.GetFactor(0); //Parameter here is m_factorType, hard coded to 0
      const std::string string = factor->GetString().as_string();
      int neuralLM_wordID = m_neuralLM->lookup_word(string);
      words.push_back(neuralLM_wordID); //In the paper it seems to be in reverse order
    }
    //Get source context

    //Get alignment for the word we require
    const AlignmentInfo& alignments = targetPhrase.GetAlignTerm();

    //We are getting word alignment for targetPhrase.GetWord(i + target_ngrams -1) according to the paper.
    //Try to get some alignment, because the word we desire might be unaligned.
    std::set<size_t> last_word_al;
    for (int j = 0; j < targetPhrase.GetSize(); j++){
      //Sometimes our word will not be aligned, so find the nearest aligned word right
      if ((i + target_ngrams -1 +j) < targetPhrase.GetSize()){
        last_word_al = alignments.GetAlignmentsForTarget(i + target_ngrams -1 + j);
        if (!last_word_al.empty()){
          break;
        }
      } else if ((i + target_ngrams -1 +j) > 0) {
        //We couldn't find word on the right, try the left.
        last_word_al = alignments.GetAlignmentsForTarget(i + target_ngrams -1 -j);
        if (!last_word_al.empty()){
          break;
        }

      }
      
    }

    //Assume we have gotten some alignment here. Now we get the source words.
    size_t source_center_index;
    if (last_word_al.size() == 1) {
      //We have only one word aligned
      source_center_index = *last_word_al.begin();
    } else { //We have more than one alignments, take the middle one
      int tempidx = 0; //Temporary index to track where the iterator is.
      for (std::set<size_t>::iterator it = last_word_al.begin(); it != last_word_al.end(); it++){
        if (tempidx == last_word_al.size()/2){
          source_center_index = *(it);
          break;
        }
      }
    }

    //We have found the alignment. Now determine how much to shift by to get the actual source word index.
    const WordsRange& wordsRange = inputPath.GetWordsRange();
    size_t phrase_start_pos = wordsRange.GetStartPos();
    size_t source_word_mid_idx = phrase_start_pos + i + target_ngrams -1; //Account for how far the current word is from the start of the phrase.

    const Sentence& source_sent = static_cast<const Sentence&>(input);
    
    //Define begin and end indexes of the lookup. Cases for even and odd ngrams
    int begin_idx;
    int end_idx;
    if (source_ngrams%2 == 0){
      int begin_idx = source_word_mid_idx - source_ngrams/2 - 1;
      int end_idx = source_word_mid_idx + source_ngrams/2;
    } else {
      int begin_idx = source_word_mid_idx - (source_ngrams - 1)/2;
      int end_idx = source_word_mid_idx + (source_ngrams - 1)/2;
    }

    //Add words to vector
    for (int j = begin_idx; j < end_idx; j++) {
      int neuralLM_wordID;
      if (j < 0) {
        neuralLM_wordID = m_neuralLM->lookup_word(BOS_);
      } else if (j > source_sent.GetSize()) {
        neuralLM_wordID = m_neuralLM->lookup_word(EOS_);
      } else {
        const Word& word = source_sent.GetWord(j);
        const Factor* factor = word.GetFactor(0); //Parameter here is m_factorType, hard coded to 0
        const std::string string = factor->GetString().as_string();
        neuralLM_wordID = m_neuralLM->lookup_word(string);
      }
      words.push_back(neuralLM_wordID);
      
    }

    value += m_neuralLM->lookup_ngram(words);
  }
  scoreBreakdown.PlusEquals(FloorScore(value)); //If the ngrams are > than the target phrase the value added will be zero.
*/
}


FFState* BilingualLM::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{

  Manager& manager = cur_hypo.GetManager();
  const Sentence& source_sent = static_cast<const Sentence&>(manager.GetSource());



  //Init vectors
  std::vector<int> source_words;
  source_words.reserve(source_ngrams);
  std::vector<int> target_words;
  target_words.reserve(target_ngrams);

  float value = 0;

  const TargetPhrase& currTargetPhrase = cur_hypo.GetCurrTargetPhrase();
  const WordsRange& sourceWordRange = cur_hypo.GetCurrSourceWordsRange(); //Source words range to calculate offsets

  //For each word in the current target phrase get its LM score
  for (int i = 0; i < currTargetPhrase.GetSize(); i++){
    //std::cout << "Size of Before Words " << all_words.size() << std::endl;
    getSourceWords(currTargetPhrase
              , i //The current target phrase
              , source_sent
              , sourceWordRange
              , source_words);

    getTargetWords(cur_hypo
              , currTargetPhrase
              , i
              , target_words);

    value += Score(source_words, target_words);

    //Clear the vector
    source_words.clear();
    target_words.clear();

  }

  size_t new_state = getState(cur_hypo); 
  accumulator->PlusEquals(this, value);

  return new BilingualLMState(new_state);
}
/*
FFState* BilingualLM::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{

  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
  }


  float totalScore = 0;
  Manager& manager = cur_hypo.GetManager();
  const Sentence& source_sent = static_cast<const Sentence&>(manager.GetSource());

  const Hypothesis * current = &cur_hypo;

  //std::cout << "Got to here!" << std::endl;
  while (current){
    //std::cout << "Started iterating!" << std::endl;
    std::vector<int> all_words(source_ngrams + target_ngrams + 1);
    std::vector<std::string> all_strings(source_ngrams + target_ngrams + 1);
    float value = 0;
    Phrase whole_phrase;
    current->GetOutputPhrase(whole_phrase);
    const TargetPhrase& currTargetPhrase = current->GetCurrTargetPhrase();
    const WordsRange& targetWordRange = current->GetCurrTargetWordsRange(); //This should be which words of whole_phrase the current hypothesis represents.
    int phrase_start_pos = targetWordRange.GetStartPos(); //Start position of the current target phrase

    const WordsRange& sourceWordRange = current->GetCurrSourceWordsRange(); //Source words range to calculate offsets

    //For each word in the current target phrase get its LM score
    for (int i = 0; i < currTargetPhrase.GetSize(); i++){
      getSourceWords(currTargetPhrase
                , i //The current target word
                , source_sent
                , sourceWordRange
                , all_words
                , all_strings);
      //std::cout << "Got a source Phrase" << std::endl;
      //for (int j = 0; j< all_words.size(); j++){
        //std::cout<< "All words source number " << j << " value " << all_words[j] << std::endl;
      //}
      //std::cout << "Phrase start pos " << phrase_start_pos << std::endl;
      getTargetWords(whole_phrase, (i + phrase_start_pos), all_words, all_strings);
      
      //for (int j = 0; j< all_words.size(); j++){
      //  std::cout<< all_words[j] << " ";
      //}
      //std::cout << std::endl;
      //for (int j = 0; j< all_strings.size(); j++){
      //  std::cout<< all_strings[j] << " ";
      //}
      //std::cout << std::endl;

      //std::cout << "Got a target Phrase" << std::endl;
      value += m_neuralLM->lookup_ngram(all_words);
      //value += 10;

    }

    totalScore += value;
    current = current->GetPrevHypo();
  }

  //Get state:
  Phrase whole_phrase;
  cur_hypo.GetOutputPhrase(whole_phrase);
  size_t new_state = getState(whole_phrase); 

  float old_score = 0;
  if (cur_hypo.GetPrevHypo()){
    const ScoreComponentCollection& scoreBreakdown = cur_hypo.GetPrevHypo()->GetScoreBreakdown();
    old_score = scoreBreakdown.GetScoreForProducer(this);
  }
  
  //std::cout << "Old score: " << old_score << " Total score: " << totalScore << " ";
  accumulator->Assign(this, totalScore - old_score);

  //old_score = accumulator->GetScoreForProducer(this);
  //std::cout << "Old score: " << old_score << " Total score: " << totalScore << " ";
  //std::cout << whole_phrase << std::endl;

  // int targetLen = cur_hypo.GetCurrTargetPhrase().GetSize(); // ??? [UG]
  return new BilingualLMState(new_state);
}
*/

size_t BilingualLM::getStateChart(Phrase& whole_phrase) const {
  size_t hashCode = 0;
  for (int i = whole_phrase.GetSize() - target_ngrams; i < whole_phrase.GetSize(); i++){
    int neuralLM_wordID;
    if (i < 0) {
      neuralLM_wordID = getNeuralLMId(BOS_word);
    } else {
      neuralLM_wordID = getNeuralLMId(whole_phrase.GetWord(i));
    }
    boost::hash_combine(hashCode, neuralLM_wordID);
  }
  return hashCode;
}

void BilingualLM::getTargetWordsChart(Phrase& whole_phrase
                , int current_word_index
                , std::vector<int> &words) const {

  for (int i = current_word_index - target_ngrams; i < current_word_index + 1; i++){
    if (i < 0) {
      words.push_back(getNeuralLMId(BOS_word));
    } else {
      const Word& word = whole_phrase.GetWord(i);
      words.push_back(getNeuralLMId(word));
    }
  }

}

void BilingualLM::appendSourceWordsToVector(const Sentence &source_sent, std::vector<int> &words, int source_word_mid_idx) const {
  //Define begin and end indexes of the lookup. Cases for even and odd ngrams
  //This can result in indexes which span larger than the length of the source phrase.
  //In this case we just
  int begin_idx;
  int end_idx;

  if (source_ngrams%2 == 0){
    begin_idx = source_word_mid_idx - source_ngrams/2 - 1;
    end_idx = source_word_mid_idx + source_ngrams/2;
  } else {
    begin_idx = source_word_mid_idx - (source_ngrams - 1)/2;
    end_idx = source_word_mid_idx + (source_ngrams - 1)/2;
  }

  //Add words to vector
  for (int j = begin_idx; j < end_idx + 1; j++) {
    int neuralLM_wordID;
    if (j < 0) {
      neuralLM_wordID = getNeuralLMId(BOS_word);
    } else if (j > source_sent.GetSize() - 1) {
      neuralLM_wordID = getNeuralLMId(EOS_word);
    } else {
      const Word& word = source_sent.GetWord(j);
      neuralLM_wordID = getNeuralLMId(word);
    }
    words.push_back(neuralLM_wordID);
  }
}

int BilingualLM::getSourceWordsChart(const TargetPhrase &targetPhrase
                , const ChartHypothesis& curr_hypothesis
                , int targetWordIdx
                , const Sentence &source_sent
                , size_t source_phrase_start_pos
                , int next_nonterminal_index
                , int featureID
                , std::vector<int> &words) const {

  //Get source context

  //Get alignment for the word we require
  const AlignmentInfo& alignments = targetPhrase.GetAlignTerm();

  //If we have to go back in previous hypothesis to find an aligned word, we would know its absolute
  //position in the source so we can skip most of the computations
  bool resolvedIndexis = false;
  size_t source_word_mid_idx; //This is the source word we are interested in

  //We are getting word alignment for targetPhrase.GetWord(i + target_ngrams -1) according to the paper.
  //Try to get some alignment, because the word we desire might be unaligned.
  std::set<size_t> last_word_al;
  for (int j = 0; j < targetPhrase.GetSize(); j++){
    //Sometimes our word will not be aligned, so find the nearest aligned word right
    if ((targetWordIdx + j) < targetPhrase.GetSize()){
      last_word_al = alignments.GetAlignmentsForTarget(targetWordIdx + j);
      //If the current word is non terminal we get the alignment from the previous state.
      if (targetPhrase.GetWord(targetWordIdx + j).IsNonTerminal()) {
        const ChartHypothesis * prev_hypo = curr_hypothesis.GetPrevHypo(next_nonterminal_index);
        const BilingualLMState * prev_state = static_cast<const BilingualLMState *>(prev_hypo->GetFFState(featureID));
        const std::vector<int>& word_alignments = prev_state->GetWordAlignmentVector();
        source_word_mid_idx = word_alignments[0]; //The first word on the right or left of our word
        resolvedIndexis = true;
        break;
      }
      if (!last_word_al.empty()){
        break;
      }
    } else if ((targetWordIdx - j) > 0) {
      //If the current word is non terminal we get the alignment from a different place
      if (targetPhrase.GetWord(targetWordIdx - j).IsNonTerminal()) {
        const ChartHypothesis * prev_hypo = curr_hypothesis.GetPrevHypo(next_nonterminal_index -1); //We need to look at the nonterm on the left.
        const BilingualLMState * prev_state = static_cast<const BilingualLMState *>(prev_hypo->GetFFState(featureID));
        const std::vector<int>& word_alignments = prev_state->GetWordAlignmentVector();
        source_word_mid_idx = word_alignments[0]; //The first word on the right or left of our word
        resolvedIndexis = true;
        break;
      }
      //We couldn't find word on the right, try the left.
      last_word_al = alignments.GetAlignmentsForTarget(targetWordIdx - j);
      if (!last_word_al.empty()){
        break;
      }

    }
    
  }

  //Assume we have gotten some alignment here. If we couldn't get an alignment from the above routine it means
  //that none of the words in the target phrase aligned to any word in the source phrase

  //Now we get the source words. We only need to do this if we haven't resolved the indexis before
  if (!resolvedIndexis) {
    size_t source_center_index;
    if (last_word_al.size() == 1) {
      //We have only one word aligned
      source_center_index = *last_word_al.begin();
    } else { //We have more than one alignments, take the middle one
      int tempidx = 0; //Temporary index to track where the iterator is.
      for (std::set<size_t>::iterator it = last_word_al.begin(); it != last_word_al.end(); it++){
        if (tempidx == last_word_al.size()/2){
          source_center_index = *(it);
          break;
        }
      }
    }

    //We have found the alignment. Now determine how much to shift by to get the actual source word index.
    source_word_mid_idx = source_phrase_start_pos + targetWordIdx; //Account for how far the current word is from the start of the phrase.  
  }
  
  appendSourceWordsToVector(source_sent, words, source_word_mid_idx); //Append to the vector
  
  return source_word_mid_idx;

}


FFState* BilingualLM::EvaluateWhenApplied(
  const ChartHypothesis& cur_hypo,
  int featureID, /* - used to index the state in the previous hypotheses */
  ScoreComponentCollection* accumulator) const
{
  //Init vectors
  std::vector<int> source_words;
  source_words.reserve(source_ngrams);
  std::vector<int> target_words;
  target_words.reserve(target_ngrams);

  float value = 0; //NeuralLM score

  Phrase whole_phrase;
  cur_hypo.GetOutputPhrase(whole_phrase);
  int next_phrase_start_idx = whole_phrase.GetSize(); //The start of the next hypothesis is 1+last idx of the current hypothesis

  const ChartManager& manager = cur_hypo.GetManager();
  const Sentence& source_sent = static_cast<const Sentence&>(manager.GetSource());

  int source_phrase_start_idx = cur_hypo.GetCurrSourceRange().GetStartPos();

  const TargetPhrase& currTargetPhrase = cur_hypo.GetCurrTargetPhrase();
  std::vector<int> word_alignments; //Word alignments of all words (+ those from previous hypothesis) included in this target phrase
  int word_alignments_curr_idx = 0;
  int next_nonterminal_index = 0;
  int additional_shift = 0; //Additional shift in case we have encountered non terminals

  for (int i = 0; i<currTargetPhrase.GetSize(); i++) { //This loop should be bigger as non terminals expand
    //Get Source phrases first
    if (!currTargetPhrase.GetWord(i).IsNonTerminal()){
      int source_word_al = getSourceWordsChart(currTargetPhrase
                , cur_hypo
                , i
                , source_sent
                , source_phrase_start_idx
                , next_nonterminal_index
                , featureID
                , source_words);
      getTargetWordsChart(whole_phrase, i + additional_shift, target_words);
      word_alignments.push_back(source_word_al);
      word_alignments_curr_idx++;

      value += Score(source_words, target_words);; //Get the score
    } else {
      //We have a non terminal. We have already resolved it, Use the state to gether the useful information
      const ChartHypothesis * prev_hypo = cur_hypo.GetPrevHypo(next_nonterminal_index);
      next_nonterminal_index++;

      const BilingualLMState * prev_state = static_cast<const BilingualLMState *>(prev_hypo->GetFFState(featureID));
      const std::vector<int>& prev_word_alignments = prev_state->GetWordAlignmentVector();
      //Get the previous whole phrase
      Phrase prev_whole_phrase;
      prev_hypo->GetOutputPhrase(prev_whole_phrase);
      //Iterate over all words from the non terminal
      for (int j = 0; j<prev_whole_phrase.GetSize(); j++){
        word_alignments.push_back(prev_word_alignments[j]);
        word_alignments_curr_idx++;

        appendSourceWordsToVector(source_sent, source_words, prev_word_alignments[j]); //Get Source words
        getTargetWordsChart(whole_phrase, i + additional_shift + j, target_words);
        value += Score(source_words, target_words); //Get the score
      }
      additional_shift += prev_whole_phrase.GetSize() - 1; //Take into account the size of this non Terminal

    }

    //Clear the vectors before the next iteration
    source_words.clear();
    target_words.clear();

  }
  size_t new_state = getStateChart(whole_phrase);
  int source_phrase_end_idx = word_alignments[word_alignments_curr_idx]; //Figure it out later

  accumulator->Assign(this, value);

  return new BilingualLMState(new_state, source_phrase_end_idx, word_alignments);
}

void BilingualLM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "filepath") {
    m_filePath = value;
  } else if (key == "ngrams") {
    m_nGramOrder = atoi(value.c_str());
  } else if (key == "target_ngrams") {
    target_ngrams = atoi(value.c_str());
  } else if (key == "source_ngrams") {
    source_ngrams = atoi(value.c_str());
  } else if (key == "cache_size") {
    neuralLM_cache = atoi(value.c_str());
  } else if (key == "premultiply") {
    std::string truestr = "true";
    std::string falsestr = "false";
    if (value == truestr) {
      premultiply = true;
    } else if (value == falsestr) {
        premultiply = false;
    } else {
      std::cerr << "UNRECOGNIZED OPTION FOR PARAMETER premultiply. Got " << value << " , expected true or false!" << std::endl;
      exit(1);
    }
  } else if (key == "factored") {
    std::string truestr = "true";
    std::string falsestr = "false";
    if (value == truestr) {
      factored = true;
    } else if (value == falsestr) {
        factored = false;
    } else {
      std::cerr << "UNRECOGNIZED OPTION FOR PARAMETER factored. Got " << value << " , expected true or false!" << std::endl;
      exit(1);
    }
  } else if (key == "pos_factor") {
    pos_factortype = (size_t)atoi(value.c_str());
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}

