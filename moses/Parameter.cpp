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
#include "util/exception.hh"

using namespace std;

namespace Moses
{

/** define allowed parameters */
Parameter::Parameter()
{
  AddParam("mapping", "description of decoding steps");
  AddParam("beam-threshold", "b", "threshold for threshold pruning");
  AddParam("config", "f", "location of the configuration file");
  //AddParam("continue-partial-translation", "cpt", "start from nonempty hypothesis");
  AddParam("decoding-graph-backoff", "dpb", "only use subsequent decoding paths for unknown spans of given length");
  AddParam("drop-unknown", "du", "drop unknown words instead of copying them");
  AddParam("disable-discarding", "dd", "disable hypothesis discarding");
  AddParam("factor-delimiter", "fd", "specify a different factor delimiter than the default");
  AddParam("input-factors", "list of factors in the input");
  AddParam("input-file", "i", "location of the input file to be translated");
  AddParam("inputtype", "text (0), confusion network (1), word lattice (2), tree (3) (default = 0)");
  AddParam("labeled-n-best-list", "print out labels for each weight type in n-best list. default is true");
  AddParam("mark-unknown", "mu", "mark unknown words in output");
  AddParam("max-partial-trans-opt", "maximum number of partial translation options per input span (during mapping steps)");
  AddParam("max-trans-opt-per-coverage", "maximum number of translation options per input span (after applying mapping steps)");
  AddParam("max-phrase-length", "maximum phrase length (default 20)");
  AddParam("n-best-list", "file and size of n-best-list to be generated; specify - as the file in order to write to STDOUT");
  AddParam("n-best-trees", "Write n-best target-side trees to n-best-list");
  AddParam("lattice-samples", "generate samples from lattice, in same format as nbest list. Uses the file and size arguments, as in n-best-list");
  AddParam("n-best-factor", "factor to compute the maximum number of contenders (=factor*nbest-size). value 0 means infinity, i.e. no threshold. default is 0");
  AddParam("print-all-derivations", "to print all derivations in search graph");
  AddParam("output-factors", "list of factors in the output");
  AddParam("phrase-drop-allowed", "da", "if present, allow dropping of source words"); //da = drop any (word); see -du for comparison
  AddParam("report-all-factors", "report all factors in output, not just first");
  AddParam("report-all-factors-in-n-best", "Report all factors in n-best-lists. Default is false");
  AddParam("stack", "s", "maximum stack size for histogram pruning. 0 = unlimited stack size");
  AddParam("stack-diversity", "sd", "minimum number of hypothesis of each coverage in stack (default 0)");
  AddParam("threads","th", "number of threads to use in decoding (defaults to single-threaded)");
  AddParam("translation-details", "T", "for each best hypothesis, report translation details to the given file");
  AddParam("tree-translation-details", "Ttree", "for each hypothesis, report translation details with tree fragment info to given file");
  //DIMw
  AddParam("translation-all-details", "Tall", "for all hypotheses, report translation details to the given file");
  AddParam("translation-option-threshold", "tot", "threshold for translation options relative to best for input phrase");
  AddParam("early-discarding-threshold", "edt", "threshold for constructing hypotheses based on estimate cost");
  AddParam("verbose", "v", "verbosity level of the logging");
  AddParam("references", "Reference file(s) - used for bleu score feature");
  AddParam("output-factors", "list if factors in the output");
  AddParam("distortion-limit", "dl", "distortion (reordering) limit in maximum number of words (0 = monotone, -1 = unlimited)");
  AddParam("monotone-at-punctuation", "mp", "do not reorder over punctuation");
  AddParam("distortion-file", "source factors (0 if table independent of source), target factors, location of the factorized/lexicalized reordering tables");
  AddParam("distortion", "configurations for each factorized/lexicalized reordering model.");
  AddParam("early-distortion-cost", "edc", "include estimate of distortion cost yet to be incurred in the score [Moore & Quirk 2007]. Default is no");
  AddParam("xml-input", "xi", "allows markup of input with desired translations and probabilities. values can be 'pass-through' (default), 'inclusive', 'exclusive', 'constraint', 'ignore'");
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
  AddParam("lmodel-oov-feature", "add language model oov feature, one per model");
  AddParam("clean-lm-cache", "clean language model caches after N translations (default N=1)");
  AddParam("recover-input-path", "r", "(conf net/word lattice only) - recover input path corresponding to the best translation");
  AddParam("output-word-graph", "owg", "Output stack info as word graph. Takes filename, 0=only hypos in stack, 1=stack + nbest hypos");
  AddParam("time-out", "seconds after which is interrupted (-1=no time-out, default is -1)");
  AddParam("output-search-graph", "osg", "Output connected hypotheses of search into specified filename");
  AddParam("output-search-graph-extended", "osgx", "Output connected hypotheses of search into specified filename, in extended format");
  AddParam("unpruned-search-graph", "usg", "When outputting chart search graph, do not exclude dead ends. Note: stack pruning may have eliminated some hypotheses");
  AddParam("output-search-graph-slf", "slf", "Output connected hypotheses of search into specified directory, one file per sentence, in HTK standard lattice format (SLF) - the flag should be followed by a directory name, which must exist");
  AddParam("output-search-graph-hypergraph", "Output connected hypotheses of search into specified directory, one file per sentence, in a hypergraph format (see Kenneth Heafield's lazy hypergraph decoder). This flag is followed by 3 values: 'true (gz|txt|bz) directory-name'");
  AddParam("include-lhs-in-search-graph", "lhssg", "When outputting chart search graph, include the label of the LHS of the rule (useful when using syntax)");
#ifdef HAVE_PROTOBUF
  AddParam("output-search-graph-pb", "pb", "Write phrase lattice to protocol buffer objects in the specified path.");
#endif
  AddParam("cube-pruning-pop-limit", "cbp", "How many hypotheses should be popped for each stack. (default = 1000)");
  AddParam("cube-pruning-diversity", "cbd", "How many hypotheses should be created for each coverage. (default = 0)");
  AddParam("description", "Source language, target language, description");
  AddParam("max-chart-span", "maximum num. of source word chart rules can consume (default 10)");
  AddParam("non-terminals", "list of non-term symbols, space separated");
  AddParam("rule-limit", "a little like table limit. But for chart decoding rules. Default is DEFAULT_MAX_TRANS_OPT_SIZE");
  AddParam("source-label-overlap", "What happens if a span already has a label. 0=add more. 1=replace. 2=discard. Default is 0");
  AddParam("output-hypo-score", "Output the hypo score to stdout with the output string. For search error analysis. Default is false");
  AddParam("unknown-lhs", "file containing target lhs of unknown words. 1 per line: LHS prob");
  AddParam("cube-pruning-lazy-scoring", "cbls", "Don't fully score a hypothesis until it is popped");
  AddParam("search-algorithm", "Which search algorithm to use. 0=normal stack, 1=cube pruning, 3=chart (with cube pruning), 4=stack with batched lm requests, 5=chart (with incremental search), 6=string-to-tree, 7=tree-to-string, 8=tree-to-string (SCFG-based), 9=forest-to-string (default = 0)");
  AddParam("link-param-count", "Number of parameters on word links when using confusion networks or lattices (default = 1)");
  AddParam("description", "Source language, target language, description");

  AddParam("max-chart-span", "maximum num. of source word chart rules can consume (default 10)");
  AddParam("non-terminals", "list of non-term symbols, space separated");
  AddParam("rule-limit", "a little like table limit. But for chart decoding rules. Default is DEFAULT_MAX_TRANS_OPT_SIZE");
  AddParam("source-label-overlap", "What happens if a span already has a label. 0=add more. 1=replace. 2=discard. Default is 0");
  AddParam("output-hypo-score", "Output the hypo score to stdout with the output string. For search error analysis. Default is false");
  AddParam("show-weights", "print feature weights and exit");
  AddParam("start-translation-id", "Id of 1st input. Default = 0");
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
  AddParam("report-segmentation-enriched", "tt", "report phrase segmentation in the output with additional information");
  AddParam("link-param-count", "DEPRECATED. DO NOT USE. Number of parameters on word links when using confusion networks or lattices (default = 1)");

  AddParam("weight-slm", "slm", "DEPRECATED. DO NOT USE. weight(s) for syntactic language model");
  AddParam("weight-bl", "bl", "DEPRECATED. DO NOT USE. weight for bleu score feature");
  AddParam("weight-d", "d", "DEPRECATED. DO NOT USE. weight(s) for distortion (reordering components)");
  AddParam("weight-dlm", "dlm", "DEPRECATED. DO NOT USE. weight for discriminative LM feature function (on top of sparse weights)");
  AddParam("weight-lr", "lr", "DEPRECATED. DO NOT USE. weight(s) for lexicalized reordering, if not included in weight-d");
  AddParam("weight-generation", "g", "DEPRECATED. DO NOT USE. weight(s) for generation components");
  AddParam("weight-i", "I", "DEPRECATED. DO NOT USE. weight(s) for word insertion - used for parameters from confusion network and lattice input links");
  AddParam("weight-l", "lm", "DEPRECATED. DO NOT USE. weight(s) for language models");
  AddParam("weight-lex", "lex", "DEPRECATED. DO NOT USE. weight for global lexical model");
  AddParam("weight-glm", "glm", "DEPRECATED. DO NOT USE. weight for global lexical feature, sparse producer");
  AddParam("weight-wt", "wt", "DEPRECATED. DO NOT USE. weight for word translation feature");
  AddParam("weight-pp", "pp", "DEPRECATED. DO NOT USE. weight for phrase pair feature");
  AddParam("weight-pb", "pb", "DEPRECATED. DO NOT USE. weight for phrase boundary feature");
  AddParam("weight-t", "tm", "DEPRECATED. DO NOT USE. weights for translation model components");
  AddParam("weight-p", "w", "DEPRECATED. DO NOT USE. weight for phrase penalty");
  AddParam("weight-w", "w", "DEPRECATED. DO NOT USE. weight for word penalty");
  AddParam("weight-u", "u", "DEPRECATED. DO NOT USE. weight for unknown word penalty");
  AddParam("weight-e", "e", "DEPRECATED. DO NOT USE. weight for word deletion");
  AddParam("text-type", "DEPRECATED. DO NOT USE. should be one of dev/devtest/test, used for domain adaptation features");
  AddParam("input-scores", "DEPRECATED. DO NOT USE. 2 numbers on 2 lines - [1] of scores on each edge of a confusion network or lattice input (default=1). [2] Number of 'real' word scores (0 or 1. default=0)");

  AddParam("dlm-model", "DEPRECATED. DO NOT USE. Order, factor and vocabulary file for discriminative LM. Use * for filename to indicate unlimited vocabulary.");
  AddParam("generation-file", "DEPRECATED. DO NOT USE. location and properties of the generation table");
  AddParam("global-lexical-file", "gl", "DEPRECATED. DO NOT USE. discriminatively trained global lexical translation model file");
  AddParam("glm-feature", "DEPRECATED. DO NOT USE. discriminatively trained global lexical translation feature, sparse producer");
  AddParam("lmodel-file", "DEPRECATED. DO NOT USE. location and properties of the language models");
  AddParam("lmodel-dub", "DEPRECATED. DO NOT USE. dictionary upper bounds of language models");

#ifdef HAVE_SYNLM
  AddParam("slmodel-file", "DEPRECATED. DO NOT USE. location of the syntactic language model file(s)");
  AddParam("slmodel-factor", "DEPRECATED. DO NOT USE. factor to use with syntactic language model");
  AddParam("slmodel-beam", "DEPRECATED. DO NOT USE. beam width to use with syntactic language model's parser");
#endif
  AddParam("ttable-file", "DEPRECATED. DO NOT USE. location and properties of the translation tables");
  AddParam("phrase-pair-feature", "DEPRECATED. DO NOT USE. Source and target factors for phrase pair feature");
  AddParam("phrase-boundary-source-feature", "DEPRECATED. DO NOT USE. Source factors for phrase boundary feature");
  AddParam("phrase-boundary-target-feature", "DEPRECATED. DO NOT USE. Target factors for phrase boundary feature");
  AddParam("phrase-length-feature", "DEPRECATED. DO NOT USE. Count features for source length, target length, both of each phrase");
  AddParam("target-word-insertion-feature", "DEPRECATED. DO NOT USE. Count feature for each unaligned target word");
  AddParam("source-word-deletion-feature", "DEPRECATED. DO NOT USE. Count feature for each unaligned source word");
  AddParam("word-translation-feature", "DEPRECATED. DO NOT USE. Count feature for word translation according to word alignment");

  AddParam("weight-file", "wf", "feature weights file. Do *not* put weights for 'core' features in here - they go in moses.ini");

  AddParam("weight", "weights for ALL models, 1 per line 'WeightName value'. Weight names can be repeated");
  AddParam("weight-overwrite", "special parameter for mert. All on 1 line. Overrides weights specified in 'weights' argument");
  AddParam("feature-overwrite", "Override arguments in a particular feature function with a particular key. Format: -feature-overwrite \"FeatureName key=value\"");
  AddParam("weight-add", "Add weight for FF if it doesn't exist, i.e weights here are added 1st, and can be override by the ini file or on the command line. Used to specify initial weights for FF that was also specified on the copmmand line");
  AddParam("feature-add", "Add a feature function on the command line. Used by mira to add BLEU feature");
  AddParam("feature-name-overwrite", "Override feature name (NOT arguments). Eg. SRILM-->KENLM, PhraseDictionaryMemory-->PhraseDictionaryScope3");

  AddParam("feature", "All the feature functions should be here");

  AddParam("print-id", "prefix translations with id. Default if false");

  AddParam("print-passthrough", "output the sgml tag <passthrough> without any computation on that. Default is false");
  AddParam("print-passthrough-in-n-best", "output the sgml tag <passthrough> without any computation on that in each entry of the n-best-list. Default is false");

  AddParam("alternate-weight-setting", "aws", "alternate set of weights to used per xml specification");

  AddParam("placeholder-factor", "Which source factor to use to store the original text for placeholders. The factor must not be used by a translation or gen model");
  AddParam("no-cache", "Disable all phrase-table caching. Default = false (ie. enable caching)");
  AddParam("default-non-term-for-empty-range-only", "Don't add [X] to all ranges, just ranges where there isn't a source non-term. Default = false (ie. add [X] everywhere)");
  AddParam("s2t-parsing-algorithm", "Which S2T parsing algorithm to use. 0=recursive CYK+, 1=scope-3 (default = 0)");

  AddParam("spe-src", "Simulated post-editing. Source filename");
  AddParam("spe-trg", "Simulated post-editing. Target filename");
  AddParam("spe-aln", "Simulated post-editing. Alignment filename");
}

Parameter::~Parameter()
{
}

const PARAM_VEC *Parameter::GetParam(const std::string &paramName) const
{
  PARAM_MAP::const_iterator iter = m_setting.find( paramName );
  if (iter == m_setting.end()) {
    return NULL;
  } else {
    return &iter->second;
  }

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
    PrintFF();

    cerr << endl;
    cerr << "No configuration file was specified.  Use -config or -f";
    cerr << endl;
    return false;
  } else {
    if (!ReadConfigFile(configPath)) {
      std::cerr << "Could not read " << configPath;
      return false;
    }
  }

