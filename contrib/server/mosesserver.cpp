#include <stdexcept>
#include <iostream>
#include <vector>
#include <algorithm>


#include "moses/Util.h"
#include "moses/ChartManager.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/StaticData.h"
#include "moses/ThreadPool.h"
#include "moses/TranslationModel/PhraseDictionaryDynSuffixArray.h"
#include "moses/TranslationModel/PhraseDictionaryMultiModelCounts.h"
#if PT_UG
#include "moses/TranslationModel/UG/mmsapt.h"
#endif
#include "moses/TreeInput.h"
#include "moses/LM/ORLM.h"
#include "moses/IOWrapper.h"

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

using namespace Moses;
using namespace std;

typedef std::map<std::string, xmlrpc_c::value> params_t;

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
#if PT_UG
    Mmsapt* pdsa = reinterpret_cast<Mmsapt*>(PhraseDictionary::GetColl()[0]);
    pdsa->add(source_,target_,alignment_);
#else
    const PhraseDictionary* pdf = PhraseDictionary::GetColl()[0];
    PhraseDictionaryDynSuffixArray* 
      pdsa = (PhraseDictionaryDynSuffixArray*) pdf;
    cerr << "Inserting into address " << pdsa << endl;
    pdsa->insertSnt(source_, target_, alignment_);
#endif
    if(add2ORLM_) {
      //updateORLM();
    }
    XVERBOSE(1,"Done inserting\n");
    //PhraseDictionary* pdsa = (PhraseDictionary*) pdf->GetDictionary(*dummy);
    map<string, xmlrpc_c::value> retData;
    //*retvalP = xmlrpc_c::value_struct(retData);
#ifndef PT_UG
    pdf = 0;
#endif
    pdsa = 0;
    *retvalP = xmlrpc_c::value_string("Phrase table updated");
  }
  string source_, target_, alignment_;
  bool bounded_, add2ORLM_;
  /*
  void updateORLM() {
    // TODO(level101): this belongs in the language model, not in moseserver.cpp
    vector<string> vl;
    map<vector<string>, int> ngSet;
    LMList lms = StaticData::Instance().GetLMList(); // get LM
    LMList::const_iterator lmIter = lms.begin();
    LanguageModel *lm = *lmIter;
    LanguageModelORLM* orlm = static_cast<LanguageModelORLM*>(lm);
    if(orlm == 0) {
      cerr << "WARNING: Unable to add target sentence to ORLM\n";
      return;
    }
    // break out new ngrams from sentence
    const int ngOrder(orlm->GetNGramOrder());
    const std::string sBOS = orlm->GetSentenceStart()->GetString().as_string();
    const std::string sEOS = orlm->GetSentenceEnd()->GetString().as_string();
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
  */
  
  void breakOutParams(const params_t& params) {
    params_t::const_iterator si = params.find("source");
    if(si == params.end())
      throw xmlrpc_c::fault("Missing source sentence", xmlrpc_c::fault::CODE_PARSE);
    source_ = xmlrpc_c::value_string(si->second);
    XVERBOSE(1,"source = " << source_ << endl);
    si = params.find("target");
    if(si == params.end())
      throw xmlrpc_c::fault("Missing target sentence", xmlrpc_c::fault::CODE_PARSE);
    target_ = xmlrpc_c::value_string(si->second);
    XVERBOSE(1,"target = " << target_ << endl);
    si = params.find("alignment");
    if(si == params.end())
      throw xmlrpc_c::fault("Missing alignment", xmlrpc_c::fault::CODE_PARSE);
    alignment_ = xmlrpc_c::value_string(si->second);
    XVERBOSE(1,"alignment = " << alignment_ << endl);
    si = params.find("bounded");
    bounded_ = (si != params.end());
    si = params.find("updateORLM");
    add2ORLM_ = (si != params.end());
  }
};

class Optimizer : public xmlrpc_c::method
{
public:
  Optimizer() {
    // signature and help strings are documentation -- the client
    // can query this information with a system.methodSignature and
    // system.methodHelp RPC.
    this->_signature = "S:S";
    this->_help = "Optimizes multi-model translation model";
  }

