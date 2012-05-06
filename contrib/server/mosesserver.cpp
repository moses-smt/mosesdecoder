#include "util/check.hh"
#include <stdexcept>
#include <iostream>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

#include "Hypothesis.h"
#include "Manager.h"
#include "StaticData.h"
#include "PhraseDictionaryDynSuffixArray.h"
#include "TranslationSystem.h"
#include "LMList.h"
#ifdef LM_ORLM
#  include "LanguageModelORLM.h"
#endif
#include <boost/algorithm/string.hpp>

using namespace Moses;
using namespace std;

typedef std::map<std::string, xmlrpc_c::value> params_t;

/** Find out which translation system to use */
const TranslationSystem& getTranslationSystem(params_t params)
{
  string system_id = TranslationSystem::DEFAULT;
  params_t::const_iterator pi = params.find("system");
  if (pi != params.end()) {
    system_id = xmlrpc_c::value_string(pi->second);
  }
  VERBOSE(1, "Using translation system " << system_id << endl;)
  return StaticData::Instance().GetTranslationSystem(system_id);
}

class Updater: public xmlrpc_c::method
{
public:
  Updater() {
    // signature and help strings are documentation -- the client
    // can query this information with a system.methodSignature and
    // system.methodHelp RPC.
    this->_signature = "S:S";
    this->_help = "Updates stuff";
  }
  void
  execute(xmlrpc_c::paramList const& paramList,
          xmlrpc_c::value *   const  retvalP) {
    const params_t params = paramList.getStruct(0);
    breakOutParams(params);
    const TranslationSystem& system = getTranslationSystem(params);
    const PhraseDictionaryFeature* pdf = system.GetPhraseDictionaries()[0];
    PhraseDictionaryDynSuffixArray* pdsa = (PhraseDictionaryDynSuffixArray*) pdf->GetDictionary();
    cerr << "Inserting into address " << pdsa << endl;
    pdsa->insertSnt(source_, target_, alignment_);
    if(add2ORLM_) {       
      updateORLM();
    }
    cerr << "Done inserting\n";
    //PhraseDictionary* pdsa = (PhraseDictionary*) pdf->GetDictionary(*dummy);
    map<string, xmlrpc_c::value> retData;
    //*retvalP = xmlrpc_c::value_struct(retData);
    pdf = 0;
    pdsa = 0;
    *retvalP = xmlrpc_c::value_string("Phrase table updated");
  }
  string source_, target_, alignment_;
  bool bounded_, add2ORLM_;
  void updateORLM() {
#ifdef LM_ORLM
    vector<string> vl;
    map<vector<string>, int> ngSet;
    LMList lms = StaticData::Instance().GetLMList(); // get LM
    LMList::const_iterator lmIter = lms.begin();
    const LanguageModel* lm = *lmIter; 
    /* currently assumes a single LM that is a ORLM */
#ifdef WITH_THREADS
    boost::shared_ptr<LanguageModelORLM> orlm; 
    orlm = boost::dynamic_pointer_cast<LanguageModelORLM>(lm->GetLMImplementation()); 
#else 
    LanguageModelORLM* orlm; 
    orlm = (LanguageModelORLM*)lm->GetLMImplementation(); 
#endif
    if(orlm == 0) {
      cerr << "WARNING: Unable to add target sentence to ORLM\n";
      return;
    }
    // break out new ngrams from sentence
    const int ngOrder(orlm->GetNGramOrder());
    const std::string sBOS = orlm->GetSentenceStart()->GetString();
    const std::string sEOS = orlm->GetSentenceEnd()->GetString();
    Utils::splitToStr(target_, vl, " ");
    // insert BOS and EOS 
    vl.insert(vl.begin(), sBOS); 
    vl.insert(vl.end(), sEOS);
    for(int j=0; j < vl.size(); ++j) {
      int i = (j<ngOrder) ? 0 : j-ngOrder+1;
      for(int t=j; t >= i; --t) {
        vector<string> ngVec;
        for(int s=t; s<=j; ++s) {
          ngVec.push_back(vl[s]);
          //cerr << vl[s] << " ";
        }
        ngSet[ngVec]++;
        //cerr << endl;
      }
    }
    // insert into LM in order from 1grams up (for LM well-formedness)
    cerr << "Inserting " << ngSet.size() << " ngrams into ORLM...\n";
    for(int i=1; i <= ngOrder; ++i) {
      iterate(ngSet, it) {
        if(it->first.size() == i)
          orlm->UpdateORLM(it->first, it->second);
      }
    }
#endif
  }
  void breakOutParams(const params_t& params) {
    params_t::const_iterator si = params.find("source");
    if(si == params.end())
      throw xmlrpc_c::fault("Missing source sentence", xmlrpc_c::fault::CODE_PARSE);
    source_ = xmlrpc_c::value_string(si->second);
    cerr << "source = " << source_ << endl;
    si = params.find("target");
    if(si == params.end())
      throw xmlrpc_c::fault("Missing target sentence", xmlrpc_c::fault::CODE_PARSE);
    target_ = xmlrpc_c::value_string(si->second);
    cerr << "target = " << target_ << endl;
    si = params.find("alignment");
    if(si == params.end())
      throw xmlrpc_c::fault("Missing alignment", xmlrpc_c::fault::CODE_PARSE);
    alignment_ = xmlrpc_c::value_string(si->second);
    cerr << "alignment = " << alignment_ << endl;
    si = params.find("bounded");
    bounded_ = (si != params.end());
    si = params.find("updateORLM");
    add2ORLM_ = (si != params.end());
  }
};