  // overwrite parameters with values from switches
  for(PARAM_STRING::const_iterator iterParam = m_description.begin();
      iterParam != m_description.end(); iterParam++) {
    const string paramName = iterParam->first;
    OverwriteParam("-" + paramName, paramName, argc, argv);
  }

  // ... also shortcuts
  for(PARAM_STRING::const_iterator iterParam = m_abbreviation.begin();
      iterParam != m_abbreviation.end(); iterParam++) {
    const string paramName = iterParam->first;
    const string paramShortName = iterParam->second;
    OverwriteParam("-" + paramShortName, paramName, argc, argv);
  }

  AddFeaturesCmd();

  // logging of parameters that were set in either config or switch
  int verbose = 1;
  if (m_setting.find("verbose") != m_setting.end() &&
      m_setting["verbose"].size() > 0)
    verbose = Scan<int>(m_setting["verbose"][0]);
  if (verbose >= 1) { // only if verbose
    TRACE_ERR( "Defined parameters (per moses.ini or switch):" << endl);
    for(PARAM_MAP::const_iterator iterParam = m_setting.begin() ;
        iterParam != m_setting.end(); iterParam++) {
      TRACE_ERR( "\t" << iterParam->first << ": ");
      for ( size_t i = 0; i < iterParam->second.size(); i++ )
        TRACE_ERR( iterParam->second[i] << " ");
      TRACE_ERR( endl);
    }
  }