  void
  execute(xmlrpc_c::paramList const& paramList,
          xmlrpc_c::value *   const  retvalP) {
#ifdef WITH_DLIB
    const params_t params = paramList.getStruct(0);
    params_t::const_iterator si = params.find("model_name");
    if (si == params.end()) {
      throw xmlrpc_c::fault(
        "Missing name of model to be optimized (e.g. PhraseDictionaryMultiModelCounts0)",
        xmlrpc_c::fault::CODE_PARSE);
    }
    const string model_name = xmlrpc_c::value_string(si->second);
    PhraseDictionaryMultiModel* pdmm = (PhraseDictionaryMultiModel*) FindPhraseDictionary(model_name);

    si = params.find("phrase_pairs");
    if (si == params.end()) {
      throw xmlrpc_c::fault(
        "Missing list of phrase pairs",
        xmlrpc_c::fault::CODE_PARSE);
    }

    vector<pair<string, string> > phrase_pairs;

    xmlrpc_c::value_array phrase_pairs_array = xmlrpc_c::value_array(si->second);
    vector<xmlrpc_c::value> phrasePairValueVector(phrase_pairs_array.vectorValueValue());
    for (size_t i=0;i < phrasePairValueVector.size();i++) {
        xmlrpc_c::value_array phrasePairArray = xmlrpc_c::value_array(phrasePairValueVector[i]);
        vector<xmlrpc_c::value> phrasePair(phrasePairArray.vectorValueValue());
        string L1 = xmlrpc_c::value_string(phrasePair[0]);
        string L2 = xmlrpc_c::value_string(phrasePair[1]);
        phrase_pairs.push_back(make_pair(L1,L2));
    }

    vector<float> weight_vector;
    weight_vector = pdmm->MinimizePerplexity(phrase_pairs);

    vector<xmlrpc_c::value> weight_vector_ret;
    for (size_t i=0;i < weight_vector.size();i++) {
        weight_vector_ret.push_back(xmlrpc_c::value_double(weight_vector[i]));
    }
    *retvalP = xmlrpc_c::value_array(weight_vector_ret);
#else
    string errmsg = "Error: Perplexity minimization requires dlib (compilation option --with-dlib)";
    cerr << errmsg << endl;
    *retvalP = xmlrpc_c::value_string(errmsg);
#endif
  }
};

/**
  * Required so that translations can be sent to a thread pool.
**/
class TranslationTask : public virtual Moses::Task {
public:
  TranslationTask(xmlrpc_c::paramList const& paramList,
    boost::condition_variable& cond, boost::mutex& mut) 
   : m_paramList(paramList),
     m_cond(cond),
     m_mut(mut),
     m_done(false)
     {}

  virtual bool DeleteAfterExecution() {return false;}

  bool IsDone() const {return m_done;}

  const map<string, xmlrpc_c::value>& GetRetData() { return m_retData;}

