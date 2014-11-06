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
		size_t sentID = 0;
		string sourceSide = "initial";
		vector<string> rightTargets;
		size_t srcTotal = 0;
		size_t tgtTotal = 0;
		size_t srcSurvived = 0;
		size_t tgtSurvived = 0;

		//One deque is one training example with multiple correct answers
		deque<PSDLine> trainingExample = m_queue->Dequeue();
		//std::cerr << "Got example from queue : " << trainingExamples.size() << std::endl;

		//iterate over the training examples and process each one
		deque<PSDLine> ::const_iterator itr_example;

		for(itr_example = trainingExample.begin(); itr_example != trainingExample.end(); itr_example++)
		{
			PSDLine current = *itr_example;

			//Process first training example
			if(sentID == 0 && sourceSide == "initial")
			{
				sentID = current.GetSentID();
				sourceSide = current.GetSrcPhrase();
				spanStart = current.GetSrcStart();
				spanEnd = current.GetSrcEnd();
				rightTargets.push_back(current.GetTgtPhrase());
			}
			else{
				//If other examples don't have the same source side, span or sentence ID something went wrong
				if(current.GetSentID() != sentID || current.GetSrcPhrase() != sourceSide || current.GetSrcStart() != spanStart || current.GetSrcEnd() != spanEnd)
				{
					std::cerr << "ERROR : Instances in same training example should have same source side information" << std::endl;
					abort();
				}
				rightTargets.push_back(current.GetTgtPhrase());
			}
		}

		//Get current sentence from corpus and parse
		corpusLine = m_corpus_input->GetLine(sentID);
		parseLine = m_parse_input->GetLine(sentID);

		//check if example is in rule table, if not we are done with this example
		if (! m_ruleTable->SrcExists(sourceSide)) {
					  //std::cout << "Source not found, continue" << std::endl;
					  continue;
					}
			// generate features
			if (hasTranslation) { //ignore first round
				//std::cerr << "EXTRACTING FEATURES (1) FOR : " << current.GetSentID() << " " << current.GetSrcPhrase() << " " << current.GetTgtPhrase() << std::endl;
				 srcSurvived++;
				 m_extractor.GenerateFeaturesChart(&m_consumer, context, sourceSide, syntFeats, parentLabel.GetString(), span, spanStart, spanEnd, translations, losses);}
				 //Fabienne Braune: Uncomment for debugging : pEgivenF is passed with losses to check numbers
				 //extractor.GenerateFeaturesChart(&consumer, context, srcPhrase, syntFeats, parentLabel.GetString(), span, spanStart, spanEnd, translations, losses, pEgivenF);}
				 // set new source phrase, context, translations and losses
				  context = ReadFactoredLine(corpusLine, m_config->GetFactors().size());
				  translations = m_ruleTable->GetTranslations(sourceSide);
				  losses.clear();
				  syntFeats.clear();
				  parentLabel.clear();
				  losses.resize(translations.size(), 1);
				  srcTotal++;

				  // after extraction, set translation to false again
				  hasTranslation = false;

				  //get span corresponding to lhs of rule
				  int spanInt = (spanEnd - spanStart) + 1;
				  int chartSpanStart = spanStart;
				  int chartSpanEnd = spanEnd;

				  CHECK(spanInt > 0);
				  stringstream s;
				  s << spanInt;
				  span = s.str();

				  // set new syntax features
				  size_t sentSize = GetSizeOfSentence(corpusLine);

				  Moses::InputTreeRep myInputChart = Moses::InputTreeRep(sentSize);
				  myInputChart.Read(parseLine);
				  vector<SyntaxLabel> lhsSyntaxLabels = myInputChart.GetLabels(chartSpanStart, chartSpanEnd);
				  //std::cerr << "Getting parent label : " << spanStart << " : " << spanEnd << std::endl;

				  bool IsBegin = false;
				  string noTag = "NOTAG";
				  parentLabel = myInputChart.GetParent(chartSpanStart,chartSpanEnd,IsBegin);
				  IsBegin = false;
				  while(!parentLabel.GetString().compare("NOTAG"))
				  {
					  parentLabel = myInputChart.GetParent(chartSpanStart,chartSpanEnd,IsBegin);
					  if( !(IsBegin ) )
					  {chartSpanStart--;}
					  else
					  {chartSpanEnd++;}
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

					bool foundTgt = false;

					//CONSIDER ALL TARGET PHRASES
					vector<size_t> targetPhraseIDs;
					vector<string> :: iterator itr_rightTargets;
					for(itr_rightTargets = rightTargets.begin();itr_rightTargets != rightTargets.end(); itr_rightTargets++)
					{
						size_t tgtPhraseID = m_ruleTable->GetTgtPhraseID(*itr_rightTargets, &foundTgt);
						targetPhraseIDs.push_back(tgtPhraseID);
					}

					//if it is not found, no example should be generated
					if (foundTgt) {
						// addadd correct translation (i.e., set its loss to 0)
						for (size_t i = 0; i < translations.size(); i++) {
							//loop through correct targetIDs
							vector<size_t> :: iterator itr_targetsIds;
							for(itr_targetsIds = targetPhraseIDs.begin(); itr_targetsIds != targetPhraseIDs.end(); itr_targetsIds++)
							{
								if (translations[i].m_index == *itr_targetsIds) {
									//std::cerr << "TRANSLATION FOUND : " << translations[i] << std::endl;
									losses[i] = 0;
									hasTranslation = true;
									tgtSurvived++;}
							}
						}
					}
					// generate features for the last source phrase
					if (hasTranslation) {
					srcSurvived++;
					//Fabienne Braune: Uncomment for debugging : pEgivenF is passed with losses to check numbers
					//extractor.GenerateFeaturesChart(&consumer, context, srcPhrase, syntFeats, parentLabel.GetString(), span, spanStart, spanEnd, translations, losses, pEgivenF);
					//std::cerr << "EXTRACTING FEATURES (2) FOR : " << current.GetSentID() << " " << current.GetSrcPhrase() << " " << current.GetTgtPhrase() << std::endl;
					m_extractor.GenerateFeaturesChart(&m_consumer, context, sourceSide, syntFeats, parentLabel.GetString(), span, spanStart, spanEnd, translations, losses);}
				  }
// Make sure we can be interrupted
boost::this_thread::interruption_point();
}