  // don't mix old and new format
  if ((GetParam("feature") || GetParam("weight"))
      && (GetParam("weight-slm") || GetParam("weight-bl") || GetParam("weight-d") ||
          GetParam("weight-dlm") || GetParam("weight-lrl") || GetParam("weight-generation") ||
          GetParam("weight-i") || GetParam("weight-l") || GetParam("weight-lex") ||
          GetParam("weight-glm") || GetParam("weight-wt") || GetParam("weight-pp") ||
          GetParam("weight-pb") || GetParam("weight-t") || GetParam("weight-w") ||
          GetParam("weight-p") ||
          GetParam("weight-u") || GetParam("weight-e") ||
          GetParam("dlm-mode") || GetParam("generation-file") || GetParam("global-lexical-file") ||
          GetParam("glm-feature") || GetParam("lmodel-file") || GetParam("lmodel-dub") ||
          GetParam("slmodel-file") || GetParam("slmodel-factor") ||
          GetParam("slmodel-beam") || GetParam("ttable-file") || GetParam("phrase-pair-feature") ||
          GetParam("phrase-boundary-source-feature") || GetParam("phrase-boundary-target-feature") || GetParam("phrase-length-feature") ||
          GetParam("target-word-insertion-feature") || GetParam("source-word-deletion-feature") || GetParam("word-translation-feature")
         )
     ) {
    UTIL_THROW(util::Exception, "Don't mix old and new ini file format");
  }