class Translator : public xmlrpc_c::method
{
public:
  Translator() {
    // signature and help strings are documentation -- the client
    // can query this information with a system.methodSignature and
    // system.methodHelp RPC.
    this->_signature = "S:S";
    this->_help = "Does translation";
  }

  void execute(xmlrpc_c::paramList const& paramList,
          xmlrpc_c::value * const  retvalP) {
    const params_t params = paramList.getStruct(0);
    paramList.verifyEnd(1);
    params_t::const_iterator si = params.find("text");
    if (si == params.end()) {
      throw xmlrpc_c::fault(
        "Missing source text",
        xmlrpc_c::fault::CODE_PARSE);
    }
    const string source(
      (xmlrpc_c::value_string(si->second)));

    cerr << "Input: " << source;
    si = params.find("align");
    bool addAlignInfo = (si != params.end());
    si = params.find("sg");
    bool addGraphInfo = (si != params.end());
    si = params.find("topt");
    bool addTopts = (si != params.end());
    si = params.find("report-all-factors");
    bool reportAllFactors = (si != params.end());

    const StaticData &staticData = StaticData::Instance();

    if (addGraphInfo) {
      (const_cast<StaticData&>(staticData)).SetOutputSearchGraph(true);
    }

    const TranslationSystem& system = getTranslationSystem(params);

    Sentence sentence;
    const vector<FactorType> &inputFactorOrder =
      staticData.GetInputFactorOrder();
    stringstream in(source + "\n");
    sentence.Read(in,inputFactorOrder);
    Manager manager(sentence,staticData.GetSearchAlgorithm(), &system);
    manager.ProcessSentence();
    const Hypothesis* hypo = manager.GetBestHypothesis();

    vector<xmlrpc_c::value> alignInfo;
    stringstream out, graphInfo, transCollOpts;
    outputHypo(out,hypo,addAlignInfo,alignInfo,reportAllFactors);

    map<string, xmlrpc_c::value> retData;
    pair<string, xmlrpc_c::value>
    text("text", xmlrpc_c::value_string(out.str()));
    cerr << "Output: " << out.str() << endl << endl;
    if (addAlignInfo) {
      retData.insert(pair<string, xmlrpc_c::value>("align", xmlrpc_c::value_array(alignInfo)));
    }
    retData.insert(text);

    if(addGraphInfo) {
      insertGraphInfo(manager,retData);
      (const_cast<StaticData&>(staticData)).SetOutputSearchGraph(false);
    }
    if (addTopts) {
      insertTranslationOptions(manager,retData);
    }
    *retvalP = xmlrpc_c::value_struct(retData);
  }

