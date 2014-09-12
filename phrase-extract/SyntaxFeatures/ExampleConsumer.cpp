#include <iostream>
#include "ExampleConsumer.h"

using namespace std;
using namespace Moses;
using namespace PSD;

ContextType ExampleConsumer::ReadFactoredLine(const string &line, size_t factorCount)
{
  ContextType out;
  vector<string> words = Tokenize(line, " ");
  vector<string>::const_iterator it;
  for (it = words.begin(); it != words.end(); it++) {
    vector<string> factors = Tokenize(*it, "|");
    if (factors.size() != factorCount) {
      cerr << "error: Wrong count of factors: " << *it << endl;
      exit(1);
    }
    out.push_back(factors);
  }
  return out;
}

size_t ExampleConsumer::GetSizeOfSentence(const string &line)
{
	vector<string> words = Tokenize(line, " ");
	return words.size();
}

ExampleConsumer::ExampleConsumer(int id, SynchronizedInput<deque<PSDLine> >* queue, RuleTable* ruleTable, ExtractorConfig* configFile, CorpusLine* corpus, CorpusLine* parse, SynchronizedInput<std::string>* writeQueue)
: m_id(id),
  m_queue(queue),
  m_ruleTable(ruleTable),
  m_consumer(VWBufferTrainConsumer(writeQueue)),
  m_corpus_input(corpus),
  m_parse_input(parse),
  m_config(configFile),
  m_extractor(FeatureExtractor(m_ruleTable->GetTargetIndexPtr(), m_config, true))
{}