  // convert old weights args to new format
  if (GetParam("feature") == NULL) {
    ConvertWeightArgs();
  }
  CreateWeightsMap();
  WeightOverwrite();

  // check for illegal parameters
  bool noErrorFlag = true;
  for (int i = 0 ; i < argc ; i++) {
    if (isOption(argv[i])) {
      string paramSwitch = (string) argv[i];
      string paramName = paramSwitch.substr(1);
      if (m_valid.find(paramName) == m_valid.end()) {
        std::cerr << "illegal switch: " << paramSwitch;
        noErrorFlag = false;
      }
    }
  }

  //Save("/tmp/moses.ini.new");

  // check if parameters make sense
  return Validate() && noErrorFlag;
}

void Parameter::AddFeaturesCmd()
{
  const PARAM_VEC *params = GetParam("feature-add");
  if (params) {
    PARAM_VEC::const_iterator iter;
    for (iter = params->begin(); iter != params->end(); ++iter) {
      const string &line = *iter;
      AddFeature(line);
    }

    m_setting.erase("feature-add");
  }
}

std::vector<float> Parameter::GetWeights(const std::string &name)
{
  std::vector<float> ret = m_weights[name];

  // cerr << "WEIGHT " << name << "=";
  // for (size_t i = 0; i < ret.size(); ++i) {
  //   cerr << ret[i] << ",";
  // }
  // cerr << endl;
  return ret;
}

void Parameter::SetWeight(const std::string &name, size_t ind, float weight)
{
  PARAM_VEC &newWeights = m_setting["weight"];
  string line = name + SPrint(ind) + "= " + SPrint(weight);
  newWeights.push_back(line);
}

void Parameter::SetWeight(const std::string &name, size_t ind, const vector<float> &weights)
{
  PARAM_VEC &newWeights = m_setting["weight"];
  string line = name + SPrint(ind) + "=";

  for (size_t i = 0; i < weights.size(); ++i) {
    line += " " + SPrint(weights[i]);
  }
  newWeights.push_back(line);
}

void
Parameter::
AddWeight(const std::string &name, size_t ind,
          const std::vector<float> &weights)
{
  PARAM_VEC &newWeights = m_setting["weight"];

  string sought = name + SPrint(ind) + "=";
  for (size_t i = 0; i < newWeights.size(); ++i) {
    string &line = newWeights[i];
    if (line.find(sought) == 0) {
      // found existing weight, most likely to be input weights. Append to this line
      for (size_t i = 0; i < weights.size(); ++i) {
        line += " " + SPrint(weights[i]);
      }
      return;
    }
  }

  // nothing found. Just set
  SetWeight(name, ind, weights);
}

void Parameter::ConvertWeightArgsSingleWeight(const string &oldWeightName, const string &newWeightName)
{
  size_t ind = 0;
  PARAM_MAP::iterator iterMap;

  iterMap = m_setting.find(oldWeightName);
  if (iterMap != m_setting.end()) {
    const PARAM_VEC &weights = iterMap->second;
    for (size_t i = 0; i < weights.size(); ++i) {
      SetWeight(newWeightName, ind, Scan<float>(weights[i]));
    }

    m_setting.erase(iterMap);
  }
}