  virtual void Run() {

    const params_t params = m_paramList.getStruct(0);
    m_paramList.verifyEnd(1);
    params_t::const_iterator si = params.find("text");
    if (si == params.end()) {
      throw xmlrpc_c::fault(
        "Missing source text",
        xmlrpc_c::fault::CODE_PARSE);
    }
    const string source((xmlrpc_c::value_string(si->second)));

    XVERBOSE(1,"Input: " << source << endl);
    si = params.find("align");
    bool addAlignInfo = (si != params.end());
    si = params.find("word-align");
    bool addWordAlignInfo = (si != params.end());
    si = params.find("sg");
    bool addGraphInfo = (si != params.end());
    si = params.find("topt");
    bool addTopts = (si != params.end());
    si = params.find("report-all-factors");
    bool reportAllFactors = (si != params.end());
    si = params.find("nbest");
    int nbest_size = (si == params.end()) ? 0 : int(xmlrpc_c::value_int(si->second));
    si = params.find("nbest-distinct");
    bool nbest_distinct = (si != params.end());

    si = params.find("add-score-breakdown");
    bool addScoreBreakdown = (si != params.end());

    vector<float> multiModelWeights;
    si = params.find("lambda");
    if (si != params.end()) {
        xmlrpc_c::value_array multiModelArray = xmlrpc_c::value_array(si->second);
        vector<xmlrpc_c::value> multiModelValueVector(multiModelArray.vectorValueValue());
        for (size_t i=0;i < multiModelValueVector.size();i++) {
            multiModelWeights.push_back(xmlrpc_c::value_double(multiModelValueVector[i]));
        }
    }

    si = params.find("model_name");
    if (si != params.end() && multiModelWeights.size() > 0) {
        const string model_name = xmlrpc_c::value_string(si->second);
        PhraseDictionaryMultiModel* pdmm = (PhraseDictionaryMultiModel*) FindPhraseDictionary(model_name);
        pdmm->SetTemporaryMultiModelWeightsVector(multiModelWeights);
    }

    const StaticData &staticData = StaticData::Instance();

    //Make sure alternative paths are retained, if necessary
    if (addGraphInfo || nbest_size>0) {
      (const_cast<StaticData&>(staticData)).SetOutputSearchGraph(true);
    }


    stringstream out, graphInfo, transCollOpts;

    if (staticData.IsChart()) {
       TreeInput tinput;
        const vector<FactorType>& 
	      inputFactorOrder = staticData.GetInputFactorOrder();
        stringstream in(source + "\n");
        tinput.Read(in,inputFactorOrder);
        ChartManager manager(tinput);
        manager.Decode();
        const ChartHypothesis *hypo = manager.GetBestHypothesis();
        outputChartHypo(out,hypo);
        if (addGraphInfo) {
          // const size_t translationId = tinput.GetTranslationId();
          std::ostringstream sgstream;
          manager.OutputSearchGraphMoses(sgstream);
          m_retData.insert(pair<string, xmlrpc_c::value>("sg", xmlrpc_c::value_string(sgstream.str())));
        }
    } else {
        size_t lineNumber = 0; // TODO: Include sentence request number here?
        Sentence sentence;
        sentence.SetTranslationId(lineNumber);

        const vector<FactorType> &
	      inputFactorOrder = staticData.GetInputFactorOrder();
        stringstream in(source + "\n");
        sentence.Read(in,inputFactorOrder);
        Manager manager(sentence);
	      manager.Decode();
        const Hypothesis* hypo = manager.GetBestHypothesis();

        vector<xmlrpc_c::value> alignInfo;
        outputHypo(out,hypo,addAlignInfo,alignInfo,reportAllFactors);
        if (addAlignInfo) {
          m_retData.insert(pair<string, xmlrpc_c::value>("align", xmlrpc_c::value_array(alignInfo)));
        }
        if (addWordAlignInfo) {
          stringstream wordAlignment;
          hypo->OutputAlignment(wordAlignment);
          vector<xmlrpc_c::value> alignments;
          string alignmentPair;
          while (wordAlignment >> alignmentPair) {
          	int pos = alignmentPair.find('-');
          	map<string, xmlrpc_c::value> wordAlignInfo;
          	wordAlignInfo["source-word"] = xmlrpc_c::value_int(atoi(alignmentPair.substr(0, pos).c_str()));
          	wordAlignInfo["target-word"] = xmlrpc_c::value_int(atoi(alignmentPair.substr(pos + 1).c_str()));
          	alignments.push_back(xmlrpc_c::value_struct(wordAlignInfo));
          }
          m_retData.insert(pair<string, xmlrpc_c::value_array>("word-align", alignments));
        }

        if (addGraphInfo) {
          insertGraphInfo(manager,m_retData);
        }
        if (addTopts) {
          insertTranslationOptions(manager,m_retData);
        }
        if (nbest_size>0) {
          outputNBest(manager, m_retData, nbest_size, nbest_distinct, 
		      reportAllFactors, addAlignInfo, addScoreBreakdown);
        }
        (const_cast<StaticData&>(staticData)).SetOutputSearchGraph(false);

    }
    pair<string, xmlrpc_c::value>
    text("text", xmlrpc_c::value_string(out.str()));
    m_retData.insert(text);
    XVERBOSE(1,"Output: " << out.str() << endl);
    {
      boost::lock_guard<boost::mutex> lock(m_mut);
      m_done = true;
    }
    m_cond.notify_one();

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
         * Add the alignment info to the array. This is in target
         * order and consists of (tgt-start, src-start, src-end)
         * triples.
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
    hypo->GetOutputPhrase(outPhrase);

    // delete 1st & last
    assert(outPhrase.GetSize() >= 2);
    outPhrase.RemoveWord(0);
    outPhrase.RemoveWord(outPhrase.GetSize() - 1);
    for (size_t pos = 0 ; pos < outPhrase.GetSize() ; pos++) {
      const Factor *factor = outPhrase.GetFactor(pos, 0);
      out << *factor << " ";
    }

  }

  bool compareSearchGraphNode(const SearchGraphNode& a, const SearchGraphNode b) {
    return a.hypo->GetId() < b.hypo->GetId();
  }

