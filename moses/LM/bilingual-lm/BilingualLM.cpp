#include <vector>
#include "BilingualLM.h"
#include "moses/ScoreComponentCollection.h"
#include "neuralLM.h"


using namespace std;

namespace Moses
{
int BilingualLMState::Compare(const FFState& other) const
{
  const BilingualLMState &otherState = static_cast<const BilingualLMState&>(other);

  if (m_targetLen == otherState.m_targetLen)
    return 0;
  return (m_targetLen < otherState.m_targetLen) ? -1 : +1;
}

////////////////////////////////////////////////////////////////
BilingualLM::BilingualLM(const std::string &line)
  :StatefulFeatureFunction(1, line)
{
  ReadParameters();
}

void BilingualLM::Load(){
  m_neuralLM_shared = new nplm::neuralLM(m_filePath, true);
  //TODO: config option?
  m_neuralLM_shared->set_cache(1000000);
  UTIL_THROW_IF2(m_nGramOrder != m_neuralLM_shared->get_order(),
                 "Wrong order of neuralLM: LM has " << m_neuralLM_shared->get_order() << ", but Moses expects " << m_nGramOrder);
}

//Returns target_ngrams sized word vector that contains the current word we are looking at. (in effect target_ngrams + 1)
void BilingualLM::getTargetWords(Phrase &whole_phrase
                , int current_word_index
                , std::vector<int> &words
                , std::vector<std::string> &strings) const {

  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
  }
  int j = source_ngrams; //Initial index for appending to the vector
  for (int i = target_ngrams; i > -1; i--){

    int neuralLM_wordID;
    int indexToRead = current_word_index - i;
    //std::cout << "Whole phrase size: " << whole_phrase.GetSize() << std::endl;

    if (indexToRead < 0) {
      //std::cout << "Index < 0 before NPLM " << indexToRead << std::endl;
      neuralLM_wordID = m_neuralLM->lookup_word(BOS_);
      strings[j] = BOS_; //For debugging purposes
      //std::cout << "Index < 0, After NPLM " << indexToRead << std::endl;
    } else {
      const Word& word = whole_phrase.GetWord(indexToRead);
      const Factor* factor = word.GetFactor(0); //Parameter here is m_factorType, hard coded to 0
      const std::string string = factor->GetString().as_string();
      strings[j] = string; //For debugging purposes
      neuralLM_wordID = m_neuralLM->lookup_word(string);
    }
    words[j] = neuralLM_wordID;
    j++;
  }

}

//Returns source words in the way NeuralLM expects them.

void BilingualLM::getSourceWords(const TargetPhrase &targetPhrase
                , int targetWordIdx
                , const Sentence &source_sent
                , const WordsRange &sourceWordRange
                , std::vector<int> &words
                , std::vector<std::string> &strings) const {

  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
  }

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
    } else if ((targetWordIdx + j) > 0) {
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

  
  //Define begin and end indexes of the lookup. Cases for even and odd ngrams
  //This can result in indexes which span larger than the length of the source phrase.
  //In this case we just
  int begin_idx;
  int end_idx;
  //std::cout << "Source ngrams: " << source_ngrams << std::endl;
  //std::cout << "source word mid idx: " << source_word_mid_idx << std::endl;
  if (source_ngrams%2 == 0){
    begin_idx = source_word_mid_idx - source_ngrams/2 - 1;
    end_idx = source_word_mid_idx + source_ngrams/2;
  } else {
    begin_idx = source_word_mid_idx - (source_ngrams - 1)/2;
    end_idx = source_word_mid_idx + (source_ngrams - 1)/2;
  }

  //std::cout << "Begin idx:" << begin_idx << std::endl;
  //std::cout << "End idx:" << end_idx << std::endl;
  //std::cout << "Words size before source " << words.size() << std::endl;
  //Add words to vector
  int i = 0; //Counter for the offset of the words vector.
  for (int j = begin_idx; j < end_idx + 1; j++) {
    int neuralLM_wordID;
    if (j < 0) {
      neuralLM_wordID = m_neuralLM->lookup_word(BOS_);
      strings[i] = BOS_;
    } else if (j > source_sent.GetSize() - 1) {
      neuralLM_wordID = m_neuralLM->lookup_word(EOS_);
      strings[i] = EOS_;
    } else {
      const Word& word = source_sent.GetWord(j);
      const Factor* factor = word.GetFactor(0); //Parameter here is m_factorType, hard coded to 0
      const std::string string = factor->GetString().as_string();
      strings[i] = string;
      //std:cout << "Source word j: " << j << " is: " << string << std::endl;
      neuralLM_wordID = m_neuralLM->lookup_word(string);
      //std::cout << "NPLM_WORDID: " << neuralLM_wordID << std::endl;
    }
    words[i] = (neuralLM_wordID);
    i++;
  }
  //std::cout << "I max is " << i << std::endl;
  //std::cout << "Words size after source " << words.size() << std::endl;

}