  void outputHypo(ostream& out, const Hypothesis* hypo, bool addAlignmentInfo, vector<xmlrpc_c::value>& alignInfo, bool reportAllFactors = false) {
    if (hypo->GetPrevHypo() != NULL) {
      outputHypo(out,hypo->GetPrevHypo(),addAlignmentInfo, alignInfo, reportAllFactors);
      Phrase p = hypo->GetCurrTargetPhrase();
      if(reportAllFactors) {
        out << p << " ";
      } else {
        for (size_t pos = 0 ; pos < p.GetSize() ; pos++) {
          const Factor *factor = p.GetFactor(pos, 0);
          out << *factor << " ";
        }
      }

      if (addAlignmentInfo) {
        /**
         * Add the alignment info to the array. This is in target order and consists of
         *       (tgt-start, src-start, src-end) triples.
         **/
        map<string, xmlrpc_c::value> phraseAlignInfo;
        phraseAlignInfo["tgt-start"] = xmlrpc_c::value_int(hypo->GetCurrTargetWordsRange().GetStartPos());
        phraseAlignInfo["src-start"] = xmlrpc_c::value_int(hypo->GetCurrSourceWordsRange().GetStartPos());
        phraseAlignInfo["src-end"] = xmlrpc_c::value_int(hypo->GetCurrSourceWordsRange().GetEndPos());
        alignInfo.push_back(xmlrpc_c::value_struct(phraseAlignInfo));
      }
    }
  }

  void insertGraphInfo(Manager& manager, map<string, xmlrpc_c::value>& retData) {
    vector<xmlrpc_c::value> searchGraphXml;
    vector<SearchGraphNode> searchGraph;
    manager.GetSearchGraph(searchGraph);
    for (vector<SearchGraphNode>::const_iterator i = searchGraph.begin(); i != searchGraph.end(); ++i) {
      map<string, xmlrpc_c::value> searchGraphXmlNode;
      searchGraphXmlNode["forward"] = xmlrpc_c::value_double(i->forward);
      searchGraphXmlNode["fscore"] = xmlrpc_c::value_double(i->fscore);
      const Hypothesis* hypo = i->hypo;
      searchGraphXmlNode["hyp"] = xmlrpc_c::value_int(hypo->GetId());
      searchGraphXmlNode["stack"] = xmlrpc_c::value_int(hypo->GetWordsBitmap().GetNumWordsCovered());
      if (hypo->GetId() != 0) {
        const Hypothesis *prevHypo = hypo->GetPrevHypo();
        searchGraphXmlNode["back"] = xmlrpc_c::value_int(prevHypo->GetId());
        searchGraphXmlNode["score"] = xmlrpc_c::value_double(hypo->GetScore());
        searchGraphXmlNode["transition"] = xmlrpc_c::value_double(hypo->GetScore() - prevHypo->GetScore());
        if (i->recombinationHypo) {
          searchGraphXmlNode["recombined"] = xmlrpc_c::value_int(i->recombinationHypo->GetId());
        }
        searchGraphXmlNode["cover-start"] = xmlrpc_c::value_int(hypo->GetCurrSourceWordsRange().GetStartPos());
        searchGraphXmlNode["cover-end"] = xmlrpc_c::value_int(hypo->GetCurrSourceWordsRange().GetEndPos());
        stringstream tmp;
        tmp << hypo->GetSourcePhraseStringRep() << "|" << hypo->GetCurrTargetPhrase().GetStringRep(StaticData::Instance().GetOutputFactorOrder());
        searchGraphXmlNode["out"] = xmlrpc_c::value_string(tmp.str());
        tmp.str("");
        tmp << hypo->GetScoreBreakdown();
        searchGraphXmlNode["scores"] = xmlrpc_c::value_string(tmp.str());
      }
      searchGraphXml.push_back(xmlrpc_c::value_struct(searchGraphXmlNode));
    }
    retData.insert(pair<string, xmlrpc_c::value>("sg", xmlrpc_c::value_array(searchGraphXml)));
  }

  void insertTranslationOptions(Manager& manager, map<string, xmlrpc_c::value>& retData) {
    const TranslationOptionCollection* toptsColl = manager.getSntTranslationOptions();
    vector<xmlrpc_c::value> toptsXml;
    for (size_t startPos = 0 ; startPos < toptsColl->GetSize() ; ++startPos) {
      size_t maxSize = toptsColl->GetSize() - startPos;
      size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      maxSize = std::min(maxSize, maxSizePhrase);

      for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos) {
        WordsRange range(startPos,endPos);
        const TranslationOptionList& fullList = toptsColl->GetTranslationOptionList(range);
        for (size_t i = 0; i < fullList.size(); i++) {
          const TranslationOption* topt = fullList.Get(i);
          map<string, xmlrpc_c::value> toptXml;
          toptXml["phrase"] = xmlrpc_c::value_string(topt->GetTargetPhrase().
                              GetStringRep(StaticData::Instance().GetOutputFactorOrder()));
          toptXml["fscore"] = xmlrpc_c::value_double(topt->GetFutureScore());
          toptXml["start"] =  xmlrpc_c::value_int(startPos);
          toptXml["end"] =  xmlrpc_c::value_int(endPos);
          vector<xmlrpc_c::value> scoresXml;
          cerr << "Warning: not adding scores to translation options.." << endl;
          ScoreComponentCollection scores = topt->GetScoreBreakdown();
//          for (size_t j = 0; j < scores.size(); ++j) {
//            scoresXml.push_back(xmlrpc_c::value_double(scores[j]));
//          }
          toptXml["scores"] = xmlrpc_c::value_array(scoresXml);
          toptsXml.push_back(xmlrpc_c::value_struct(toptXml));
        }
      }
    }
    retData.insert(pair<string, xmlrpc_c::value>("topt", xmlrpc_c::value_array(toptsXml)));
  }
};

