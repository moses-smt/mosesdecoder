// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <ctime>
#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "Parameter.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "UserMessage.h"

using namespace std;

namespace Moses
{
/** define allowed parameters */
Parameter::Parameter()
{
  AddParam("beam-threshold", "b", "threshold for threshold pruning");
  AddParam("config", "f", "location of the configuration file");
  AddParam("continue-partial-translation", "cpt", "start from nonempty hypothesis");
  AddParam("decoding-graph-backoff", "dpb", "only use subsequent decoding paths for unknown spans of given length");
  AddParam("dlm-model", "Order, factor and vocabulary file for discriminative LM. Use * for filename to indicate unlimited vocabulary.");
  AddParam("drop-unknown", "du", "drop unknown words instead of copying them");
  AddParam("disable-discarding", "dd", "disable hypothesis discarding");
  AddParam("factor-delimiter", "fd", "specify a different factor delimiter than the default");
  AddParam("generation-file", "location and properties of the generation table");
  AddParam("global-lexical-file", "gl", "discriminatively trained global lexical translation model file");
  AddParam("glm-feature", "discriminatively trained global lexical translation feature, sparse producer");
  AddParam("input-factors", "list of factors in the input");
  AddParam("input-file", "i", "location of the input file to be translated");
  AddParam("inputtype", "text (0), confusion network (1), word lattice (2) (default = 0)");
  AddParam("labeled-n-best-list", "print out labels for each weight type in n-best list. default is true");
  AddParam("lmodel-file", "location and properties of the language models");
  AddParam("lmodel-dub", "dictionary upper bounds of language models");
  AddParam("lmodel-oov-feature", "add language model oov feature, one per model");
  AddParam("mapping", "description of decoding steps");
  AddParam("max-partial-trans-opt", "maximum number of partial translation options per input span (during mapping steps)");
  AddParam("max-trans-opt-per-coverage", "maximum number of translation options per input span (after applying mapping steps)");
  AddParam("max-phrase-length", "maximum phrase length (default 20)");
  AddParam("n-best-list", "file and size of n-best-list to be generated; specify - as the file in order to write to STDOUT");
  AddParam("lattice-samples", "generate samples from lattice, in same format as nbest list. Uses the file and size arguments, as in n-best-list");
  AddParam("n-best-factor", "factor to compute the maximum number of contenders (=factor*nbest-size). value 0 means infinity, i.e. no threshold. default is 0");
  AddParam("print-all-derivations", "to print all derivations in search graph");
  AddParam("output-factors", "list of factors in the output");
  AddParam("phrase-drop-allowed", "da", "if present, allow dropping of source words"); //da = drop any (word); see -du for comparison
  AddParam("report-all-factors", "report all factors in output, not just first");
  AddParam("report-all-factors-in-n-best", "Report all factors in n-best-lists. Default is false");
#ifdef HAVE_SYNLM
	AddParam("slmodel-file", "location of the syntactic language model file(s)");
	AddParam("weight-slm", "slm", "weight(s) for syntactic language model");
	AddParam("slmodel-factor", "factor to use with syntactic language model");
	AddParam("slmodel-beam", "beam width to use with syntactic language model's parser");
#endif
  AddParam("stack", "s", "maximum stack size for histogram pruning");
  AddParam("stack-diversity", "sd", "minimum number of hypothesis of each coverage in stack (default 0)");
  AddParam("threads","th", "number of threads to use in decoding (defaults to single-threaded)");
	AddParam("translation-details", "T", "for each best hypothesis, report translation details to the given file");
	AddParam("ttable-file", "location and properties of the translation tables");
	AddParam("ttable-limit", "ttl", "maximum number of translation table entries per input phrase");
	AddParam("translation-option-threshold", "tot", "threshold for translation options relative to best for input phrase");
	AddParam("early-discarding-threshold", "edt", "threshold for constructing hypotheses based on estimate cost");
	AddParam("verbose", "v", "verbosity level of the logging");
  AddParam("references", "Reference file(s) - used for bleu score feature");
  AddParam("weight-bl", "bl", "weight for bleu score feature");
	AddParam("weight-d", "d", "weight(s) for distortion (reordering components)");
	AddParam("weight-dlm", "dlm", "weight for discriminative LM feature function (on top of sparse weights)");
  AddParam("weight-lr", "lr", "weight(s) for lexicalized reordering, if not included in weight-d");
	AddParam("weight-generation", "g", "weight(s) for generation components");
	AddParam("weight-i", "I", "weight(s) for word insertion - used for parameters from confusion network and lattice input links");
	AddParam("weight-l", "lm", "weight(s) for language models");
	AddParam("weight-lex", "lex", "weight for global lexical model");
	AddParam("weight-glm", "glm", "weight for global lexical feature, sparse producer");
	AddParam("weight-wt", "wt", "weight for word translation feature");
	AddParam("weight-pp", "pp", "weight for phrase pair feature");
	AddParam("weight-pb", "pb", "weight for phrase boundary feature");
	AddParam("weight-t", "tm", "weights for translation model components");
	AddParam("weight-w", "w", "weight for word penalty");
	AddParam("weight-u", "u", "weight for unknown word penalty");
	AddParam("weight-e", "e", "weight for word deletion"); 
  AddParam("weight-t-multimodel", "tmo", "weights for multi-model mode");
  AddParam("weight-file", "wf", "feature weights file. Do *not* put weights for 'core' features in here - they go in moses.ini");
	AddParam("output-factors", "list if factors in the output");
	AddParam("cache-path", "?");
	AddParam("distortion-limit", "dl", "distortion (reordering) limit in maximum number of words (0 = monotone, -1 = unlimited)");	
	AddParam("monotone-at-punctuation", "mp", "do not reorder over punctuation");
	AddParam("distortion-file", "source factors (0 if table independent of source), target factors, location of the factorized/lexicalized reordering tables");
 	AddParam("distortion", "configurations for each factorized/lexicalized reordering model.");
 	AddParam("early-distortion-cost", "edc", "include estimate of distortion cost yet to be incurred in the score [Moore & Quirk 2007]. Default is no");
	AddParam("xml-input", "xi", "allows markup of input with desired translations and probabilities. values can be 'pass-through' (default), 'inclusive', 'exclusive', 'ignore'");
  AddParam("xml-brackets", "xb", "specify strings to be used as xml tags opening and closing, e.g. \"{{ }}\" (default \"< >\"). Avoid square brackets because of configuration file format. Valid only with text input mode" );
 	AddParam("minimum-bayes-risk", "mbr", "use miminum Bayes risk to determine best translation");
  AddParam("lminimum-bayes-risk", "lmbr", "use lattice miminum Bayes risk to determine best translation");
  AddParam("mira", "do mira training");
  AddParam("consensus-decoding", "con", "use consensus decoding (De Nero et. al. 2009)");
  AddParam("mbr-size", "number of translation candidates considered in MBR decoding (default 200)");
  AddParam("mbr-scale", "scaling factor to convert log linear score probability in MBR decoding (default 1.0)");
  AddParam("lmbr-thetas", "theta(s) for lattice mbr calculation");
  AddParam("lmbr-pruning-factor", "average number of nodes/word wanted in pruned lattice");
  AddParam("lmbr-p", "unigram precision value for lattice mbr");
  AddParam("lmbr-r", "ngram precision decay value for lattice mbr");
  AddParam("lmbr-map-weight", "weight given to map solution when doing lattice MBR (default 0)");
  AddParam("lattice-hypo-set", "to use lattice as hypo set during lattice MBR");
  AddParam("clean-lm-cache", "clean language model caches after N translations (default N=1)");
  AddParam("use-persistent-cache", "cache translation options across sentences (default true)");
  AddParam("persistent-cache-size", "maximum size of cache for translation options (default 10,000 input phrases)");
  AddParam("recover-input-path", "r", "(conf net/word lattice only) - recover input path corresponding to the best translation");
  AddParam("output-word-graph", "owg", "Output stack info as word graph. Takes filename, 0=only hypos in stack, 1=stack + nbest hypos");
  AddParam("time-out", "seconds after which is interrupted (-1=no time-out, default is -1)");
  AddParam("output-search-graph", "osg", "Output connected hypotheses of search into specified filename");
  AddParam("output-search-graph-extended", "osgx", "Output connected hypotheses of search into specified filename, in extended format");
  AddParam("unpruned-search-graph", "usg", "When outputting chart search graph, do not exclude dead ends. Note: stack pruning may have eliminated some hypotheses");
  AddParam("output-search-graph-slf", "slf", "Output connected hypotheses of search into specified directory, one file per sentence, in HTK standard lattice format (SLF)");
  AddParam("output-search-graph-hypergraph", "Output connected hypotheses of search into specified directory, one file per sentence, in a hypergraph format (see Kenneth Heafield's lazy hypergraph decoder)");
  AddParam("include-lhs-in-search-graph", "lhssg", "When outputting chart search graph, include the label of the LHS of the rule (useful when using syntax)");
#ifdef HAVE_PROTOBUF
  AddParam("output-search-graph-pb", "pb", "Write phrase lattice to protocol buffer objects in the specified path.");
#endif
	AddParam("cube-pruning-pop-limit", "cbp", "How many hypotheses should be popped for each stack. (default = 1000)");
	AddParam("cube-pruning-diversity", "cbd", "How many hypotheses should be created for each coverage. (default = 0)");
	AddParam("search-algorithm", "Which search algorithm to use. 0=normal stack, 1=cube pruning, 2=cube growing. (default = 0)");
	AddParam("constraint", "Location of the file with target sentences to produce constraining the search");
	AddParam("link-param-count", "Number of parameters on word links when using confusion networks or lattices (default = 1)");
	AddParam("description", "Source language, target language, description");
	AddParam("max-chart-span", "maximum num. of source word chart rules can consume (default 10)");
	AddParam("non-terminals", "list of non-term symbols, space separated");
	AddParam("rule-limit", "a little like table limit. But for chart decoding rules. Default is DEFAULT_MAX_TRANS_OPT_SIZE");
	AddParam("source-label-overlap", "What happens if a span already has a label. 0=add more. 1=replace. 2=discard. Default is 0");
	AddParam("output-hypo-score", "Output the hypo score to stdout with the output string. For search error analysis. Default is false");
	AddParam("unknown-lhs", "file containing target lhs of unknown words. 1 per line: LHS prob");
  AddParam("phrase-pair-feature", "Source and target factors for phrase pair feature");
  AddParam("phrase-boundary-source-feature", "Source factors for phrase boundary feature");
  AddParam("phrase-boundary-target-feature", "Target factors for phrase boundary feature");
  AddParam("phrase-length-feature", "Count features for source length, target length, both of each phrase");
  AddParam("target-word-insertion-feature", "Count feature for each unaligned target word");
  AddParam("source-word-deletion-feature", "Count feature for each unaligned source word");
  AddParam("word-translation-feature", "Count feature for word translation according to word alignment");
  AddParam("report-sparse-features", "Indicate which sparse feature functions should report detailed scores in n-best, instead of aggregate");
  AddParam("cube-pruning-lazy-scoring", "cbls", "Don't fully score a hypothesis until it is popped");
  AddParam("parsing-algorithm", "Which parsing algorithm to use. 0=CYK+, 1=scope-3. (default = 0)");
  AddParam("search-algorithm", "Which search algorithm to use. 0=normal stack, 1=cube pruning, 2=cube growing, 4=stack with batched lm requests (default = 0)");
  AddParam("constraint", "Location of the file with target sentences to produce constraining the search");
  AddParam("link-param-count", "Number of parameters on word links when using confusion networks or lattices (default = 1)");
  AddParam("description", "Source language, target language, description");

  AddParam("max-chart-span", "maximum num. of source word chart rules can consume (default 10)");
  AddParam("non-terminals", "list of non-term symbols, space separated");
  AddParam("rule-limit", "a little like table limit. But for chart decoding rules. Default is DEFAULT_MAX_TRANS_OPT_SIZE");
  AddParam("source-label-overlap", "What happens if a span already has a label. 0=add more. 1=replace. 2=discard. Default is 0");
  AddParam("output-hypo-score", "Output the hypo score to stdout with the output string. For search error analysis. Default is false");
  AddParam("unknown-lhs", "file containing target lhs of unknown words. 1 per line: LHS prob");
  AddParam("translation-systems", "specify multiple translation systems, each consisting of an id, followed by a set of models ids, eg '0 T1 R1 L0'");
  AddParam("show-weights", "print feature weights and exit");
  AddParam("start-translation-id", "Id of 1st input. Default = 0");
  AddParam("text-type", "should be one of dev/devtest/test, used for domain adaptation features");
  AddParam("output-unknowns", "Output the unknown (OOV) words to the given file, one line per sentence");
  
  // Compact phrase table and reordering table.                                                                                  
  AddParam("minlexr-memory", "Load lexical reordering table in minlexr format into memory");                                          
  AddParam("minphr-memory", "Load phrase table in minphr format into memory");

  AddParam("print-alignment-info", "Output word-to-word alignment to standard out, separated from translation by |||. Word-to-word alignments are takne from the phrase table if any. Default is false");
  AddParam("include-segmentation-in-n-best", "include phrasal segmentation in the n-best list. default is false");
  AddParam("print-alignment-info-in-n-best", "Include word-to-word alignment in the n-best list. Word-to-word alignments are takne from the phrase table if any. Default is false");
  AddParam("alignment-output-file", "print output word alignments into given file");
  AddParam("sort-word-alignment", "Sort word alignments for more consistent display. 0=no sort (default), 1=target order");

  AddParam("report-segmentation", "t", "report phrase segmentation in the output");
  AddParam("print-id", "prefix translations with id. Default if false");
  AddParam("print-passthrough", "output the sgml tag <passthrough> without any computation on that. Default is false");
  AddParam("print-passthrough-in-n-best", "output the sgml tag <passthrough> without any computation on that in each entry of the n-best-list. Default is false");


}

Parameter::~Parameter()
{
}

/** initialize a parameter, sub of constructor */
void Parameter::AddParam(const string &paramName, const string &description)
{
  m_valid[paramName] = true;
  m_description[paramName] = description;
}

/** initialize a parameter (including abbreviation), sub of constructor */
void Parameter::AddParam(const string &paramName, const string &abbrevName, const string &description)
{
  m_valid[paramName] = true;
  m_valid[abbrevName] = true;
  m_abbreviation[paramName] = abbrevName;
	m_fullname[abbrevName] = paramName;
  m_description[paramName] = description;
}

/** print descriptions of all parameters */
void Parameter::Explain()
{
  cerr << "Usage:" << endl;
  for(PARAM_STRING::const_iterator iterParam = m_description.begin(); iterParam != m_description.end(); iterParam++) {
    const string paramName = iterParam->first;
    const string paramDescription = iterParam->second;
    cerr <<  "\t-" << paramName;
    PARAM_STRING::const_iterator iterAbbr = m_abbreviation.find( paramName );
    if ( iterAbbr != m_abbreviation.end() )
      cerr <<  " (" << iterAbbr->second << ")";
    cerr <<  ": " << paramDescription << endl;
  }
}

/** check whether an item on the command line is a switch or a value
 * \param token token on the command line to checked **/

bool Parameter::isOption(const char* token)
{
  if (! token) return false;
  std::string tokenString(token);
  size_t length = tokenString.size();
  if (length > 0 && tokenString.substr(0,1) != "-") return false;
  if (length > 1 && tokenString.substr(1,1).find_first_not_of("0123456789") == 0) return true;
  return false;
}

/** load all parameters from the configuration file and the command line switches */
bool Parameter::LoadParam(const string &filePath)
{
  const char *argv[] = {"executable", "-f", filePath.c_str() };
  return LoadParam(3, (char**) argv);
}

/** load all parameters from the configuration file and the command line switches */
bool Parameter::LoadParam(int argc, char* argv[])
{
  // config file (-f) arg mandatory
  string configPath;
  if ( (configPath = FindParam("-f", argc, argv)) == ""
       && (configPath = FindParam("-config", argc, argv)) == "") {
    PrintCredit();
    Explain();

    cerr << endl;    
    UserMessage::Add("No configuration file was specified.  Use -config or -f");
    cerr << endl;
    return false;
  } else {
    if (!ReadConfigFile(configPath)) {
      UserMessage::Add("Could not read "+configPath);
      return false;
    }
  }

  // overwrite parameters with values from switches
  for(PARAM_STRING::const_iterator iterParam = m_description.begin(); iterParam != m_description.end(); iterParam++) {
    const string paramName = iterParam->first;
    OverwriteParam("-" + paramName, paramName, argc, argv);
  }

  // ... also shortcuts
  for(PARAM_STRING::const_iterator iterParam = m_abbreviation.begin(); iterParam != m_abbreviation.end(); iterParam++) {
    const string paramName = iterParam->first;
    const string paramShortName = iterParam->second;
    OverwriteParam("-" + paramShortName, paramName, argc, argv);
  }

  // logging of parameters that were set in either config or switch
  int verbose = 1;
  if (m_setting.find("verbose") != m_setting.end() &&
      m_setting["verbose"].size() > 0)
    verbose = Scan<int>(m_setting["verbose"][0]);
  if (verbose >= 1) { // only if verbose
    TRACE_ERR( "Defined parameters (per moses.ini or switch):" << endl);
    for(PARAM_MAP::const_iterator iterParam = m_setting.begin() ; iterParam != m_setting.end(); iterParam++) {
      TRACE_ERR( "\t" << iterParam->first << ": ");
      for ( size_t i = 0; i < iterParam->second.size(); i++ )
        TRACE_ERR( iterParam->second[i] << " ");
      TRACE_ERR( endl);
    }
  }

  // check for illegal parameters
  bool noErrorFlag = true;
  for (int i = 0 ; i < argc ; i++) {
    if (isOption(argv[i])) {
      string paramSwitch = (string) argv[i];
      string paramName = paramSwitch.substr(1);
      if (m_valid.find(paramName) == m_valid.end()) {
        UserMessage::Add("illegal switch: " + paramSwitch);
        noErrorFlag = false;
      }
    }
  }

  // check if parameters make sense
  return Validate() && noErrorFlag;
}

/** check that parameter settings make sense */
bool Parameter::Validate()
{
  bool noErrorFlag = true;

  PARAM_MAP::const_iterator iterParams;
  for (iterParams = m_setting.begin(); iterParams != m_setting.end(); ++iterParams) {
    const std::string &key = iterParams->first;
    
    if (m_valid.find(key) == m_valid.end())
    {
      UserMessage::Add("Unknown parameter " + key);
      noErrorFlag = false;
    }
  }
  

  // required parameters
  if (m_setting["ttable-file"].size() == 0) {
    UserMessage::Add("No phrase translation table (ttable-file)");
    noErrorFlag = false;
  }

  if (m_setting["lmodel-dub"].size() > 0) {
    if (m_setting["lmodel-file"].size() != m_setting["lmodel-dub"].size()) {
      stringstream errorMsg("");
      errorMsg << "Config and parameters specify "
               << static_cast<int>(m_setting["lmodel-file"].size())
               << " language model files (lmodel-file), but "
               << static_cast<int>(m_setting["lmodel-dub"].size())
               << " LM upperbounds (lmodel-dub)"
               << endl;
      UserMessage::Add(errorMsg.str());
      noErrorFlag = false;
    }
  }

  if (m_setting["lmodel-file"].size() * (m_setting.find("lmodel-oov-feature") != m_setting.end() ? 2 : 1)
         != m_setting["weight-l"].size()) {
    stringstream errorMsg("");
    errorMsg << "Config and parameters specify "
             << static_cast<int>(m_setting["lmodel-file"].size())
             << " language model files (lmodel-file), but "
             << static_cast<int>(m_setting["weight-l"].size())
             << " weights (weight-l)";
    errorMsg << endl << "You might be giving '-lmodel-file TYPE FACTOR ORDER FILENAME' but you should be giving these four as a single argument, i.e. '-lmodel-file \"TYPE FACTOR ORDER FILENAME\"'";
    errorMsg << endl << "You should also remember that each language model requires 2 weights, if and only if lmodel-oov-feature is on.";
    UserMessage::Add(errorMsg.str());
    noErrorFlag = false;
  }

  // do files exist?

  // input file
  if (noErrorFlag && m_setting["input-file"].size() == 1) {
    noErrorFlag = FileExists(m_setting["input-file"][0]);
    if (!noErrorFlag) {
      stringstream errorMsg("");
      errorMsg << endl << "Input file " << m_setting["input-file"][0] << " does not exist";
      UserMessage::Add(errorMsg.str());
    }
  }
  // generation tables
  if (noErrorFlag) {
    std::vector<std::string> ext;
    //raw tables in either un compressed or compressed form
    ext.push_back("");
    ext.push_back(".gz");
    noErrorFlag = FilesExist("generation-file", 3, ext);
  }
  // distortion
  if (noErrorFlag) {
    std::vector<std::string> ext;
    //raw tables in either un compressed or compressed form
    ext.push_back("");
    ext.push_back(".gz");
    //prefix tree format
    ext.push_back(".binlexr.idx");
    //prefix tree format
    ext.push_back(".minlexr");
    noErrorFlag = FilesExist("distortion-file", 3, ext);
  }
  return noErrorFlag;
}

/** check whether a file exists */
bool Parameter::FilesExist(const string &paramName, int fieldNo, std::vector<std::string> const& extensions)
{
  typedef std::vector<std::string> StringVec;
  StringVec::const_iterator iter;

  PARAM_MAP::const_iterator iterParam = m_setting.find(paramName);
  if (iterParam == m_setting.end()) {
    // no param. therefore nothing to check
    return true;
  }
  const StringVec &pathVec = (*iterParam).second;
  for (iter = pathVec.begin() ; iter != pathVec.end() ; ++iter) {
    StringVec vec = Tokenize(*iter);

    size_t tokenizeIndex;
    if (fieldNo == -1)
      tokenizeIndex = vec.size() - 1;
    else
      tokenizeIndex = static_cast<size_t>(fieldNo);

    if (tokenizeIndex >= vec.size()) {
      stringstream errorMsg("");
      errorMsg << "Expected at least " << (tokenizeIndex+1) << " tokens per entry in '"
               << paramName << "', but only found "
               << vec.size();
      UserMessage::Add(errorMsg.str());
      return false;
    }
    const string &pathStr = vec[tokenizeIndex];

    bool fileFound=0;
    for(size_t i=0; i<extensions.size() && !fileFound; ++i) {
      fileFound|=FileExists(pathStr + extensions[i]);
    }
    if(!fileFound) {
      stringstream errorMsg("");
      errorMsg << "File " << pathStr << " does not exist";
      UserMessage::Add(errorMsg.str());
      return false;
    }
  }
  return true;
}

/** look for a switch in arg, update parameter */
// TODO arg parsing like this does not belong in the library, it belongs
// in moses-cmd
string Parameter::FindParam(const string &paramSwitch, int argc, char* argv[])
{
  for (int i = 0 ; i < argc ; i++) {
    if (string(argv[i]) == paramSwitch) {
      if (i+1 < argc) {
        return argv[i+1];
      } else {
        stringstream errorMsg("");
        errorMsg << "Option " << paramSwitch << " requires a parameter!";
        UserMessage::Add(errorMsg.str());
        // TODO return some sort of error, not the empty string
      }
    }
  }
  return "";
}

/** update parameter settings with command line switches
 * \param paramSwitch (potentially short) name of switch
 * \param paramName full name of parameter
 * \param argc number of arguments on command line
 * \param argv values of paramters on command line */
void Parameter::OverwriteParam(const string &paramSwitch, const string &paramName, int argc, char* argv[])
{
  int startPos = -1;
  for (int i = 0 ; i < argc ; i++) {
    if (string(argv[i]) == paramSwitch) {
      startPos = i+1;
      break;
    }
  }
  if (startPos < 0)
    return;

  int index = 0;
  m_setting[paramName]; // defines the parameter, important for boolean switches
  while (startPos < argc && (!isOption(argv[startPos]))) {
    if (m_setting[paramName].size() > (size_t)index)
      m_setting[paramName][index] = argv[startPos];
    else
      m_setting[paramName].push_back(argv[startPos]);
    index++;
    startPos++;
  }
}


/** read parameters from a configuration file */
bool Parameter::ReadConfigFile(const string &filePath )
{
  InputFileStream inFile(filePath);
  string line, paramName;
  while(getline(inFile, line)) {
    // comments
    size_t comPos = line.find_first_of("#");
    if (comPos != string::npos)
      line = line.substr(0, comPos);
    // trim leading and trailing spaces/tabs
    line = Trim(line);

    if (line.size() == 0) {
      // blank line. do nothing.
    }
    else if (line[0]=='[') {
      // new parameter
      for (size_t currPos = 0 ; currPos < line.size() ; currPos++) {
        if (line[currPos] == ']') {
          paramName = line.substr(1, currPos - 1);
          break;
        }
      }
    } else {
      // add value to parameter
      m_setting[paramName].push_back(line);
    }
  }
  return true;
}

struct Credit {
  string name, contact, currentPursuits, areaResponsibility;
  int sortId;

