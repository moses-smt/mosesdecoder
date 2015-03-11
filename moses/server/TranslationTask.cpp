#include "TranslationTask.h"

namespace MosesServer
{
  using namespace Moses;
  using namespace std;

  virtual void 
  TranslationTask::
  Run() 
  {
    parse_request();
      
    StaticData cosnt& SD = StaticData::Instance();
      
    //Make sure alternative paths are retained, if necessary
    if (m_withGraphInfo || m_nbestSize>0) 
      // why on earth is this a global variable? Is this even thread-safe???? UG
      (const_cast<StaticData&>(staticData)).SetOutputSearchGraph(true);
      
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
  TranslationTask::
  add_phrase_aln_info(Hypothesis const& h, vector<xmlrpc_c::value>& aInfo)
  {
    if (!m_oopt.withPhraseAlignment) return;
    WordsRange const& trg = h.GetCurrTargetWordsRange();
    WordsRange const& src = h.GetCurrSourceWordsRange();
    
    std::map<std::string, xmlrpc_c::value> pAlnInfo;
    pAlnInfo["tgt-start"] = xmlrpc_c::value_int(trg.GetStartPos());
    pAlgInfo["src-start"] = xmlrpc_c::value_int(src.GetStartPos());
    pAlnInfo["src-end"]   = xmlrpc_c::value_int(src.GetEndPos());
    aInfo.push_back(xmlrpc_c::value_struct(phraseAlignInfo));
  }
  
  void 
  TranslationTask::
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
  TranslationTask::
  compareSearchGraphNode(const SearchGraphNode& a, const SearchGraphNode b) 
  { return a.hypo->GetId() < b.hypo->GetId(); }

  void 
  TranslationTask::
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
    BOOST_FOREACH(SearchGraphNode const& n, searchGraph)
      {
	map<string, xmlrpc_c::value> x; // search graph xml node
	x["forward"] = value_double(n.forward);
	x["fscore"] = value_double(n.fscore);
	const Hypothesis* hypo = n.hypo;
	x["hyp"] = value_int(hypo->GetId());
	x["stack"] = value_int(hypo->GetWordsBitmap().GetNumWordsCovered());
	if (hypo->GetId() != 0) 
	  {
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
    retData["sg"] = value_array(searchGraphXml);
  }

  void 
  TranslationTask::
  output_phrase(ostream& out, Phrase const& phrase)
  {
    if (!m_reportAllFactors) 
      {
	for (size_t i = 0 ; i < phrase.GetSize(); ++i) 
	  out << *phrase.GetFactor(i, 0) << " ";
      }
    else out << phrase;
  }
  
  void 
  TranslationTask::
  outputNBest(const Manager& manager, map<string, xmlrpc_c::value>& retData)
  {
    TrellisPathList nBestList;
    vector<xmlrpc_c::value> nBestXml;
    manager.CalcNBest(m_nbestSize, nBestList, m_nbestDistinct);
    
    BOOST_FOREACH(TrellisPath const& path, nBestList)
      {
	vector<const Hypothesis *> const& E = path.GetEdges();
	if (!E.size()) continue;
	std::map<std::string, xmlrpc_c::value> nBestXmlItem;
	pack_hypothesis(E, "hyp", nBestXmlItem);
	if (m_withScoreBreakdown)
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
    retData["nbest"] = xmlrpc_c::value_array(nBestXml);
  }
  
  void 
  TranslationTask::
  insertTranslationOptions(Manager& manager, 
			   map<string, xmlrpc_c::value>& retData) 
  {
    const TranslationOptionCollection* toptsColl 
      = manager.getSntTranslationOptions();
    vector<xmlrpc_c::value> toptsXml;
    size_t const stop = toptsColl->GetSource().GetSize();
    TranslationOptionList const* tol;
    for (size_t s = 0 ; s < stop ; ++s) 
      {
	for (size_t e = s; 
	     (tol = toptsColl->GetTranslationOptionList(s,e)) != NULL;
	     ++e)
	  {
	    BOOST_FOREACH(TranslationOption const* topt, *tol)
	      {
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
  
  TranslationTask::
  TranslationTask(xmlrpc_c::paramList const& paramList,
		  boost::condition_variable& cond, boost::mutex& mut)
    : m_paramList(paramList), m_cond(cond), m_mutex(mutex), m_done(false)
  { }

  void
  TranslationTask::
  parse_request(std::map<std::string, xmlrpc_c::value> const& req)
  { // parse XMLRPC request
    // params_t const params = m_paramList.getStruct(0);
    m_paramList.verifyEnd(1); // ??? UG
    
    // source text must be given, or we don't know what to translate
    params_t::const_iterator si = params.find("text");
    if (si == params.end()) 
      throw xmlrpc_c::fault("Missing source text", xmlrpc_c::fault::CODE_PARSE);
    m_source = xmlrpc_c::value_string(si->second);
    XVERBOSE(1,"Input: " << source << endl);
    
    m_oopt.init(req);
    m_withPhraseAlignment = check(params, "align");
    m_withWordAlignment   = check(params, "word-align");
    m_withSearchGraph     = check(params, "sg");
    m_withTransOpts       = check(params, "topt");
    m_outputAllFactors    = check(params, "report-all-factors");
    m_nbestDistinct       = check(params, "nbest-distinct");
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
	    // PhraseDictionaryMultiModel* pdmm 
	    // = (PhraseDictionaryMultiModel*) FindPhraseDictionary(model_name);
	    PhraseDictionaryMultiModel* pdmm = FindPhraseDictionary(model_name);
	    pdmm->SetTemporaryMultiModelWeightsVector(w);
	  }
      }
    
    // biased sampling for suffix-array-based sampling phrase table?
    if ((si = params.find("bias")) != params.end())
      { 
	std::vector<xmlrpc_c::value> tmp 
	  = xmlrpc_c::value_array(si->second).cvalue();
	for (size_t i = 1; i < tmp.size(); i += 2)
	  m_bias[xmlrpc_c::value_int(tmp[i-1])] = xmlrpc_c::value_double(tmp[i]);
      }
  } // end of Translationtask::parse_request()


  void
  TranslationTask::
  run_chart_decoder()
  {
    TreeInput tinput; 
    tinput.Read(stringstream(m_source + "\n"), 
		StaticData::Instance().GetInputFactorOrder());
    
    ChartManager manager(tinput);
    manager.Decode();
    
    const ChartHypothesis *hypo = manager.GetBestHypothesis();
    ostringstream out;
    outputChartHypo(out,hypo);
    
    m_target = out.str();
    m_retData["text"] = xmlrpc_c::value_string(m_target);
    
    if (m_addGraphInfo) 
      {
	std::ostringstream sgstream;
	manager.OutputSearchGraphMoses(sgstream);
	m_retData["sg"] =  xmlrpc_c::value_string(sgstream.str());
      }
  } // end of TranslationTask::run_chart_decoder()
  
  void
  TranslationTask::
  pack_hypothesis(vector<Hypothesis const* > const& edges, string const& key,
		  map<string, xmlrpc_c::value> & dest) const
  {
    // target string
    ostringstream target;
    BOOST_REVERSE_FOREACH(Hypothesis const* e, edges)
      output_phrase(target, e->GetCurrTargetPhrase());
    dest[key] = xmlrpc_c::value_string(target.str());
  
    if (m_addAlignmentInfo)
      { // phrase alignment, if requested

	vector<xmlrpc_c::value> p_aln;
	BOOST_REVERSE_FOREACH(Hypothesis const* e, edges)
	  add_phrase_aln_info(*e, p_aln);
	dest["align"] = xmlrpc_c::value_array(p_aln);
      }

    if (m_addWordAlignmentInfo)
      { // word alignment, if requested
	vector<xmlrpc_c::value> w_aln;
	BOOST_FOREACH(Hypothesis const* e, edges)
	  e->OutputLocalWordAligment(w_aln);
	dest["word-align"] = xmlrpc_c::value_array(w_aln);
      }
  }

  void
  TranslationTask::
  pack_hypothesis(Hypothesis const* h, string const& key,
		  map<string, xmlrpc_c::value>& dest) const
  {
    using namespace std;
    vector<Hypothesis const*> edges;
    for (;h; h = h->GetPrevHypo())
      edges.push_back(h);
    pack_hypothesis(edges, key, dest);
  }


  void
  TranslationTask::
  run_phrase_decoder()
  {
    Manager manager(Sentence(0, m_source));
    // if (m_bias.size()) manager.SetBias(&m_bias);
    manager.Decode();
    
    pack_hypothesis(manager.GetBestHypothesis(), m_retData);
    
    if (m_addGraphInfo) insertGraphInfo(manager,m_retData);
    if (m_addTopts) insertTranslationOptions(manager,m_retData);
    if (m_nbestSize) outputNBest(manager, m_retData);
    
    (const_cast<StaticData&>(staticData)).SetOutputSearchGraph(false); 
    // WTF? one more reason not to have this as global variable! --- UG
    
  }
}
