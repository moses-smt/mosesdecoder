#include "TranslationRequest.h"
#include <boost/foreach.hpp>

namespace MosesServer
{
using namespace std;
using Moses::Hypothesis;
using Moses::StaticData;
using Moses::WordsRange;
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

boost::shared_ptr<TranslationRequest>
TranslationRequest::
create(xmlrpc_c::paramList const& paramList,
       boost::condition_variable& cond,
       boost::mutex& mut)
{
  boost::shared_ptr<TranslationRequest> ret;
  ret.reset(new TranslationRequest(paramList,cond, mut));
  ret->m_self = ret;
  return ret;
}

void
TranslationRequest::
Run()
{
  parse_request(m_paramList.getStruct(0));

  Moses::StaticData const& SD = Moses::StaticData::Instance();

  //Make sure alternative paths are retained, if necessary
  if (m_withGraphInfo || m_nbestSize>0)
    // why on earth is this a global variable? Is this even thread-safe???? UG
    (const_cast<Moses::StaticData&>(SD)).SetOutputSearchGraph(true);

  std::stringstream out, graphInfo, transCollOpts;

  if (SD.IsSyntax())
    run_chart_decoder();
  else
    run_phrase_decoder();

  XVERBOSE(1,"Output: " << out.str() << endl);
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
  WordsRange const& trg = h.GetCurrTargetWordsRange();
  WordsRange const& src = h.GetCurrSourceWordsRange();

  std::map<std::string, xmlrpc_c::value> pAlnInfo;
  pAlnInfo["tgt-start"] = xmlrpc_c::value_int(trg.GetStartPos());
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
      x["out"] = value_string(hypo->GetCurrTargetPhrase().GetStringRep(StaticData::Instance().GetOutputFactorOrder()));
    }
    searchGraphXml.push_back(value_struct(x));
  }
  retData["sg"] = xmlrpc_c::value_array(searchGraphXml);
}

void
TranslationRequest::
output_phrase(ostream& out, Phrase const& phrase) const
{
  if (!m_reportAllFactors) {
    for (size_t i = 0 ; i < phrase.GetSize(); ++i)
      out << *phrase.GetFactor(i, 0) << " ";
  } else out << phrase;
}

void
TranslationRequest::
outputNBest(const Manager& manager, map<string, xmlrpc_c::value>& retData)
{
  TrellisPathList nBestList;
  vector<xmlrpc_c::value> nBestXml;
  manager.CalcNBest(m_nbestSize, nBestList, m_nbestDistinct);

  BOOST_FOREACH(Moses::TrellisPath const* path, nBestList) {
    vector<const Hypothesis *> const& E = path->GetEdges();
    if (!E.size()) continue;
    std::map<std::string, xmlrpc_c::value> nBestXmlItem;
    pack_hypothesis(E, "hyp", nBestXmlItem);
    if (m_withScoreBreakdown) {
      // should the score breakdown be reported in a more structured manner?
      ostringstream buf;
      path->GetScoreBreakdown()->OutputAllFeatureScores(buf);
      nBestXmlItem["fvals"] = xmlrpc_c::value_string(buf.str());
    }

    // weighted score
    nBestXmlItem["totalScore"] = xmlrpc_c::value_double(path->GetTotalScore());
    nBestXml.push_back(xmlrpc_c::value_struct(nBestXmlItem));
  }
  retData["nbest"] = xmlrpc_c::value_array(nBestXml);
}

void
TranslationRequest::
insertTranslationOptions(Moses::Manager& manager,
                         std::map<std::string, xmlrpc_c::value>& retData)
{
  const TranslationOptionCollection* toptsColl
  = manager.getSntTranslationOptions();
  vector<xmlrpc_c::value> toptsXml;
  size_t const stop = toptsColl->GetSource().GetSize();
  TranslationOptionList const* tol;
  for (size_t s = 0 ; s < stop ; ++s) {
    for (size_t e = s;
         (tol = toptsColl->GetTranslationOptionList(s,e)) != NULL;
         ++e) {
      BOOST_FOREACH(TranslationOption const* topt, *tol) {
        std::map<std::string, xmlrpc_c::value> toptXml;
        TargetPhrase const& tp = topt->GetTargetPhrase();
        StaticData const& GLOBAL = StaticData::Instance();
        std::string tphrase = tp.GetStringRep(GLOBAL.GetOutputFactorOrder());
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
        toptsXml.push_back(xmlrpc_c::value_struct(toptXml));
      }
    }
  }
  retData["topt"] = xmlrpc_c::value_array(toptsXml);
}

bool
check(std::map<std::string, xmlrpc_c::value> const& params, std::string const key)
{
  std::map<std::string, xmlrpc_c::value>::const_iterator m;
  return (params.find(key) != params.end());
}

TranslationRequest::
TranslationRequest(xmlrpc_c::paramList const& paramList,
                   boost::condition_variable& cond, boost::mutex& mut)
  : m_cond(cond), m_mutex(mut), m_done(false), m_paramList(paramList)
  , m_nbestSize(0)
{ }

