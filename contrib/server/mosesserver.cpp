#include "util/check.hh"
#include <stdexcept>
#include <iostream>


#include "moses/ChartManager.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/StaticData.h"
#include "moses/PhraseDictionaryDynSuffixArray.h"
#include "moses/TranslationSystem.h"
#include "moses/TreeInput.h"
#include "moses/LMList.h"
#include "moses/LM/ORLM.h"

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

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
    // TODO(level101): this belongs in the language model, not in moseserver.cpp
    vector<string> vl;
    map<vector<string>, int> ngSet;
    LMList lms = StaticData::Instance().GetLMList(); // get LM
    LMList::const_iterator lmIter = lms.begin();
    LanguageModelORLM* orlm = static_cast<LanguageModelORLM*>(static_cast<LMRefCount*>(*lmIter)->MosesServerCppShouldNotHaveLMCode());
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

  void
  execute(xmlrpc_c::paramList const& paramList,
          xmlrpc_c::value *   const  retvalP) {

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

    cerr << "Input: " << source << endl;
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
    stringstream out, graphInfo, transCollOpts;
    map<string, xmlrpc_c::value> retData;

    if (staticData.IsChart()) {
       TreeInput tinput; 
        const vector<FactorType> &inputFactorOrder =
          staticData.GetInputFactorOrder();
        stringstream in(source + "\n");
        tinput.Read(in,inputFactorOrder);
        ChartManager manager(tinput, &system);
        manager.ProcessSentence();
        const ChartHypothesis *hypo = manager.GetBestHypothesis();
        outputChartHypo(out,hypo);
    } else {
        Sentence sentence;
        const vector<FactorType> &inputFactorOrder =
          staticData.GetInputFactorOrder();
        stringstream in(source + "\n");
        sentence.Read(in,inputFactorOrder);
	size_t lineNumber = 0; // TODO: Include sentence request number here?
        Manager manager(lineNumber, sentence, staticData.GetSearchAlgorithm(), &system);
        manager.ProcessSentence();
        const Hypothesis* hypo = manager.GetBestHypothesis();

        vector<xmlrpc_c::value> alignInfo;
        outputHypo(out,hypo,addAlignInfo,alignInfo,reportAllFactors);
        if (addAlignInfo) {
          retData.insert(pair<string, xmlrpc_c::value>("align", xmlrpc_c::value_array(alignInfo)));
        }

        if(addGraphInfo) {
          insertGraphInfo(manager,retData);
            (const_cast<StaticData&>(staticData)).SetOutputSearchGraph(false);
        }
        if (addTopts) {
          insertTranslationOptions(manager,retData);
        }
    }
    pair<string, xmlrpc_c::value>
    text("text", xmlrpc_c::value_string(out.str()));
    retData.insert(text);
    cerr << "Output: " << out.str() << endl;
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

  void outputChartHypo(ostream& out, const ChartHypothesis* hypo) {
    Phrase outPhrase(20);
    hypo->CreateOutputPhrase(outPhrase);

    // delete 1st & last
    assert(outPhrase.GetSize() >= 2);
    outPhrase.RemoveWord(0);
    outPhrase.RemoveWord(outPhrase.GetSize() - 1);
    for (size_t pos = 0 ; pos < outPhrase.GetSize() ; pos++) {
      const Factor *factor = outPhrase.GetFactor(pos, 0);
      out << *factor << " ";
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
        searchGraphXmlNode["out"] =
          xmlrpc_c::value_string(hypo->GetCurrTargetPhrase().GetStringRep(StaticData::Instance().GetOutputFactorOrder()));
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
          const std::valarray<FValue> &scores = topt->GetScoreBreakdown().getCoreFeatures();
          for (size_t j = 0; j < scores.size(); ++j) {
            scoresXml.push_back(xmlrpc_c::value_double(scores[j]));
          }
          toptXml["scores"] = xmlrpc_c::value_array(scoresXml);
          toptsXml.push_back(xmlrpc_c::value_struct(toptXml));
        }
      }
    }
    retData.insert(pair<string, xmlrpc_c::value>("topt", xmlrpc_c::value_array(toptsXml)));
  }



};


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

  Parameter* params = new Parameter();
  if (!params->LoadParam(mosesargc,mosesargv)) {
    params->Explain();
    exit(1);
  }
  if (!StaticData::LoadDataStatic(params, argv[0])) {
    exit(1);
  }

  xmlrpc_c::registry myRegistry;

  xmlrpc_c::methodPtr const translator(new Translator);
  xmlrpc_c::methodPtr const updater(new Updater);

  myRegistry.addMethod("translate", translator);
  myRegistry.addMethod("updater", updater);

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