void Parameter::ConvertWeightArgsPhraseModel(const string &oldWeightName)
{
  const PARAM_VEC *params;

  // process input weights 1st
  params = GetParam("weight-i");
  if (params) {
    vector<float> inputWeights = Scan<float>(*params);
    PARAM_VEC &numInputScores = m_setting["input-scores"];
    if (inputWeights.size() == 1) {
      UTIL_THROW_IF2(numInputScores.size() != 0, "No [input-scores] section allowed");
      numInputScores.push_back("1");
      numInputScores.push_back("0");
    } else if (inputWeights.size() == 2) {
      UTIL_THROW_IF2(numInputScores.size() != 0, "No [input-scores] section allowed");
      numInputScores.push_back("1");
      numInputScores.push_back("1");
    }

    SetWeight("PhraseDictionaryBinary", 0, inputWeights);
  }

  // convert actually pt feature
  VERBOSE(2,"Creating phrase table features" << endl);

  size_t numInputScores = 0;
  size_t numRealWordsInInput = 0;
  map<string, size_t> ptIndices;

  params = GetParam("input-scores");
  if (params) {
    numInputScores = Scan<size_t>(params->at(0));

    if (params->size() > 1) {
      numRealWordsInInput = Scan<size_t>(params->at(1));
    }
  }

  // load phrase translation tables
  params = GetParam("ttable-file");
  if (params) {
    // weights
    const vector<string> translationVector = *params;

    vector<size_t>  maxTargetPhrase;
    params = GetParam("ttable-limit");
    if (params) {
      maxTargetPhrase = Scan<size_t>(*params);
    }

    if(maxTargetPhrase.size() == 1 && translationVector.size() > 1) {
      VERBOSE(1, "Using uniform ttable-limit of " << maxTargetPhrase[0] << " for all translation tables." << endl);
      for(size_t i = 1; i < translationVector.size(); i++)
        maxTargetPhrase.push_back(maxTargetPhrase[0]);
    } else if(maxTargetPhrase.size() != 1 && maxTargetPhrase.size() < translationVector.size()) {
      std::cerr << "You specified " << translationVector.size() << " translation tables, but only " << maxTargetPhrase.size() << " ttable-limits.";
      return;
    }

    // MAIN LOOP
    const PARAM_VEC &oldWeights = m_setting[oldWeightName];

    size_t currOldInd = 0;
    for(size_t currDict = 0 ; currDict < translationVector.size(); currDict++) {
      stringstream ptLine;

      vector<string> token = Tokenize(translationVector[currDict]);

      if(currDict == 0 && token.size() == 4) {
        std::cerr << "Phrase table specification in old 4-field format. No longer supported";
        return;
      }
      UTIL_THROW_IF2(token.size() < 5, "Phrase table must have at least 5 scores");

      int implementation = Scan<int>(token[0]);

      string ptType;
      switch (implementation) {
      case 0: // Memory
        ptType = "PhraseDictionaryMemory";
        break;
      case 1: // Binary
        ptType = "PhraseDictionaryBinary";
        break;
      case 2: // OnDisk
        ptType = "PhraseDictionaryOnDisk";
        break;
      case 6: // SCFG
        ptType = "PhraseDictionaryMemory";
        break;
      case 12: // Compact
        ptType = "PhraseDictionaryCompact";
        break;
      case 8: // SuffixArray
        ptType = "PhraseDictionarySuffixArray";
        break;
      case 14: // DSuffixArray
        ptType = "PhraseDictionaryDynSuffixArray";
        break;
      case 15: // DCacheBased:
        ptType = "PhraseDictionaryDynamicCacheBased";
        break;
      default:
        break;
      }

      size_t ptInd;
      if (ptIndices.find(ptType) == ptIndices.end()) {
        ptIndices[ptType] = 0;
        ptInd = 0;
      } else {
        ptInd = ++ptIndices[ptType];
      }

      // weights
      size_t numFFInd = (token.size() == 4) ? 2 : 3;
      size_t numFF = Scan<size_t>(token[numFFInd]);

      vector<float> weights(numFF);
      for (size_t currFF = 0; currFF < numFF; ++currFF) {
        UTIL_THROW_IF2(currOldInd >= oldWeights.size(),
                       "Errors converting old phrase-table weights to new weights");
        float weight = Scan<float>(oldWeights[currOldInd]);
        weights[currFF] = weight;

        ++currOldInd;
      }

      // cerr << weights.size() << " PHRASE TABLE WEIGHTS "
      // << __FILE__ << ":" << __LINE__ << endl;
      AddWeight(ptType, ptInd, weights);

      // actual pt
      ptLine << ptType << " ";
      ptLine << "input-factor=" << token[1] << " ";
      ptLine << "output-factor=" << token[2] << " ";
      ptLine << "path=" << token[4] << " ";

      //characteristics of the phrase table

      vector<FactorType>  input   = Tokenize<FactorType>(token[1], ",")
                                    ,output  = Tokenize<FactorType>(token[2], ",");
      size_t numScoreComponent = Scan<size_t>(token[3]);
      string filePath= token[4];

      if(currDict==0) {
        // only the 1st pt. THis is shit
        // TODO. find what the assumptions made by confusion network about phrase table output which makes
        // it only work with binary file. This is a hack
        numScoreComponent += numInputScores + numRealWordsInInput;
      }

      ptLine << "num-features=" << numScoreComponent << " ";
      ptLine << "table-limit=" << maxTargetPhrase[currDict] << " ";

      if (implementation == 8 || implementation == 14) {
        ptLine << "target-path=" << token[5] << " ";
        ptLine << "alignment-path=" << token[6] << " ";
      }

      AddFeature(ptLine.str());
    } // for(size_t currDict = 0 ; currDict < translationVector.size(); currDict++) {
  } // if (GetParam("ttable-file").size() > 0) {

  m_setting.erase("weight-i");
  m_setting.erase(oldWeightName);
  m_setting.erase("ttable-file");
  m_setting.erase("ttable-limit");

}