/*const char* ffNames[] = { "Distortion", "WordPenalty", "!UnknownWordPenalty 1", "LexicalReordering_wbe-msd-bidirectional-fe-allff_1",
		"LexicalReordering_wbe-msd-bidirectional-fe-allff_2", "LexicalReordering_wbe-msd-bidirectional-fe-allff_3", 
		"LexicalReordering_wbe-msd-bidirectional-fe-allff_4", "LexicalReordering_wbe-msd-bidirectional-fe-allff_5", 
		"LexicalReordering_wbe-msd-bidirectional-fe-allff_6", "LM", "PhraseModel_1", "PhraseModel_2", "PhraseModel_3",
		"PhraseModel_4", "PhraseModel_5" };*/

const char* ffNames[] = { "Distortion", "WordPenalty", "!UnknownWordPenalty 1", "LM", "PhraseModel_1", "PhraseModel_2" };

class WeightUpdater: public xmlrpc_c::method
{	
public:
  WeightUpdater() {
    // signature and help strings are documentation -- the client
    // can query this information with a system.methodSignature and
    // system.methodHelp RPC.
    this->_signature = "S:S";
    this->_help = "Updates Moses weights";
  }
  
  void execute(xmlrpc_c::paramList const& paramList,
          xmlrpc_c::value * const  retvalP) {
    const params_t params = paramList.getStruct(0);
    paramList.verifyEnd(1);
    
    ScoreComponentCollection updatedWeights;
    
    params_t::const_iterator si = params.find("core-weights");
    string coreWeights;
    if (si == params.end()) {
      throw xmlrpc_c::fault(
        "Missing core weights",
        xmlrpc_c::fault::CODE_PARSE);
    }
    coreWeights = xmlrpc_c::value_string(si->second);
    VERBOSE(1, "core weights: " << coreWeights << endl);

    StaticData &staticData = StaticData::InstanceNonConst();
	const vector<const ScoreProducer*> featureFunctions = staticData.GetTranslationSystem(TranslationSystem::DEFAULT).GetFeatureFunctions();
	vector<string> coreWeightVector;
	boost::split(coreWeightVector, coreWeights, boost::is_any_of(","));
	loadCoreWeight(updatedWeights, coreWeightVector, featureFunctions);
	    
    si = params.find("sparse-weights");
    string sparseWeights;
    if (si != params.end()) {
      sparseWeights = xmlrpc_c::value_string(si->second);
      VERBOSE(1, "sparse weights: " << sparseWeights << endl);
      
      vector<string> sparseWeightVector;
      boost::split(sparseWeightVector, sparseWeights, boost::is_any_of("\t "));
      for(size_t i=0; i<sparseWeightVector.size(); ++i) {
    	vector<string> name_value; 
    	boost::split(name_value, sparseWeightVector[i], boost::is_any_of("="));
    	if (name_value.size() > 2) {
    		string tmp1 = name_value[name_value.size()-1];
    		name_value.erase(name_value.end());
    		string tmp2 = boost::algorithm::join(name_value, "=");
    		name_value[0] = tmp2;
    		name_value[1] = tmp1;
    	}
    	const string name(name_value[0]);
    	float value = Scan<float>(name_value[1]);
    	VERBOSE(1, "Setting sparse weight " << name << " to value " << value << "." << endl);
		updatedWeights.Assign(name, value);
      }
    }

    staticData.SetAllWeights(updatedWeights);
    cerr << "\nUpdated weights: " << staticData.GetAllWeights() << endl;
    *retvalP = xmlrpc_c::value_string("Weights updated!");
  }
  