size_t BilingualLM::getState(Phrase &whole_phrase) const {
  
  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
  }

  //Get last n-1 target
  size_t hashCode = 0;
  for (int i = whole_phrase.GetSize() - target_ngrams; i < whole_phrase.GetSize(); i++) {
    int neuralLM_wordID;
    const Word& word = whole_phrase.GetWord(i);
    const Factor* factor = word.GetFactor(0); //Parameter here is m_factorType, hard coded to 0
    const std::string string = factor->GetString().as_string();
    neuralLM_wordID = m_neuralLM->lookup_word(string);
    boost::hash_combine(hashCode, neuralLM_wordID);
  }
  return hashCode;
}

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

  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
  }



  Manager& manager = cur_hypo.GetManager();
  const Sentence& source_sent = static_cast<const Sentence&>(manager.GetSource());



  //std::cout << "Started iterating!" << std::endl;
  std::vector<int> all_words(source_ngrams + target_ngrams + 1);
  std::vector<std::string> all_strings(source_ngrams + target_ngrams + 1);
  float value = 0;
  Phrase whole_phrase;
  cur_hypo.GetOutputPhrase(whole_phrase);
  const TargetPhrase& currTargetPhrase = cur_hypo.GetCurrTargetPhrase();
  const WordsRange& targetWordRange = cur_hypo.GetCurrTargetWordsRange(); //This should be which words of whole_phrase the current hypothesis represents.
  int phrase_start_pos = targetWordRange.GetStartPos(); //Start position of the current target phrase

  const WordsRange& sourceWordRange = cur_hypo.GetCurrSourceWordsRange(); //Source words range to calculate offsets

  //For each word in the current target phrase get its LM score
  for (int i = 0; i < currTargetPhrase.GetSize(); i++){
    //std::cout << "Size of Before Words " << all_words.size() << std::endl;
    getSourceWords(currTargetPhrase
              , i //The current target phrase
              , source_sent
              , sourceWordRange
              , all_words
              , all_strings);
    //std::cout << "Got a source Phrase" << std::endl;
    //for (int j = 0; j< all_words.size(); j++){
    //  std::cout<< "All words " << j << " value " << all_words[j] << std::endl;
    //}
    //std::cout << "Phrase start pos " << phrase_start_pos << std::endl;
    getTargetWords(whole_phrase, (i + phrase_start_pos), all_words, all_strings);
    /*
    for (int j = 0; j< all_words.size(); j++){
      std::cout<< all_words[j] << " ";
    }
    std::cout << std::endl;
    for (int j = 0; j< all_strings.size(); j++){
      std::cout<< all_strings[j] << " ";
    }
    std::cout << std::endl;
    */
    //std::cout << "Size of After Words " << all_words.size() << std::endl;
    //std::cout << "Got a target Phrase" << std::endl;
    value += m_neuralLM->lookup_ngram(all_words);

  }

  size_t new_state = getState(whole_phrase); 
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
                , i //The current target phrase
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
      
      for (int j = 0; j< all_words.size(); j++){
        std::cout<< all_words[j] << " ";
      }
      std::cout << std::endl;
      for (int j = 0; j< all_strings.size(); j++){
        std::cout<< all_strings[j] << " ";
      }
      std::cout << std::endl;

      //std::cout << "Got a target Phrase" << std::endl;
      value += m_neuralLM->lookup_ngram(all_words);

    }

    totalScore += value;
    current = current->GetPrevHypo();
  }

  //Get state:
  Phrase whole_phrase;
  cur_hypo.GetOutputPhrase(whole_phrase);
  size_t new_state = getState(whole_phrase);


  accumulator->Assign(this, totalScore);

  // int targetLen = cur_hypo.GetCurrTargetPhrase().GetSize(); // ??? [UG]
  return new BilingualLMState(new_state);
}
*/

FFState* BilingualLM::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new BilingualLMState(0);
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
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}