void Parameter::AddFeature(const std::string &line)
{
  PARAM_VEC &features = m_setting["feature"];
  features.push_back(line);
}

void Parameter::ConvertWeightArgsDistortion()
{
  const string oldWeightName = "weight-d";
  const string oldLexReordingName = "distortion-file";

  // distortion / lex distortion
  const PARAM_VEC *oldWeights = GetParam(oldWeightName);

  if (oldWeights) {
    const PARAM_VEC *searchAlgo = GetParam("search-algorithm");
    if (searchAlgo == NULL ||
        (searchAlgo->size() > 0
         && (Trim(searchAlgo->at(0)) == "0" || Trim(searchAlgo->at(0)) == "1")
        )
       ) {
      // phrase-based. Add distance distortion to list of features
      AddFeature("Distortion");
      SetWeight("Distortion", 0, Scan<float>(oldWeights->at(0)));
    }

    // everything but the last is lex reordering model

    size_t currOldInd = 1;
    const PARAM_VEC *lextable = GetParam(oldLexReordingName);

    for (size_t indTable = 0; lextable && indTable < lextable->size(); ++indTable) {
      const string &line = lextable->at(indTable);
      vector<string> toks = Tokenize(line);

      size_t numFF = Scan<size_t>(toks[2]);

      vector<float> weights(numFF);
      for (size_t currFF = 0; currFF < numFF; ++currFF) {
        UTIL_THROW_IF2(oldWeights && currOldInd >= oldWeights->size(),
                       "Errors converting old distortion weights to new weights");
        float weight = Scan<float>(oldWeights->at(currOldInd));
        weights[currFF] = weight;

        ++currOldInd;
      }
      SetWeight("LexicalReordering", indTable, weights);

      stringstream strme;
      strme << "LexicalReordering "
            << "type=" << toks[1] << " ";

      vector<FactorType> factors = Tokenize<FactorType>(toks[0], "-");
      UTIL_THROW_IF2(factors.size() != 2,
                     "Error in old factor specification for lexicalized reordering model: "
                     << toks[0]);
      strme << "input-factor=" << factors[0]
            << " output-factor=" << factors[1] << " ";

      strme << "num-features=" << toks[2] << " ";
      strme << "path=" << toks[3];

      AddFeature(strme.str());
    }
  }

  m_setting.erase(oldWeightName);
  m_setting.erase(oldLexReordingName);

}

void Parameter::ConvertWeightArgsLM()
{
  const string oldWeightName = "weight-l";
  const string oldFeatureName = "lmodel-file";
  const PARAM_VEC *params;

  bool isChartDecoding = true;

  params = GetParam("search-algorithm");
  if (params == NULL ||
      (params->size() > 0
       && (Trim(params->at(0)) == "0" || Trim(params->at(0)) == "1")
      )
     ) {
    isChartDecoding = false;
  }

  vector<int> oovWeights;
  params = GetParam("lmodel-oov-feature");
  if (params) {
    oovWeights = Scan<int>(*params);
  }

  PARAM_MAP::iterator iterMap;

  iterMap = m_setting.find(oldWeightName);
  if (iterMap != m_setting.end()) {

    size_t currOldInd = 0;
    const PARAM_VEC &weights = iterMap->second;
    const PARAM_VEC &models = m_setting[oldFeatureName];
    for (size_t lmIndex = 0; lmIndex < models.size(); ++lmIndex) {
      const string &line = models[lmIndex];
      vector<string> modelToks = Tokenize(line);

      int lmType = Scan<int>(modelToks[0]);

      string newFeatureName;
      switch (lmType) {
      case 0:
        newFeatureName = "SRILM";
        break;
      case 1:
        newFeatureName = "IRSTLM";
        break;
      case 8:
      case 9:
        newFeatureName = "KENLM";
        break;
      default:
        UTIL_THROW2("Unkown language model type id:"  << lmType);
      }

      size_t numFF = 1;
      if (oovWeights.size() > lmIndex)
        numFF += oovWeights[lmIndex];

      vector<float> weightsLM(numFF);
      for (size_t currFF = 0; currFF < numFF; ++currFF) {
        UTIL_THROW_IF2(currOldInd >= weights.size(),
                       "Errors converting old LM weights to new weights");
        weightsLM[currFF] = Scan<float>(weights[currOldInd]);
        if (isChartDecoding) {
          weightsLM[currFF] = UntransformLMScore(weightsLM[currFF]);
        }

        ++currOldInd;
      }

      SetWeight(newFeatureName, lmIndex, weightsLM);

      string featureLine = newFeatureName + " "
                           + "factor=" + modelToks[1] + " "  // factor
                           + "order="  + modelToks[2] + " " // order
                           + "num-features=" + SPrint(numFF) + " ";
      if (lmType == 9) {
        featureLine += "lazyken=1 ";
      } else if (lmType == 8) {
        featureLine += "lazyken=0 ";
      }

      featureLine += "path=" + modelToks[3]; // file

      AddFeature(featureLine);
    } // for (size_t lmIndex = 0; lmIndex < models.size(); ++lmIndex) {

    m_setting.erase(iterMap);
  }

  m_setting.erase(oldFeatureName);
}