  Credit(string name, string contact, string currentPursuits, string areaResponsibility) {
    this->name								= name							;
    this->contact							= contact						;
    this->currentPursuits			= currentPursuits		;
    this->areaResponsibility	= areaResponsibility;
    this->sortId							= rand() % 1000;
  }

  bool operator<(const Credit &other) const {
    /*
    if (areaResponsibility.size() != 0 && other.areaResponsibility.size() ==0)
    	return true;
    if (areaResponsibility.size() == 0 && other.areaResponsibility.size() !=0)
    	return false;

    return name < other.name;
    */
    return sortId < other.sortId;
  }

};

std::ostream& operator<<(std::ostream &os, const Credit &credit)
{
  os << credit.name;
  if (credit.contact != "")
    os << "\t   contact: " << credit.contact;
  if (credit.currentPursuits != "")
    os << "   " << credit.currentPursuits;
  if (credit.areaResponsibility != "")
    os << "   I'll answer question on: " << credit.areaResponsibility;
  return os;
}

void Parameter::PrintCredit()
{
  vector<Credit> everyone;
  srand ( time(NULL) );

  everyone.push_back(Credit("Nicola Bertoldi"
                            , "911"
                            , ""
                            , "scripts & other stuff"));
  everyone.push_back(Credit("Ondrej Bojar"
                            , ""
                            , "czech this out!"
                            , ""));
  everyone.push_back(Credit("Chris Callison-Burch"
                            , "anytime, anywhere"
                            , "international playboy"
                            , ""));
  everyone.push_back(Credit("Alexandra Constantin"
                            , ""
                            , "eu sunt varza"
                            , ""));
  everyone.push_back(Credit("Brooke Cowan"
                            , "brooke@csail.mit.edu"
                            , "if you're going to san francisco, be sure to wear a flower in your hair"
                            , ""));
  everyone.push_back(Credit("Chris Dyer"
                            , "can't. i'll be out driving my mustang"
                            , "driving my mustang"
                            , ""));
  everyone.push_back(Credit("Marcello Federico"
                            , "federico at itc at it"
                            , "Researcher at ITC-irst, Trento, Italy"
                            , "IRST language model"));
  everyone.push_back(Credit("Evan Herbst"
                            , "Small college in upstate New York"
                            , ""
                            , ""));
  everyone.push_back(Credit("Philipp Koehn"
                            , "only between 2 and 4am"
                            , ""
                            , "Nothing fazes this dude"));
  everyone.push_back(Credit("Christine Moran"
                            , "weird building at MIT"
                            , ""
                            , ""));
  everyone.push_back(Credit("Wade Shen"
                            , "via morse code"
                            , "buying another laptop"
                            , ""));
  everyone.push_back(Credit("Richard Zens"
                            , "richard at aachen dot de"
                            , ""
                            , "ambiguous source input, confusion networks, confusing source code"));
  everyone.push_back(Credit("Hieu Hoang", "http://www.hoang.co.uk/hieu/"
                            , "phd student at Edinburgh Uni. Original Moses developer"
                            , "general queries/ flames on Moses."));

  sort(everyone.begin(), everyone.end());


  cerr <<  "Moses - A beam search decoder for phrase-based statistical machine translation models" << endl
       << "Copyright (C) 2006 University of Edinburgh" << endl << endl

       << "This library is free software; you can redistribute it and/or" << endl
       << "modify it under the terms of the GNU Lesser General Public" << endl
       << "License as published by the Free Software Foundation; either" << endl
       << "version 2.1 of the License, or (at your option) any later version." << endl << endl

       << "This library is distributed in the hope that it will be useful," << endl
       << "but WITHOUT ANY WARRANTY; without even the implied warranty of" << endl
       << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU" << endl
       << "Lesser General Public License for more details." << endl << endl

       << "You should have received a copy of the GNU Lesser General Public" << endl
       << "License along with this library; if not, write to the Free Software" << endl
       << "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA" << endl << endl
       << "***********************************************************************" << endl << endl
       << "Built on " << __DATE__ << " at " __TIME__ << endl << endl
       << "WHO'S FAULT IS THIS GODDAM SOFTWARE:" << endl;

  ostream_iterator<Credit> out(cerr, "\n");
  copy(everyone.begin(), everyone.end(), out);
  cerr <<  endl << endl;
}

/** update parameter settings with command line switches
 * \param paramName full name of parameter
 * \param values inew values for paramName */
void Parameter::OverwriteParam(const string &paramName, PARAM_VEC values)
{
	VERBOSE(2,"Overwriting parameter " << paramName);
	
	m_setting[paramName]; // defines the parameter, important for boolean switches
	if (m_setting[paramName].size() > 1){
		VERBOSE(2," (the parameter had " << m_setting[paramName].size() << " previous values)");
		CHECK(m_setting[paramName].size() == values.size());
	}else{
		VERBOSE(2," (the parameter does not have previous values)");
		m_setting[paramName].resize(values.size());
	}
	VERBOSE(2," with the following values:");
	int i=0;
	for (PARAM_VEC::iterator iter = values.begin(); iter != values.end() ; iter++, i++){
		m_setting[paramName][i] = *iter;
		VERBOSE(2, " " << *iter);
	}
	VERBOSE(2, std::endl);
}
	
}