void
TranslationRequest::
parse_request(std::map<std::string, xmlrpc_c::value> const& params)
{
  // parse XMLRPC request
  // params_t const params = m_paramList.getStruct(0);
  m_paramList.verifyEnd(1); // ??? UG

  // source text must be given, or we don't know what to translate
  typedef std::map<std::string, xmlrpc_c::value> params_t;
  params_t::const_iterator si = params.find("text");
  if (si == params.end())
    throw xmlrpc_c::fault("Missing source text", xmlrpc_c::fault::CODE_PARSE);
  m_source_string = xmlrpc_c::value_string(si->second);
  XVERBOSE(1,"Input: " << m_source_string << endl);

  m_withAlignInfo       = check(params, "align");
  m_withWordAlignInfo   = check(params, "word-align");
  m_withGraphInfo       = check(params, "sg");
  m_withTopts           = check(params, "topt");
  m_reportAllFactors    = check(params, "report-all-factors");
  m_nbestDistinct       = check(params, "nbest-distinct");
  m_withScoreBreakdown  = check(params, "add-score-breakdown");
  m_source.reset(new Sentence(0,m_source_string));
  si = params.find("lambda");
  if (si != params.end()) {
    // muMo = multiModel
    xmlrpc_c::value_array muMoArray = xmlrpc_c::value_array(si->second);
    vector<xmlrpc_c::value> muMoValVec(muMoArray.vectorValueValue());
    vector<float> w(muMoValVec.size());
    for (size_t i = 0; i < muMoValVec.size(); ++i)
      w[i] = xmlrpc_c::value_double(muMoValVec[i]);
    if (w.size() && (si = params.find("model_name")) != params.end()) {
      string const model_name = xmlrpc_c::value_string(si->second);
      PhraseDictionaryMultiModel* pdmm
      = (PhraseDictionaryMultiModel*) FindPhraseDictionary(model_name);
      // Moses::PhraseDictionaryMultiModel* pdmm
      // = FindPhraseDictionary(model_name);
      pdmm->SetTemporaryMultiModelWeightsVector(w);
    }
  }

  si = params.find("nbest");
  if (si != params.end())
    m_nbestSize = xmlrpc_c::value_int(si->second);


  // // biased sampling for suffix-array-based sampling phrase table?
  // if ((si = params.find("bias")) != params.end())
  //   {
  // 	std::vector<xmlrpc_c::value> tmp
  // 	  = xmlrpc_c::value_array(si->second).cvalue();
  // 	for (size_t i = 1; i < tmp.size(); i += 2)
  // 	  m_bias[xmlrpc_c::value_int(tmp[i-1])] = xmlrpc_c::value_double(tmp[i]);
  //   }
} // end of Translationtask::parse_request()


void
TranslationRequest::
run_chart_decoder()
{
  Moses::TreeInput tinput;
  istringstream buf(m_source_string + "\n");
  tinput.Read(buf, StaticData::Instance().GetInputFactorOrder());

  Moses::ChartManager manager(this->self());
  manager.Decode();

  const Moses::ChartHypothesis *hypo = manager.GetBestHypothesis();
  ostringstream out;
  outputChartHypo(out,hypo);

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
pack_hypothesis(vector<Hypothesis const* > const& edges, string const& key,
                map<string, xmlrpc_c::value> & dest) const
{
  // target string
  ostringstream target;
  BOOST_REVERSE_FOREACH(Hypothesis const* e, edges)
  output_phrase(target, e->GetCurrTargetPhrase());
  dest[key] = xmlrpc_c::value_string(target.str());

  if (m_withAlignInfo) {
    // phrase alignment, if requested

    vector<xmlrpc_c::value> p_aln;
    BOOST_REVERSE_FOREACH(Hypothesis const* e, edges)
    add_phrase_aln_info(*e, p_aln);
    dest["align"] = xmlrpc_c::value_array(p_aln);
  }

  if (m_withWordAlignInfo) {
    // word alignment, if requested
    vector<xmlrpc_c::value> w_aln;
    BOOST_FOREACH(Hypothesis const* e, edges)
    e->OutputLocalWordAlignment(w_aln);
    dest["word-align"] = xmlrpc_c::value_array(w_aln);
  }
}

void
TranslationRequest::
pack_hypothesis(Hypothesis const* h, string const& key,
                map<string, xmlrpc_c::value>& dest) const
{
  using namespace std;
  vector<Hypothesis const*> edges;
  for (; h; h = h->GetPrevHypo())
    edges.push_back(h);
  pack_hypothesis(edges, key, dest);
}


void
TranslationRequest::
run_phrase_decoder()
{
  Manager manager(this->self());
  // if (m_bias.size()) manager.SetBias(&m_bias);
  manager.Decode();

  pack_hypothesis(manager.GetBestHypothesis(), "text", m_retData);

  if (m_withGraphInfo) insertGraphInfo(manager,m_retData);
  if (m_withTopts) insertTranslationOptions(manager,m_retData);
  if (m_nbestSize) outputNBest(manager, m_retData);

  (const_cast<StaticData&>(Moses::StaticData::Instance()))
  .SetOutputSearchGraph(false);
  // WTF? one more reason not to have this as global variable! --- UG

}
}