void Parameter::ConvertWeightArgsGeneration(const std::string &oldWeightName, const std::string &newWeightName)
{
  string oldFeatureName = "generation-file";

  // distortion / lex distortion
  PARAM_VEC &oldWeights = m_setting[oldWeightName];

  if (oldWeights.size() > 0) {
    size_t currOldInd = 0;
    PARAM_VEC &models = m_setting[oldFeatureName];

    for (size_t indTable = 0; indTable < models.size(); ++indTable) {
      string &line = models[indTable];
      vector<string> modelToks = Tokenize(line);

      size_t numFF = Scan<size_t>(modelToks[2]);

      vector<float> weights(numFF);
      for (size_t currFF = 0; currFF < numFF; ++currFF) {
        UTIL_THROW_IF2(currOldInd >= oldWeights.size(),
                       "Errors converting old generation weights to new weights");
        float weight = Scan<float>(oldWeights[currOldInd]);
        weights[currFF] = weight;

        ++currOldInd;
      }
      SetWeight(newWeightName, indTable, weights);

      stringstream strme;
      strme << "Generation "
            << "input-factor=" << modelToks[0] << " "
            << "output-factor=" << modelToks[1] << " "
            << "num-features=" << modelToks[2] << " "
            << "path=" << modelToks[3];
      AddFeature(strme.str());
    }
  }

  m_setting.erase(oldWeightName);
  m_setting.erase(oldFeatureName);
}

void Parameter::ConvertWeightArgsWordPenalty()
{
  const std::string oldWeightName = "weight-w";
  const std::string newWeightName = "WordPenalty";

  bool isChartDecoding = true;
  const PARAM_VEC *searchAlgo = GetParam("search-algorithm");
  if (searchAlgo == NULL ||
      (searchAlgo->size() > 0
       && (Trim(searchAlgo->at(0)) == "0" || Trim(searchAlgo->at(0)) == "1")
      )
     ) {
    isChartDecoding = false;
  }

  PARAM_MAP::iterator iterMap;

  iterMap = m_setting.find(oldWeightName);
  if (iterMap != m_setting.end()) {
    const PARAM_VEC &weights = iterMap->second;
    for (size_t i = 0; i < weights.size(); ++i) {
      float weight = Scan<float>(weights[i]);
      if (isChartDecoding) {
        weight *= 0.434294482;
      }
      SetWeight(newWeightName, i, weight);
    }

    m_setting.erase(iterMap);
  }

}

void Parameter::ConvertPhrasePenalty()
{
  string oldWeightName = "weight-p";
  const PARAM_VEC *params = GetParam(oldWeightName);
  if (params) {
    UTIL_THROW_IF2(params->size() != 1,
                   "There should be only 1 phrase-penalty weight");
    float weight = Scan<float>(params->at(0));
    AddFeature("PhrasePenalty");
    SetWeight("PhrasePenalty", 0, weight);

    m_setting.erase(oldWeightName);
  }
}

void Parameter::ConvertWeightArgs()
{
  // can't handle discr LM. must do it manually 'cos of bigram/n-gram split
  UTIL_THROW_IF2( m_setting.count("weight-dlm") != 0,
                  "Can't handle discr LM. must do it manually 'cos of bigram/n-gram split");

  // check that old & new format aren't mixed
  if (m_setting.count("weight") &&
      (m_setting.count("weight-i") || m_setting.count("weight-t") || m_setting.count("weight-w") ||
       m_setting.count("weight-l") || m_setting.count("weight-u") || m_setting.count("weight-lex") ||
       m_setting.count("weight-generation") || m_setting.count("weight-lr") || m_setting.count("weight-d")
      )) {
    cerr << "Do not mix old and new format for specify weights";
  }

  ConvertWeightArgsWordPenalty();
  ConvertWeightArgsLM();
  ConvertWeightArgsSingleWeight("weight-slm", "SyntacticLM");
  ConvertWeightArgsSingleWeight("weight-u", "UnknownWordPenalty");
  ConvertWeightArgsGeneration("weight-generation", "Generation");
  ConvertWeightArgsDistortion();

  // don't know or can't be bothered converting these weights
  ConvertWeightArgsSingleWeight("weight-lr", "LexicalReordering");
  ConvertWeightArgsSingleWeight("weight-bl", "BleuScoreFeature");
  ConvertWeightArgsSingleWeight("weight-glm", "GlobalLexicalModel");
  ConvertWeightArgsSingleWeight("weight-wt", "WordTranslationFeature");
  ConvertWeightArgsSingleWeight("weight-pp", "PhrasePairFeature");
  ConvertWeightArgsSingleWeight("weight-pb", "PhraseBoundaryFeature");

  ConvertWeightArgsSingleWeight("weight-e", "WordDeletion"); // TODO Can't find real name
  ConvertWeightArgsSingleWeight("weight-lex", "GlobalLexicalReordering"); // TODO Can't find real name

  ConvertPhrasePenalty();

  AddFeature("WordPenalty");
  AddFeature("UnknownWordPenalty");

  ConvertWeightArgsPhraseModel("weight-t");

}

void Parameter::CreateWeightsMap()
{
  CreateWeightsMap(m_setting["weight-add"]);
  CreateWeightsMap(m_setting["weight"]);
}

void Parameter::CreateWeightsMap(const PARAM_VEC &vec)
{
  for (size_t i = 0; i < vec.size(); ++i) {
    const string &line = vec[i];
    vector<string> toks = Tokenize(line);
    UTIL_THROW_IF2(toks.size() < 2,
                   "Error in format of weights: " << line);

    string name = toks[0];
    name = name.substr(0, name.size() - 1);

    vector<float> weights(toks.size() - 1);
    for (size_t i = 1; i < toks.size(); ++i) {
      float weight = Scan<float>(toks[i]);
      weights[i - 1] = weight;
    }
    m_weights[name] = weights;
  }
}