  void insertGraphInfo(Manager& manager, map<string, xmlrpc_c::value>& retData) {
    vector<xmlrpc_c::value> searchGraphXml;
    vector<SearchGraphNode> searchGraph;
    manager.GetSearchGraph(searchGraph);
    std::sort(searchGraph.begin(), searchGraph.end());
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

  void outputNBest(const Manager& manager,
                   map<string, xmlrpc_c::value>& retData,
                   const int n=100,
                   const bool distinct=false,
                   const bool reportAllFactors=false,
                   const bool addAlignmentInfo=false,
		   const bool addScoreBreakdown=false)
  {
    TrellisPathList nBestList;
    manager.CalcNBest(n, nBestList, distinct);

    vector<xmlrpc_c::value> nBestXml;
    TrellisPathList::const_iterator iter;
    for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter) {
      const TrellisPath &path = **iter;
      const std::vector<const Hypothesis *> &edges = path.GetEdges();
      map<string, xmlrpc_c::value> nBestXMLItem;

      // output surface
      ostringstream out;
      vector<xmlrpc_c::value> alignInfo;
      for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
        const Hypothesis &edge = *edges[currEdge];
        const Phrase& phrase = edge.GetCurrTargetPhrase();
        if(reportAllFactors) {
          out << phrase << " ";
        } else {
          for (size_t pos = 0 ; pos < phrase.GetSize() ; pos++) {
            const Factor *factor = phrase.GetFactor(pos, 0);
            out << *factor << " ";
          }
        }

        if (addAlignmentInfo && currEdge != (int)edges.size() - 1) {
          map<string, xmlrpc_c::value> phraseAlignInfo;
          phraseAlignInfo["tgt-start"] = xmlrpc_c::value_int(edge.GetCurrTargetWordsRange().GetStartPos());
          phraseAlignInfo["src-start"] = xmlrpc_c::value_int(edge.GetCurrSourceWordsRange().GetStartPos());
          phraseAlignInfo["src-end"] = xmlrpc_c::value_int(edge.GetCurrSourceWordsRange().GetEndPos());
          alignInfo.push_back(xmlrpc_c::value_struct(phraseAlignInfo));
        }
      }
      nBestXMLItem["hyp"] = xmlrpc_c::value_string(out.str());

      if (addAlignmentInfo) {
        nBestXMLItem["align"] = xmlrpc_c::value_array(alignInfo);

        if ((int)edges.size() > 0) {
          stringstream wordAlignment;
					const Hypothesis *edge = edges[0];
          edge->OutputAlignment(wordAlignment);
          vector<xmlrpc_c::value> alignments;
          string alignmentPair;
          while (wordAlignment >> alignmentPair) {
          	int pos = alignmentPair.find('-');
          	map<string, xmlrpc_c::value> wordAlignInfo;
          	wordAlignInfo["source-word"] = xmlrpc_c::value_int(atoi(alignmentPair.substr(0, pos).c_str()));
          	wordAlignInfo["target-word"] = xmlrpc_c::value_int(atoi(alignmentPair.substr(pos + 1).c_str()));
          	alignments.push_back(xmlrpc_c::value_struct(wordAlignInfo));
          }
          nBestXMLItem["word-align"] = xmlrpc_c::value_array(alignments);
        }
      }

      if (addScoreBreakdown)
	{
	  // should the score breakdown be reported in a more structured manner?
	  ostringstream buf;
	  path.GetScoreBreakdown().OutputAllFeatureScores(buf);
	  nBestXMLItem["fvals"] = xmlrpc_c::value_string(buf.str());
	}

      // weighted score
      nBestXMLItem["totalScore"] = xmlrpc_c::value_double(path.GetTotalScore());
      nBestXml.push_back(xmlrpc_c::value_struct(nBestXMLItem));
    }
    retData.insert(pair<string, xmlrpc_c::value>("nbest", xmlrpc_c::value_array(nBestXml)));
  }

  void insertTranslationOptions(Manager& manager, map<string, xmlrpc_c::value>& retData) {
    const TranslationOptionCollection* toptsColl = manager.getSntTranslationOptions();
    vector<xmlrpc_c::value> toptsXml;
    for (size_t startPos = 0 ; startPos < toptsColl->GetSource().GetSize() ; ++startPos) {
      size_t maxSize = toptsColl->GetSource().GetSize() - startPos;
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

private:
  xmlrpc_c::paramList const& m_paramList;
  map<string, xmlrpc_c::value> m_retData;
  boost::condition_variable& m_cond;
  boost::mutex& m_mut;
  bool m_done;
};

class Translator : public xmlrpc_c::method
{
public:
  Translator(size_t numThreads = 10) : m_threadPool(numThreads) {
    // signature and help strings are documentation -- the client
    // can query this information with a system.methodSignature and
    // system.methodHelp RPC.
    this->_signature = "S:S";
    this->_help = "Does translation";
  }

  void
  execute(xmlrpc_c::paramList const& paramList,
          xmlrpc_c::value *   const  retvalP) {
    boost::condition_variable cond;
    boost::mutex mut;
    TranslationTask task(paramList,cond,mut);
    m_threadPool.Submit(&task);
    boost::unique_lock<boost::mutex> lock(mut);
    while (!task.IsDone()) {
      cond.wait(lock);
    }
    *retvalP = xmlrpc_c::value_struct(task.GetRetData());
  }
private:
  Moses::ThreadPool m_threadPool;
};

static 
void 
PrintFeatureWeight(ostream& out, const FeatureFunction* ff)
{
  out << ff->GetScoreProducerDescription() << "=";
  size_t numScoreComps = ff->GetNumScoreComponents();
  vector<float> values = StaticData::Instance().GetAllWeights().GetScoresForProducer(ff);
  for (size_t i = 0; i < numScoreComps; ++i) {
    out << " " << values[i];
  }
  out << endl;
}

static 
void 
ShowWeights(ostream& out)
{
  // adapted from moses-cmd/Main.cpp
  std::ios::fmtflags old_flags = out.setf(std::ios::fixed);
  size_t         old_precision = out.precision(6);
  const vector<const StatelessFeatureFunction*>& 
    slf = StatelessFeatureFunction::GetStatelessFeatureFunctions();
  const vector<const StatefulFeatureFunction*>& 
    sff = StatefulFeatureFunction::GetStatefulFeatureFunctions();

  for (size_t i = 0; i < sff.size(); ++i) {
    const StatefulFeatureFunction *ff = sff[i];
    if (ff->IsTuneable()) {
      PrintFeatureWeight(out,ff);
    }
    else {
      out << ff->GetScoreProducerDescription() << " UNTUNEABLE" << endl;
    }
  }
  for (size_t i = 0; i < slf.size(); ++i) {
    const StatelessFeatureFunction *ff = slf[i];
    if (ff->IsTuneable()) {
      PrintFeatureWeight(out,ff);
    }
    else {
      out << ff->GetScoreProducerDescription() << " UNTUNEABLE" << endl;
    }
  }
  if (! (old_flags & std::ios::fixed)) 
    out.unsetf(std::ios::fixed);
  out.precision(old_precision);
}

int main(int argc, char** argv)
{

  //Extract port and log, send other args to moses
  char** mosesargv = new char*[argc+2]; // why "+2" [UG]
  int mosesargc = 0;
  int port = 8080;
  const char* logfile = "/dev/null";
  bool isSerial = false;
  size_t numThreads = 10; //for translation tasks

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
    } else if (!strcmp(argv[i], "--threads")) {
      ++i;
      if (i>=argc) {
        cerr << "Error: Missing argument to --threads" << endl;
        exit(1);
      } else {
        numThreads = atoi(argv[i]);
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

  if (params->isParamSpecified("show-weights")) {
    ShowWeights(cout);
    exit(0);
  }

  //512 MB data limit (512KB is not enough for optimization)
  xmlrpc_limit_set(XMLRPC_XML_SIZE_LIMIT_ID, 512*1024*1024);

  xmlrpc_c::registry myRegistry;

  xmlrpc_c::methodPtr const translator(new Translator(numThreads));
  xmlrpc_c::methodPtr const updater(new Updater);
  xmlrpc_c::methodPtr const optimizer(new Optimizer);

  myRegistry.addMethod("translate", translator);
  myRegistry.addMethod("updater", updater);
  myRegistry.addMethod("optimize", optimizer);

  xmlrpc_c::serverAbyss myAbyssServer(
				      myRegistry,
				      port,              // TCP port on which to listen
				      logfile
				      );
  /* doesn't work with xmlrpc-c v. 1.16.33 - ie very old lib on Ubuntu 12.04
  xmlrpc_c::serverAbyss myAbyssServer(
    xmlrpc_c::serverAbyss::constrOpt()
    .registryPtr(&myRegistry)
    .portNumber(port)              // TCP port on which to listen
    .logFileName(logfile)
    .allowOrigin("*")
  );
  */
  
  XVERBOSE(1,"Listening on port " << port << endl);
  if (isSerial) {
    while(1) myAbyssServer.runOnce();
  } else {
    myAbyssServer.run();
  }
  std::cerr << "xmlrpc_c::serverAbyss.run() returned but should not." << std::endl;
  return 1;
}