void ExampleConsumer::operator () ()
{
	std::cerr << "Consumer " << m_id << " building training example" << std::endl;
	while (true)
	{
		string corpusLine;
		string parseLine;
		deque<string> targetSides;
		bool hasTranslation = false;

		ContextType context;
		vector<float> losses;
		vector<string> syntFeats;
		vector<ChartTranslation> translations;
		vector<SyntaxLabel> syntLabels;
		SyntaxLabel parentLabel("NOTAG",true);
		string span;
		size_t spanStart = 0;
		size_t spanEnd = 0;
		size_t srcTotal = 0;
		size_t tgtTotal = 0;
		size_t srcSurvived = 0;
		size_t tgtSurvived = 0;

		deque<PSDLine> trainingExamples = m_queue->Dequeue();
		//std::cerr << "Got example from queue : " << trainingExamples.size() << std::endl;

		//iterate over the training examples and process each one
		deque<PSDLine> ::const_iterator itr_examples;
		for(itr_examples = trainingExamples.begin(); itr_examples != trainingExamples.end(); itr_examples++)
		{
			PSDLine current = *itr_examples;
			//std::cerr << "CONSUMED PSD LINE : " << current.GetSentID() << " " << current.GetSrcPhrase() << " " << current.GetTgtPhrase() << std::endl;

			//Get current sentence from corpus and parse
			corpusLine = m_corpus_input->GetLine(current.GetSentID());
			parseLine = m_parse_input->GetLine(current.GetSentID());

			//check if example is in rule table, if not we are done with this example
			if (! m_ruleTable->SrcExists(current.GetSrcPhrase())) {
					  //std::cout << "Source not found, continue" << std::endl;
					  continue;
					}
			// generate features
			if (hasTranslation) { //ignore first round
				//std::cerr << "EXTRACTING FEATURES (1) FOR : " << current.GetSentID() << " " << current.GetSrcPhrase() << " " << current.GetTgtPhrase() << std::endl;
				 srcSurvived++;
				 m_extractor.GenerateFeaturesChart(&m_consumer, context, current.GetSrcPhrase(), syntFeats, parentLabel.GetString(), span, spanStart, spanEnd, translations, losses);}
				 //Fabienne Braune: Uncomment for debugging : pEgivenF is passed with losses to check numbers
				 //extractor.GenerateFeaturesChart(&consumer, context, srcPhrase, syntFeats, parentLabel.GetString(), span, spanStart, spanEnd, translations, losses, pEgivenF);}
				 // set new source phrase, context, translations and losses
				  spanStart = current.GetSrcStart();
				  spanEnd = current.GetSrcEnd();
				  context = ReadFactoredLine(corpusLine, m_config->GetFactors().size());
				  translations = m_ruleTable->GetTranslations(current.GetSrcPhrase());
				  losses.clear();
				  syntFeats.clear();
				  parentLabel.clear();
				  losses.resize(translations.size(), 1);
				  srcTotal++;

				  // after extraction, set translation to false again
				  hasTranslation = false;

				  //get span corresponding to lhs of rule
				  int spanInt = (spanEnd - spanStart) + 1;

				  CHECK(spanInt > 0);
				  stringstream s;
				  s << spanInt;
				  span = s.str();

				  // set new syntax features
				  size_t sentSize = GetSizeOfSentence(corpusLine);

				  Moses::InputTreeRep myInputChart = Moses::InputTreeRep(sentSize);
				  myInputChart.Read(parseLine);
				  vector<SyntaxLabel> lhsSyntaxLabels = myInputChart.GetLabels(spanStart, spanEnd);
				  //std::cerr << "Getting parent label : " << spanStart << " : " << spanEnd << std::endl;

				  bool IsBegin = false;
				  string noTag = "NOTAG";
				  parentLabel = myInputChart.GetParent(spanStart,spanEnd,IsBegin);
				  IsBegin = false;
				  while(!parentLabel.GetString().compare("NOTAG"))
				  {
					  parentLabel = myInputChart.GetParent(spanStart,spanEnd,IsBegin);
					  if( !(IsBegin ) )
					  {spanStart--;}
					  else
					  {spanEnd++;}
				   }

				   vector<SyntaxLabel>::iterator itr_syn_lab;
				   for(itr_syn_lab = lhsSyntaxLabels.begin(); itr_syn_lab != lhsSyntaxLabels.end(); itr_syn_lab++)
				   {
					   SyntaxLabel syntaxLabel = *itr_syn_lab;
					   CHECK(syntaxLabel.IsNonTerm() == 1);
					   string syntFeat = syntaxLabel.GetString();

					   bool toRemove = false;
					   if( (lhsSyntaxLabels.size() > 1 ) && !(syntFeat.compare( myInputChart.GetNoTag() )) )
					   {toRemove = true;}

						if(toRemove == false)
						{
							syntFeats.push_back(syntFeat);
						}
				   }

					//restore span start and span end for extraction of context and bow features
					spanStart = current.GetSrcStart();
					spanEnd = current.GetSrcEnd();

					bool foundTgt = false;

					//TODO : FIX considering all target phrases
					size_t tgtPhraseID = m_ruleTable->GetTgtPhraseID(current.GetTgtPhrase(), &foundTgt);

					//if it is not found, no example should be generated
					if (foundTgt) {
						// addadd correct translation (i.e., set its loss to 0)
						for (size_t i = 0; i < translations.size(); i++) {
							if (translations[i].m_index == tgtPhraseID) {
								//std::cerr << "ID for target found : " << tgtPhraseID << " : " << translations[i].m_index << std::endl;
								losses[i] = 0;
								hasTranslation = true;
								tgtSurvived++;
								break;}
							else
							{
								//std::cerr << "ID for target not found : " << tgtPhraseID << " : " << translations[i].m_index << std::endl;
							}
						}
					}
					// generate features for the last source phrase
					if (hasTranslation) {
					srcSurvived++;
					//Fabienne Braune: Uncomment for debugging : pEgivenF is passed with losses to check numbers
					//extractor.GenerateFeaturesChart(&consumer, context, srcPhrase, syntFeats, parentLabel.GetString(), span, spanStart, spanEnd, translations, losses, pEgivenF);
					//std::cerr << "EXTRACTING FEATURES (2) FOR : " << current.GetSentID() << " " << current.GetSrcPhrase() << " " << current.GetTgtPhrase() << std::endl;
					m_extractor.GenerateFeaturesChart(&m_consumer, context, current.GetSrcPhrase(), syntFeats, parentLabel.GetString(), span, spanStart, spanEnd, translations, losses);}
				  }
	}
// Make sure we can be interrupted
boost::this_thread::interruption_point();
}