void Parameter::WeightOverwrite()
{
  PARAM_VEC &vec = m_setting["weight-overwrite"];

  if (vec.size() == 0)
    return;

  // should only be on 1 line
  UTIL_THROW_IF2(vec.size() != 1,
                 "Weight override should only be on 1 line");

  string name("");
  vector<float> weights;
  vector<string> toks = Tokenize(vec[0]);
  for (size_t i = 0; i < toks.size(); ++i) {
    const string &tok = toks[i];

    if (tok.substr(tok.size() - 1, 1) == "=") {
      // start of new feature

      if (name != "") {
        // save previous ff
        m_weights[name] = weights;
        weights.clear();
      }

      name = tok.substr(0, tok.size() - 1);
    } else {
      // a weight for curr ff
      float weight = Scan<float>(toks[i]);
      weights.push_back(weight);
    }
  }

  m_weights[name] = weights;

}

/** check that parameter settings make sense */
bool Parameter::Validate()
{
  bool noErrorFlag = true;

  PARAM_MAP::const_iterator iterParams;
  for (iterParams = m_setting.begin(); iterParams != m_setting.end(); ++iterParams) {
    const std::string &key = iterParams->first;

    if (m_valid.find(key) == m_valid.end()) {
      std::cerr << "Unknown parameter " << key;
      noErrorFlag = false;
    }
  }

  if (m_setting["lmodel-dub"].size() > 0) {
    if (m_setting["lmodel-file"].size() != m_setting["lmodel-dub"].size()) {
      std::cerr << "Config and parameters specify "
                << static_cast<int>(m_setting["lmodel-file"].size())
                << " language model files (lmodel-file), but "
                << static_cast<int>(m_setting["lmodel-dub"].size())
                << " LM upperbounds (lmodel-dub)"
                << endl;
      noErrorFlag = false;
    }
  }

  // do files exist?

  // input file
  if (noErrorFlag && m_setting["input-file"].size() == 1) {
    noErrorFlag = FileExists(m_setting["input-file"][0]);
    if (!noErrorFlag) {
      std::cerr << endl << "Input file " << m_setting["input-file"][0] << " does not exist";
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
      std::cerr << "Expected at least " << (tokenizeIndex+1) << " tokens per entry in '"
                << paramName << "', but only found "
                << vec.size();
      return false;
    }
    const string &pathStr = vec[tokenizeIndex];

    bool fileFound=0;
    for(size_t i=0; i<extensions.size() && !fileFound; ++i) {
      fileFound|=FileExists(pathStr + extensions[i]);
    }
    if(!fileFound) {
      std::cerr << "File " << pathStr << " does not exist";
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
        std::cerr << "Option " << paramSwitch << " requires a parameter!";
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
    } else if (line[0]=='[') {
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
  if (m_setting[paramName].size() > 1) {
    VERBOSE(2," (the parameter had " << m_setting[paramName].size() << " previous values)");
    UTIL_THROW_IF2(m_setting[paramName].size() != values.size(),
                   "Number of weight override for " << paramName
                   << " is not the same as the original number of weights");
  } else {
    VERBOSE(2," (the parameter does not have previous values)");
    m_setting[paramName].resize(values.size());
  }
  VERBOSE(2," with the following values:");
  int i=0;
  for (PARAM_VEC::iterator iter = values.begin(); iter != values.end() ; iter++, i++) {
    m_setting[paramName][i] = *iter;
    VERBOSE(2, " " << *iter);
  }
  VERBOSE(2, std::endl);
}

void Parameter::PrintFF() const
{
  StaticData::Instance().GetFeatureRegistry().PrintFF();
}

std::set<std::string> Parameter::GetWeightNames() const
{
  std::set<std::string> ret;
  std::map<std::string, std::vector<float> >::const_iterator iter;
  for (iter = m_weights.begin(); iter != m_weights.end(); ++iter) {
    const string &key = iter->first;
    ret.insert(key);
  }
  return ret;
}

void Parameter::Save(const std::string path)
{
  ofstream file;
  file.open(path.c_str());

  PARAM_MAP::const_iterator iterOuter;
  for (iterOuter = m_setting.begin(); iterOuter != m_setting.end(); ++iterOuter) {
    const std::string &sectionName = iterOuter->first;
    file << "[" << sectionName << "]" << endl;

    const PARAM_VEC &values = iterOuter->second;

    PARAM_VEC::const_iterator iterInner;
    for (iterInner = values.begin(); iterInner != values.end(); ++iterInner) {
      const std::string &value = *iterInner;
      file << value << endl;
    }

    file << endl;
  }


  file.close();
}

template<>
void Parameter::SetParameter<bool>(bool &parameter, const std::string &parameterName, const bool &defaultValue) const
{
  const PARAM_VEC *params = GetParam(parameterName);

  // default value if nothing is specified
  parameter = defaultValue;
  if (params == NULL) {
    return;
  }

  // if parameter is just specified as, e.g. "-parameter" set it true
  if (params->size() == 0) {
    parameter = true;
  }
  // if paramter is specified "-parameter true" or "-parameter false"
  else if (params->size() == 1) {
    parameter = Scan<bool>( params->at(0));
  }
}

} // namespace