  bool loadCoreWeight(ScoreComponentCollection &weights, vector<string> &coreWeightVector, const vector<const ScoreProducer*> &featureFunctions) {
	vector< float > store_weights;
	for (size_t i=0; i<coreWeightVector.size(); ++i) {
	  string name(ffNames[i]);
	  float weight = Scan<float>(coreWeightVector[i]);  
		
	  VERBOSE(1, "loading core weight " << name << endl);
	  for (size_t i=0; i < featureFunctions.size(); ++i) {
		std::string prefix = featureFunctions[i]->GetScoreProducerDescription();
		if (name.substr( 0, prefix.length() ).compare( prefix ) == 0) {
		  size_t numberScoreComponents = featureFunctions[i]->GetNumScoreComponents();
  	      if (numberScoreComponents == 1) {
  	    	VERBOSE(1, "assign 1 weight for " << featureFunctions[i]->GetScoreProducerDescription());
  	    	VERBOSE(1, " (" << weight << ")" << endl << endl);
  		    weights.Assign(featureFunctions[i], weight);
  	      }
  	      else {
  	    	store_weights.push_back(weight);
  		    if (store_weights.size() == numberScoreComponents) {
  		      VERBOSE(1, "assign " << store_weights.size() << " weights for " << featureFunctions[i]->GetScoreProducerDescription() << " (");
  		      for (size_t j=0; j < store_weights.size(); ++j)
  		    	VERBOSE(1, store_weights[j] << " ");
  		      VERBOSE(1, ")" << endl << endl);
  		      weights.Assign(featureFunctions[i], store_weights);
  		      store_weights.clear();
  		    }			
  	      }
		}
	  }
	}
    return true;
  }
};

/**
  * Allocates a char* and copies string into it.
**/
static char* strToChar(const string& s) {
  char* c = new char[s.size()+1];
  strcpy(c,s.c_str());
  return c;
}

int main(int argc, char** argv)
{

  //Extract port and log, send other args to moses
  char** mosesargv = new char*[argc+2];
  int mosesargc = 0;
  int port = 8080;
  const char* logfile = "/dev/null";
  bool isSerial = false;

  for (int i = 0; i < argc; ++i) {
    if (!strcmp(argv[i],"--server-port")) {
      ++i;
      if (i >= argc) {
        cerr << "Error: Missing argument to --server-port" << endl;
        exit(1);
      } else {
        port = atoi(argv[i]);
      }
    } else if (!strcmp(argv[i],"--server-log")) {
      ++i;
      if (i >= argc) {
        cerr << "Error: Missing argument to --server-log" << endl;
        exit(1);
      } else {
        logfile = argv[i];
      }
    } else if (!strcmp(argv[i], "--serial")) {
      cerr << "Running single-threaded server" << endl;
      isSerial = true;
    } else {
      mosesargv[mosesargc] = new char[strlen(argv[i])+1];
      strcpy(mosesargv[mosesargc],argv[i]);
      ++mosesargc;
    }
  }
  
  cerr << "Switching off translation option cache.." << endl;
  mosesargv[mosesargc] = strToChar("-use-persistent-cache");
  ++mosesargc;
  mosesargv[mosesargc] = strToChar("0");
  ++mosesargc;
  mosesargv[mosesargc] = strToChar("-persistent-cache-size");
  ++mosesargc;
  mosesargv[mosesargc] = strToChar("0");
  ++mosesargc;
  
  Parameter* params = new Parameter();
  if (!params->LoadParam(mosesargc,mosesargv)) {
    params->Explain();
    exit(1);
  }
  if (!StaticData::LoadDataStatic(params)) {
    exit(1);
  }
  
  cerr << "start weights: " << StaticData::Instance().GetAllWeights() << endl;

  xmlrpc_c::registry myRegistry;

  xmlrpc_c::methodPtr const translator(new Translator);
  xmlrpc_c::methodPtr const updater(new Updater);
  xmlrpc_c::methodPtr const weightUpdater(new WeightUpdater);

  myRegistry.addMethod("translate", translator);
  myRegistry.addMethod("updater", updater);
  myRegistry.addMethod("updateWeights", weightUpdater);

  xmlrpc_c::serverAbyss myAbyssServer(
    myRegistry,
    port,              // TCP port on which to listen
    logfile
  );

  cerr << "Listening on port " << port << endl;
  if (isSerial) {
    while(1) {
      myAbyssServer.runOnce();
    }
  } else {
    myAbyssServer.run();
  }
  // xmlrpc_c::serverAbyss.run() never returns
  CHECK(false);
  return 0;
}
