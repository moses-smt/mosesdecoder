#include "TranslationRequest.h"
#include "PackScores.h"
#include "moses/ContextScope.h"
#include <boost/foreach.hpp>
#include "moses/Util.h"
#include "moses/Hypothesis.h"

namespace MosesServer
{
using namespace std;
using Moses::Hypothesis;
using Moses::StaticData;
using Moses::Range;
using Moses::ChartHypothesis;
using Moses::Phrase;
using Moses::Manager;
using Moses::SearchGraphNode;
using Moses::TrellisPathList;
using Moses::TranslationOptionCollection;
using Moses::TranslationOptionList;
using Moses::TranslationOption;
using Moses::TargetPhrase;
using Moses::FValue;
using Moses::PhraseDictionaryMultiModel;
using Moses::FindPhraseDictionary;
using Moses::Sentence;
using Moses::TokenizeMultiCharSeparator;
using Moses::FeatureFunction;
using Moses::Scan;

boost::shared_ptr<TranslationRequest>
TranslationRequest::
create(Translator* translator, xmlrpc_c::paramList const& paramList,
       boost::condition_variable& cond, boost::mutex& mut)
{
  boost::shared_ptr<TranslationRequest> ret;
  ret.reset(new TranslationRequest(paramList, cond, mut));
  ret->m_self = ret;
  ret->m_translator = translator;
  return ret;
}

void
SetContextWeights(Moses::ContextScope& s, xmlrpc_c::value const& w)
{
  SPTR<std::map<std::string,float> > M(new std::map<std::string, float>);
  typedef std::map<std::string,xmlrpc_c::value> tmap;
  tmap const tmp = static_cast<tmap>(xmlrpc_c::value_struct(w));
  for(tmap::const_iterator m = tmp.begin(); m != tmp.end(); ++m)
    (*M)[m->first] = xmlrpc_c::value_double(m->second);
  s.SetContextWeights(M);
}
  
void
TranslationRequest::
Run()
{
  typedef std::map<std::string,xmlrpc_c::value> param_t;
  param_t const& params = m_paramList.getStruct(0);
  parse_request(params);
  // cerr << "SESSION ID" << ret->m_session_id << endl;


  // settings within the session scope
  param_t::const_iterator si = params.find("context-weights");
  if (si != params.end()) SetContextWeights(*m_scope, si->second);
  
  Moses::StaticData const& SD = Moses::StaticData::Instance();

  if (is_syntax(m_options->search.algo))
    run_chart_decoder();
  else
    run_phrase_decoder();

  {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    m_done = true;
  }
  m_cond.notify_one();

}

/// add phrase alignment information from a Hypothesis
void
TranslationRequest::
add_phrase_aln_info(Hypothesis const& h, vector<xmlrpc_c::value>& aInfo) const
{
  if (!m_withAlignInfo) return;
  //  if (!options()->output.ReportSegmentation) return;
  Range const& trg = h.GetCurrTargetWordsRange();
  Range const& src = h.GetCurrSourceWordsRange();

  std::map<std::string, xmlrpc_c::value> pAlnInfo;
  pAlnInfo["tgt-start"] = xmlrpc_c::value_int(trg.GetStartPos());
  pAlnInfo["tgt-end"] = xmlrpc_c::value_int(trg.GetEndPos());
  pAlnInfo["src-start"] = xmlrpc_c::value_int(src.GetStartPos());
  pAlnInfo["src-end"]   = xmlrpc_c::value_int(src.GetEndPos());
  aInfo.push_back(xmlrpc_c::value_struct(pAlnInfo));
}

void
TranslationRequest::
outputChartHypo(ostream& out, const ChartHypothesis* hypo)
{
  Phrase outPhrase(20);
  hypo->GetOutputPhrase(outPhrase);

  // delete 1st & last
  assert(outPhrase.GetSize() >= 2);
  outPhrase.RemoveWord(0);
  outPhrase.RemoveWord(outPhrase.GetSize() - 1);
  for (size_t pos = 0 ; pos < outPhrase.GetSize() ; pos++)
    out << *outPhrase.GetFactor(pos, 0) << " ";
}

bool
TranslationRequest::
compareSearchGraphNode(const Moses::SearchGraphNode& a,
                       const Moses::SearchGraphNode& b)
{
  return a.hypo->GetId() < b.hypo->GetId();
}

void
TranslationRequest::
insertGraphInfo(Manager& manager, map<string, xmlrpc_c::value>& retData)
{
  using xmlrpc_c::value_int;
  using xmlrpc_c::value_double;
  using xmlrpc_c::value_struct;
  using xmlrpc_c::value_string;
  vector<xmlrpc_c::value> searchGraphXml;
  vector<SearchGraphNode> searchGraph;
  manager.GetSearchGraph(searchGraph);
  std::sort(searchGraph.begin(), searchGraph.end());
  BOOST_FOREACH(Moses::SearchGraphNode const& n, searchGraph) {
    map<string, xmlrpc_c::value> x; // search graph xml node
    x["forward"] = value_double(n.forward);
    x["fscore"] = value_double(n.fscore);
    const Hypothesis* hypo = n.hypo;
    x["hyp"] = value_int(hypo->GetId());
    x["stack"] = value_int(hypo->GetWordsBitmap().GetNumWordsCovered());
    if (hypo->GetId() != 0) {
      const Hypothesis *prevHypo = hypo->GetPrevHypo();
      x["back"] = value_int(prevHypo->GetId());
      x["score"] = value_double(hypo->GetScore());
      x["transition"] = value_double(hypo->GetScore() - prevHypo->GetScore());
      if (n.recombinationHypo)
        x["recombined"] = value_int(n.recombinationHypo->GetId());
      x["cover-start"] = value_int(hypo->GetCurrSourceWordsRange().GetStartPos());
      x["cover-end"] = value_int(hypo->GetCurrSourceWordsRange().GetEndPos());
      x["out"] = value_string(hypo->GetCurrTargetPhrase().GetStringRep(options()->output.factor_order));
    }
    searchGraphXml.push_back(value_struct(x));
  }
  retData["sg"] = xmlrpc_c::value_array(searchGraphXml);
}

void
TranslationRequest::
outputNBest(const Manager& manager, map<string, xmlrpc_c::value>& retData)
{
  TrellisPathList nBestList;
  vector<xmlrpc_c::value> nBestXml;

  Moses::NBestOptions const& nbo = m_options->nbest; 
  manager.CalcNBest(nbo.nbest_size, nBestList, nbo.only_distinct);
  manager.OutputNBest(cout, nBestList); 

  BOOST_FOREACH(Moses::TrellisPath const* path, nBestList) {
    vector<const Hypothesis *> const& E = path->GetEdges();
    if (!E.size()) continue;
    std::map<std::string, xmlrpc_c::value> nBestXmlItem;
    pack_hypothesis(manager, E, "hyp", nBestXmlItem);
    if (m_withScoreBreakdown) {
      // should the score breakdown be reported in a more structured manner?
      ostringstream buf;
      bool with_labels = nbo.include_feature_labels;
      path->GetScoreBreakdown()->OutputAllFeatureScores(buf, with_labels);
      nBestXmlItem["fvals"] = xmlrpc_c::value_string(buf.str());
      nBestXmlItem["scores"] = PackScores(*path->GetScoreBreakdown());
    }

    // weighted score
    nBestXmlItem["totalScore"] = xmlrpc_c::value_double(path->GetFutureScore());
    nBestXml.push_back(xmlrpc_c::value_struct(nBestXmlItem));
  }
  retData["nbest"] = xmlrpc_c::value_array(nBestXml);
}

void
TranslationRequest::
insertTranslationOptions(Moses::Manager& manager,
                         std::map<std::string, xmlrpc_c::value>& retData)
{
  std::vector<Moses::FactorType> const& ofactor_order = options()->output.factor_order;
  
  const TranslationOptionCollection* toptsColl = manager.getSntTranslationOptions();
  vector<xmlrpc_c::value> toptsXml;
  size_t const stop = toptsColl->GetSource().GetSize();
  TranslationOptionList const* tol;
  for (size_t s = 0 ; s < stop ; ++s) {
    for (size_t e=s;(tol=toptsColl->GetTranslationOptionList(s,e))!=NULL;++e) {
      BOOST_FOREACH(TranslationOption const* topt, *tol) {
        std::map<std::string, xmlrpc_c::value> toptXml;
        TargetPhrase const& tp = topt->GetTargetPhrase();
        std::string tphrase = tp.GetStringRep(ofactor_order);
        toptXml["phrase"] = xmlrpc_c::value_string(tphrase);
        toptXml["fscore"] = xmlrpc_c::value_double(topt->GetFutureScore());
        toptXml["start"]  = xmlrpc_c::value_int(s);
        toptXml["end"]    = xmlrpc_c::value_int(e);
        vector<xmlrpc_c::value> scoresXml;
        const std::valarray<FValue> &scores
	  = topt->GetScoreBreakdown().getCoreFeatures();
        for (size_t j = 0; j < scores.size(); ++j)
          scoresXml.push_back(xmlrpc_c::value_double(scores[j]));
        toptXml["scores"] = xmlrpc_c::value_array(scoresXml);
	ostringstream buf;
	topt->GetScoreBreakdown().OutputAllFeatureScores(buf, true);
	toptXml["labelledScores"] = PackScores(topt->GetScoreBreakdown());
        toptsXml.push_back(xmlrpc_c::value_struct(toptXml));
      }
    }
  }
  retData["topt"] = xmlrpc_c::value_array(toptsXml);
}

TranslationRequest::
TranslationRequest(xmlrpc_c::paramList const& paramList,
                   boost::condition_variable& cond, boost::mutex& mut)
  : m_cond(cond), m_mutex(mut), m_done(false), m_paramList(paramList)
  , m_session_id(0)
{ 

}

bool
check(std::map<std::string, xmlrpc_c::value> const& param, 
      std::string const key)
{
  std::map<std::string, xmlrpc_c::value>::const_iterator m = param.find(key);
  if(m == param.end()) return false;

  if (m->second.type() == xmlrpc_c::value::TYPE_BOOLEAN)
    return xmlrpc_c::value_boolean(m->second);

  std::string val = string(xmlrpc_c::value_string(m->second));
  if(val == "true" || val == "True" || val == "TRUE" || val == "1") return true;
  return false;
}

void
TranslationRequest::
parse_request(std::map<std::string, xmlrpc_c::value> const& params)
{
  // parse XMLRPC request
  m_paramList.verifyEnd(1); // ??? UG

  typedef std::map<std::string, xmlrpc_c::value> params_t;
  params_t::const_iterator si;

  si = params.find("session-id");
  if (si != params.end()) 
    {
      m_session_id = xmlrpc_c::value_int(si->second);
      Session const& S = m_translator->get_session(m_session_id);
      m_scope = S.scope;
      m_session_id = S.id;
    } 
  else
    {
      m_session_id = 0;
      m_scope.reset(new Moses::ContextScope);
    }

  boost::shared_ptr<Moses::AllOptions> opts(new Moses::AllOptions(*StaticData::Instance().options()));
  opts->update(params);

  m_withGraphInfo = check(params, "sg");
  if (m_withGraphInfo || opts->nbest.nbest_size > 0) {
    opts->output.SearchGraph = "true";
    opts->nbest.enabled = true;
  }

  m_options = opts;

  // source text must be given, or we don't know what to translate
  si = params.find("text");
  if (si == params.end())
    throw xmlrpc_c::fault("Missing source text", xmlrpc_c::fault::CODE_PARSE);
  m_source_string = xmlrpc_c::value_string(si->second);
  XVERBOSE(1,"Input: " << m_source_string << endl);
  
  m_withTopts           = check(params, "topt");
  m_withScoreBreakdown  = check(params, "add-score-breakdown");
  si = params.find("lambda");
  if (si != params.end()) 
    {
      // muMo = multiModel
      xmlrpc_c::value_array muMoArray = xmlrpc_c::value_array(si->second);
      vector<xmlrpc_c::value> muMoValVec(muMoArray.vectorValueValue());
      vector<float> w(muMoValVec.size());
      for (size_t i = 0; i < muMoValVec.size(); ++i)
	w[i] = xmlrpc_c::value_double(muMoValVec[i]);
      if (w.size() && (si = params.find("model_name")) != params.end()) 
	{
	  string const model_name = xmlrpc_c::value_string(si->second);
	  PhraseDictionaryMultiModel* pdmm
	    = (PhraseDictionaryMultiModel*) FindPhraseDictionary(model_name);
	  pdmm->SetTemporaryMultiModelWeightsVector(w);
	}
    }
  
  si = params.find("context");
  if (si != params.end()) 
    {
      string context = xmlrpc_c::value_string(si->second);
      VERBOSE(1,"CONTEXT " << context);
      m_context.reset(new std::vector<std::string>(1,context));
    }

  si = params.find("context-scope");
  if (si != params.end())
    {

      string context = xmlrpc_c::value_string(si->second);

      string groupSeparator("Moses::ContextScope::GroupSeparator");
      string recordSeparator("Moses::ContextScope::RecordSeparator");

      // Here, we assume that any XML-RPC value
      //       associated with the key "context-scope"
      //       has the following format:
      //
      // FeatureFunctionName followed by recordSeparator
      //                     followed by the value of interest
      //                     followed by groupSeparator
      //
      // In the following code, the value of interest will be stored
      //        in contextScope under the key FeatureFunctionName,
      //        where FeatureFunctionName is the actual name of the feature function

      boost::shared_ptr<Moses::ContextScope> contextScope = GetScope();

      BOOST_FOREACH(string group, TokenizeMultiCharSeparator(context, groupSeparator)) {

	vector<string> record = TokenizeMultiCharSeparator(group, recordSeparator);

	// Use the feature function whose name is record[0] as a key
	FeatureFunction& ff = Moses::FeatureFunction::FindFeatureFunction(record[0]);
	void const* key = static_cast<void const*>(&ff);

	// Store (in the context scope) record[1] as the value associated with that key
	boost::shared_ptr<string> value = contextScope->get<string>(key,true);
	value->replace(value->begin(), value->end(), record[1]);

      }
    }

  // Report alignment info if Moses config says to or if XML request says to
  m_withAlignInfo = options()->output.ReportSegmentation || check(params, "align");

  // Report word alignment info if Moses config says to or if XML request says to
  m_withWordAlignInfo = options()->output.PrintAlignmentInfo || check(params, "word-align");

  si = params.find("weights");
  if (si != params.end())
    {

      boost::unordered_map<string, FeatureFunction*> map;
      {
	const vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
	BOOST_FOREACH(FeatureFunction* const& ff, ffs) {
	  map[ff->GetScoreProducerDescription()] = ff;
	}
      }

      string allValues = xmlrpc_c::value_string(si->second);

      BOOST_FOREACH(string values, TokenizeMultiCharSeparator(allValues, "\t")) {

	vector<string> record = TokenizeMultiCharSeparator(values, "=");

	if (record.size() == 2) {
	  string featureName = record[0];
	  string featureWeights = record[1];

	  boost::unordered_map<string, FeatureFunction*>::iterator ffi = map.find(featureName);

	  if (ffi != map.end()) {
	    FeatureFunction* ff = ffi->second;

	    size_t prevNumWeights = ff->GetNumScoreComponents();

	    vector<float> ffWeights;
	    BOOST_FOREACH(string weight, TokenizeMultiCharSeparator(featureWeights, " ")) {
	      ffWeights.push_back(Scan<float>(weight));
	    }

	    if (ffWeights.size() == ff->GetNumScoreComponents()) {

	      // XXX: This is NOT thread-safe
	      Moses::StaticData::InstanceNonConst().SetWeights(ff, ffWeights);
	      VERBOSE(1, "WARNING: THIS IS NOT THREAD-SAFE!\tUpdating weights for " << featureName << " to " << featureWeights << "\n");

	    } else {
	      TRACE_ERR("ERROR: Unable to update weights for " << featureName << " because " << ff->GetNumScoreComponents() << " weights are required but only " << ffWeights.size() << " were provided\n");
	    }

	  } else {
	    TRACE_ERR("ERROR: No FeatureFunction with name " << featureName << ", no weight update\n");
	  }

	} else {
	  TRACE_ERR("WARNING: XML-RPC weights update was improperly formatted:\t" << values << "\n");
	}

      }

    }


  // // biased sampling for suffix-array-based sampling phrase table?
  // if ((si = params.find("bias")) != params.end())
  //   {
  // 	std::vector<xmlrpc_c::value> tmp
  // 	  = xmlrpc_c::value_array(si->second).cvalue();
  // 	for (size_t i = 1; i < tmp.size(); i += 2)
  // 	  m_bias[xmlrpc_c::value_int(tmp[i-1])] = xmlrpc_c::value_double(tmp[i]);
  //   }
  if (is_syntax(m_options->search.algo)) {
    m_source.reset(new Sentence(m_options,0,m_source_string));
  } else {
    m_source.reset(new Sentence(m_options,0,m_source_string));
  }
	interpret_dlt();
} // end of Translationtask::parse_request()


void
TranslationRequest::
run_chart_decoder()
{
  Moses::ChartManager manager(this->self());
  manager.Decode();

  const Moses::ChartHypothesis *hypo = manager.GetBestHypothesis();
  ostringstream out;
  if (hypo) outputChartHypo(out,hypo);

  m_target_string = out.str();
  m_retData["text"] = xmlrpc_c::value_string(m_target_string);

  if (m_withGraphInfo) {
    std::ostringstream sgstream;
    manager.OutputSearchGraphMoses(sgstream);
    m_retData["sg"] =  xmlrpc_c::value_string(sgstream.str());
  }
} // end of TranslationRequest::run_chart_decoder()

void
TranslationRequest::
pack_hypothesis(const Moses::Manager& manager, 
		vector<Hypothesis const* > const& edges, string const& key,
                map<string, xmlrpc_c::value> & dest) const
{
  // target string
  ostringstream target;
  BOOST_REVERSE_FOREACH(Hypothesis const* e, edges) {
    manager.OutputSurface(target, *e); 
  }
  XVERBOSE(1, "BEST TRANSLATION: " << *(manager.GetBestHypothesis()) 
	   << std::endl);
  dest[key] = xmlrpc_c::value_string(target.str());

  if (m_withAlignInfo) {
  //  if (options()->output.ReportSegmentation) {
    // phrase alignment, if requested

    vector<xmlrpc_c::value> p_aln;
    BOOST_REVERSE_FOREACH(Hypothesis const* e, edges)
      add_phrase_aln_info(*e, p_aln);
    dest["align"] = xmlrpc_c::value_array(p_aln);
  }

  if (m_withWordAlignInfo) {
    //if (options()->output.PrintAlignmentInfo) { 
    // word alignment, if requested
    vector<xmlrpc_c::value> w_aln;
    BOOST_REVERSE_FOREACH(Hypothesis const* e, edges)
      e->OutputLocalWordAlignment(w_aln);
    dest["word-align"] = xmlrpc_c::value_array(w_aln);
  }
}

void
TranslationRequest::
pack_hypothesis(const Moses::Manager& manager, Hypothesis const* h, string const& key,
                map<string, xmlrpc_c::value>& dest) const
{
  using namespace std;
  vector<Hypothesis const*> edges;
  for (; h; h = h->GetPrevHypo())
    edges.push_back(h);
  pack_hypothesis(manager, edges, key, dest);
}


void
TranslationRequest::
run_phrase_decoder()
{
  Manager manager(this->self());
  manager.Decode();
  pack_hypothesis(manager, manager.GetBestHypothesis(), "text", m_retData);
  if (m_session_id)
    m_retData["session-id"] = xmlrpc_c::value_int(m_session_id);
  
  if (m_withGraphInfo) insertGraphInfo(manager,m_retData);
  if (m_withTopts) insertTranslationOptions(manager,m_retData);
  if (m_options->nbest.nbest_size) outputNBest(manager, m_retData);

}
}
