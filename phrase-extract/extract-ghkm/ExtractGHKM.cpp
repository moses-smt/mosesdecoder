/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "ExtractGHKM.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

#include <boost/program_options.hpp>

#include "syntax-common/exception.h"
#include "syntax-common/xml_tree_parser.h"

#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "SyntaxNode.h"
#include "SyntaxNodeCollection.h"
#include "SyntaxTree.h"
#include "tables-core.h"
#include "XmlException.h"
#include "XmlTree.h"

#include "Alignment.h"
#include "AlignmentGraph.h"
#include "Node.h"
#include "Options.h"
#include "PhraseOrientation.h"
#include "ScfgRule.h"
#include "ScfgRuleWriter.h"
#include "Span.h"
#include "StsgRule.h"
#include "StsgRuleWriter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

int ExtractGHKM::Main(int argc, char *argv[])
{
  using Moses::InputFileStream;
  using Moses::OutputFileStream;

  // Process command-line options.
  Options options;
  ProcessOptions(argc, argv, options);

  // Open input files.
  //
  // The GHKM algorithm is neutral about whether the model is string-to-tree or
  // tree-to-string.  This implementation assumes the model to be
  // string-to-tree, but if the -t2s option is given then the source and target
  // input files are switched prior to extraction and then the source and
  // target of the extracted rules are switched on output.
  std::string effectiveTargetFile = options.t2s ? options.sourceFile
                                    : options.targetFile;
  std::string effectiveSourceFile = options.t2s ? options.targetFile
                                    : options.sourceFile;
  InputFileStream targetStream(effectiveTargetFile);
  InputFileStream sourceStream(effectiveSourceFile);
  InputFileStream alignmentStream(options.alignmentFile);

  // Open output files.
  OutputFileStream fwdExtractStream;
  OutputFileStream invExtractStream;
  OutputFileStream glueGrammarStream;
  OutputFileStream targetUnknownWordStream;
  OutputFileStream sourceUnknownWordStream;
  OutputFileStream sourceLabelSetStream;
  OutputFileStream unknownWordSoftMatchesStream;

  std::string fwdFileName = options.extractFile;
  std::string invFileName = options.extractFile + std::string(".inv");
  if (options.gzOutput) {
    fwdFileName += ".gz";
    invFileName += ".gz";
  }
  OpenOutputFileOrDie(fwdFileName, fwdExtractStream);
  OpenOutputFileOrDie(invFileName, invExtractStream);

  if (!options.glueGrammarFile.empty()) {
    OpenOutputFileOrDie(options.glueGrammarFile, glueGrammarStream);
  }
  if (!options.targetUnknownWordFile.empty()) {
    OpenOutputFileOrDie(options.targetUnknownWordFile, targetUnknownWordStream);
  }
  if (!options.sourceUnknownWordFile.empty()) {
    OpenOutputFileOrDie(options.sourceUnknownWordFile, sourceUnknownWordStream);
  }
  if (!options.sourceLabelSetFile.empty()) {
    if (!options.sourceLabels) {
      Error("SourceLabels should be active if SourceLabelSet is supposed to be written to a file");
    }
    OpenOutputFileOrDie(options.sourceLabelSetFile, sourceLabelSetStream); // note that this is not a global source label set if extraction is parallelized
  }
  if (!options.unknownWordSoftMatchesFile.empty()) {
    OpenOutputFileOrDie(options.unknownWordSoftMatchesFile, unknownWordSoftMatchesStream);
  }

  // Word count statistics for producing unknown word labels.
  std::map<std::string, int> targetWordCount;
  std::map<std::string, std::string> targetWordLabel;

  // Word count statistics for producing unknown word labels: source side.
  std::map<std::string, int> sourceWordCount;
  std::map<std::string, std::string> sourceWordLabel;

  std::string targetLine;
  std::string sourceLine;
  std::string alignmentLine;
  Alignment alignment;
  XmlTreeParser targetXmlTreeParser;
  XmlTreeParser sourceXmlTreeParser;
  ScfgRuleWriter scfgWriter(fwdExtractStream, invExtractStream, options);
  StsgRuleWriter stsgWriter(fwdExtractStream, invExtractStream, options);
  size_t lineNum = options.sentenceOffset;
  while (true) {
    std::getline(targetStream, targetLine);
    std::getline(sourceStream, sourceLine);
    std::getline(alignmentStream, alignmentLine);

    if (targetStream.eof() && sourceStream.eof() && alignmentStream.eof()) {
      break;
    }

    if (targetStream.eof() || sourceStream.eof() || alignmentStream.eof()) {
      Error("Files must contain same number of lines");
    }

    ++lineNum;

    // Parse target tree.
    if (targetLine.size() == 0) {
      std::cerr << "skipping line " << lineNum << " with empty target tree\n";
      continue;
    }
    std::auto_ptr<SyntaxTree> targetParseTree;
    try {
      targetParseTree = targetXmlTreeParser.Parse(targetLine);
      assert(targetParseTree.get());
    } catch (const Exception &e) {
      std::ostringstream oss;
      oss << "Failed to parse target XML tree at line " << lineNum;
      if (!e.msg().empty()) {
        oss << ": " << e.msg();
      }
      Error(oss.str());
    }

    // Read source tokens (and parse tree if using source labels).
    std::vector<std::string> sourceTokens;
    std::auto_ptr<SyntaxTree> sourceParseTree;
    if (!options.sourceLabels) {
      sourceTokens = ReadTokens(sourceLine);
    } else {
      try {
        sourceParseTree = sourceXmlTreeParser.Parse(sourceLine);
        assert(sourceParseTree.get());
      } catch (const Exception &e) {
        std::ostringstream oss;
        oss << "Failed to parse source XML tree at line " << lineNum;
        if (!e.msg().empty()) {
          oss << ": " << e.msg();
        }
        Error(oss.str());
      }
      sourceTokens = sourceXmlTreeParser.words();
    }

    // Read word alignments.
    try {
      ReadAlignment(alignmentLine, alignment);
    } catch (const Exception &e) {
      std::ostringstream oss;
      oss << "Failed to read alignment at line " << lineNum << ": ";
      oss << e.msg();
      Error(oss.str());
    }
    if (alignment.size() == 0) {
      std::cerr << "skipping line " << lineNum << " without alignment points\n";
      continue;
    }
    if (options.t2s) {
      FlipAlignment(alignment);
    }

    // Record word counts.
    if (!options.targetUnknownWordFile.empty()) {
      CollectWordLabelCounts(*targetParseTree, options, targetWordCount,
                             targetWordLabel);
    }

    // Record word counts: source side.
    if (options.sourceLabels && !options.sourceUnknownWordFile.empty()) {
      CollectWordLabelCounts(*sourceParseTree, options, sourceWordCount,
                             sourceWordLabel);
    }

    // Form an alignment graph from the target tree, source words, and
    // alignment.
    AlignmentGraph graph(targetParseTree.get(), sourceTokens, alignment);

    // Extract minimal rules, adding each rule to its root node's rule set.
    graph.ExtractMinimalRules(options);

    // Extract composed rules.
    if (!options.minimal) {
      graph.ExtractComposedRules(options);
    }

    // Initialize phrase orientation scoring object
    PhraseOrientation phraseOrientation(sourceTokens.size(),
                                        targetXmlTreeParser.words().size(), alignment);

    // Write the rules, subject to scope pruning.
    const std::vector<Node *> &targetNodes = graph.GetTargetNodes();
    for (std::vector<Node *>::const_iterator p = targetNodes.begin();
         p != targetNodes.end(); ++p) {

      const std::vector<const Subgraph *> &rules = (*p)->GetRules();

      PhraseOrientation::REO_CLASS l2rOrientation=PhraseOrientation::REO_CLASS_UNKNOWN, r2lOrientation=PhraseOrientation::REO_CLASS_UNKNOWN;
      if (options.phraseOrientation && !rules.empty()) {
        int sourceSpanBegin = *((*p)->GetSpan().begin());
        int sourceSpanEnd   = *((*p)->GetSpan().rbegin());
        l2rOrientation = phraseOrientation.GetOrientationInfo(sourceSpanBegin,sourceSpanEnd,PhraseOrientation::REO_DIR_L2R);
        r2lOrientation = phraseOrientation.GetOrientationInfo(sourceSpanBegin,sourceSpanEnd,PhraseOrientation::REO_DIR_R2L);
        // std::cerr << "span " << sourceSpanBegin << " " << sourceSpanEnd << std::endl;
        // std::cerr << "phraseOrientation " << phraseOrientation.GetOrientationInfo(sourceSpanBegin,sourceSpanEnd) << std::endl;
      }

      for (std::vector<const Subgraph *>::const_iterator q = rules.begin();
           q != rules.end(); ++q) {
        // STSG output.
        if (options.stsg) {
          StsgRule rule(**q);
          if (rule.Scope() <= options.maxScope) {
            stsgWriter.Write(rule);
          }
          continue;
        }
        // SCFG output.
        ScfgRule *r = 0;
        if (options.sourceLabels) {
          r = new ScfgRule(**q, &sourceXmlTreeParser.node_collection());
        } else {
          r = new ScfgRule(**q);
        }
        // TODO Can scope pruning be done earlier?
        if (r->Scope() <= options.maxScope) {
          scfgWriter.Write(*r,lineNum,false);
          if (options.treeFragments) {
            fwdExtractStream << " {{Tree ";
            (*q)->PrintTree(fwdExtractStream);
            fwdExtractStream << "}}";
          }
          if (options.partsOfSpeech) {
            fwdExtractStream << " {{POS";
            (*q)->PrintPartsOfSpeech(fwdExtractStream);
            fwdExtractStream << "}}";
          }
          if (options.phraseOrientation) {
            fwdExtractStream << " {{Orientation ";
            phraseOrientation.WriteOrientation(fwdExtractStream,l2rOrientation);
            fwdExtractStream << " ";
            phraseOrientation.WriteOrientation(fwdExtractStream,r2lOrientation);
            fwdExtractStream << "}}";
            phraseOrientation.IncrementPriorCount(PhraseOrientation::REO_DIR_L2R,l2rOrientation,1);
            phraseOrientation.IncrementPriorCount(PhraseOrientation::REO_DIR_R2L,r2lOrientation,1);
          }
          fwdExtractStream << std::endl;
          invExtractStream << std::endl;
        }
        delete r;
      }
    }
  }

  if (options.phraseOrientation) {
    std::string phraseOrientationPriorsFileName = options.extractFile + std::string(".phraseOrientationPriors");
    OutputFileStream phraseOrientationPriorsStream;
    OpenOutputFileOrDie(phraseOrientationPriorsFileName, phraseOrientationPriorsStream);
    PhraseOrientation::WritePriorCounts(phraseOrientationPriorsStream);
  }

  std::map<std::string,size_t> sourceLabels;
  if (options.sourceLabels && !options.sourceLabelSetFile.empty()) {
    std::set<std::string> extendedLabelSet = sourceXmlTreeParser.label_set();
    extendedLabelSet.insert("XLHS"); // non-matching label (left-hand side)
    extendedLabelSet.insert("XRHS"); // non-matching label (right-hand side)
    extendedLabelSet.insert("TOPLABEL");  // as used in the glue grammar
    extendedLabelSet.insert("SOMELABEL"); // as used in the glue grammar
    size_t index = 0;
    for (std::set<std::string>::const_iterator iter=extendedLabelSet.begin();
         iter!=extendedLabelSet.end(); ++iter, ++index) {
      sourceLabels.insert(std::pair<std::string,size_t>(*iter,index));
    }
    WriteSourceLabelSet(sourceLabels, sourceLabelSetStream);
  }

  std::set<std::string> strippedTargetLabelSet;
  std::map<std::string, int> strippedTargetTopLabelSet;
  if (options.stripBitParLabels &&
      (!options.glueGrammarFile.empty() || !options.unknownWordSoftMatchesFile.empty())) {
    StripBitParLabels(targetXmlTreeParser.label_set(),
                      targetXmlTreeParser.top_label_set(),
                      strippedTargetLabelSet, strippedTargetTopLabelSet);
  }

  if (!options.glueGrammarFile.empty()) {
    if (options.stripBitParLabels) {
      WriteGlueGrammar(strippedTargetLabelSet, strippedTargetTopLabelSet, sourceLabels, options, glueGrammarStream);
    } else {
      WriteGlueGrammar(targetXmlTreeParser.label_set(),
                       targetXmlTreeParser.top_label_set(),
                       sourceLabels, options, glueGrammarStream);
    }
  }

  if (!options.targetUnknownWordFile.empty()) {
    WriteUnknownWordLabel(targetWordCount, targetWordLabel, options, targetUnknownWordStream);
  }

  if (options.sourceLabels && !options.sourceUnknownWordFile.empty()) {
    WriteUnknownWordLabel(sourceWordCount, sourceWordLabel, options, sourceUnknownWordStream, true);
  }

  if (!options.unknownWordSoftMatchesFile.empty()) {
    if (options.stripBitParLabels) {
      WriteUnknownWordSoftMatches(strippedTargetLabelSet, unknownWordSoftMatchesStream);
    } else {
      WriteUnknownWordSoftMatches(targetXmlTreeParser.label_set(),
                                  unknownWordSoftMatchesStream);
    }
  }

  return 0;
}

void ExtractGHKM::ProcessOptions(int argc, char *argv[],
                                 Options &options) const
{
  namespace po = boost::program_options;
  namespace cls = boost::program_options::command_line_style;

  // Construct the 'top' of the usage message: the bit that comes before the
  // options list.
  std::ostringstream usageTop;
  usageTop << "Usage: " << name()
           << " [OPTION]... TARGET SOURCE ALIGNMENT EXTRACT\n\n"
           << "SCFG rule extractor based on the GHKM algorithm described in\n"
           << "Galley et al. (2004).\n\n"
           << "Options";

  // Construct the 'bottom' of the usage message.
  std::ostringstream usageBottom;
  usageBottom << "\nImplementation Notes:\n"
              << "\nThe parse tree is assumed to contain part-of-speech preterminal nodes.\n"
              << "\n"
              << "For the composed rule constraints: rule depth is the "
              "maximum distance from the\nrule's root node to a sink "
              "node, not counting preterminal expansions or word\n"
              "alignments.  Rule size is the measure defined in DeNeefe "
              "et al (2007): the\nnumber of non-part-of-speech, non-leaf "
              "constituent labels in the target tree.\nNode count is the "
              "number of target tree nodes (excluding target words).\n"
              << "\n"
              << "Scope pruning (Hopkins and Langmead, 2010) is applied to both minimal and\ncomposed rules.\n"
              << "\n"
              << "Unaligned source words are attached to the tree using the "
              "following heuristic:\nif there are aligned source words to "
              "both the left and the right of an unaligned\nsource word "
              "then it is attached to the lowest common ancestor of its "
              "nearest\nsuch left and right neighbours.  Otherwise, it is "
              "attached to the root of the\nparse tree.\n"
              << "\n"
              << "Unless the --AllowUnary option is given, unary rules containing no lexical\nsource items are eliminated using the method described in Chung et al. (2011).\nThe parsing algorithm used in Moses is unable to handle such rules.\n"
              << "\n"
              << "References:\n"
              << "Galley, M., Hopkins, M., Knight, K., and Marcu, D. (2004)\n"
              << "\"What's in a Translation Rule?\", In Proceedings of HLT/NAACL 2004.\n"
              << "\n"
              << "DeNeefe, S., Knight, K., Wang, W., and Marcu, D. (2007)\n"
              << "\"What Can Syntax-Based MT Learn from Phrase-Based MT?\", In Proceedings of\nEMNLP-CoNLL 2007.\n"
              << "\n"
              << "Hopkins, M. and Langmead, G. (2010)\n"
              << "\"SCFG Decoding Without Binarization\", In Proceedings of EMNLP 2010.\n"
              << "\n"
              << "Chung, T. and Fang, L. and Gildea, D. (2011)\n"
              << "\"Issues Concerning Decoding with Synchronous Context-free Grammar\", In\nProceedings of ACL/HLT 2011.";

  // Declare the command line options that are visible to the user.
  po::options_description visible(usageTop.str());
  visible.add_options()
  //("help", "print this help message and exit")
  ("AllowUnary",
   "allow fully non-lexical unary rules")
  ("ConditionOnTargetLHS",
   "write target LHS instead of \"X\" as source LHS")
  ("GlueGrammar",
   po::value(&options.glueGrammarFile),
   "write glue grammar to named file")
  ("GZOutput",
   "write gzipped extract files")
  ("IncludeSentenceId",
   "include sentence ID")
  ("MaxNodes",
   po::value(&options.maxNodes)->default_value(options.maxNodes),
   "set maximum number of tree nodes for composed rules")
  ("MaxRuleDepth",
   po::value(&options.maxRuleDepth)->default_value(options.maxRuleDepth),
   "set maximum depth for composed rules")
  ("MaxRuleSize",
   po::value(&options.maxRuleSize)->default_value(options.maxRuleSize),
   "set maximum size for composed rules")
  ("MaxScope",
   po::value(&options.maxScope)->default_value(options.maxScope),
   "set maximum allowed scope")
  ("Minimal",
   "extract minimal rules only")
  ("PartsOfSpeech",
   "output parts-of-speech as property (preterminals from the parse tree)")
  ("PartsOfSpeechFactor",
   "output parts-of-speech as factor (preterminals from the parse tree)")
  ("PCFG",
   "include score based on PCFG scores in target corpus")
  ("PhraseOrientation",
   "output phrase orientation information")
  ("StripBitParLabels",
   "strip suffix starting with a hyphen symbol (\"-\") from non-terminal labels")
  ("STSG",
   "output STSG rules (default is SCFG)")
  ("T2S",
   "enable tree-to-string rule extraction (string-to-tree is assumed by default)")
  ("TreeFragments",
   "output parse tree information")
  ("SourceLabels",
   "output source syntax label information")
  ("SourceLabelSet",
   po::value(&options.sourceLabelSetFile),
   "write source syntax label set to named file")
  ("SentenceOffset",
   po::value(&options.sentenceOffset)->default_value(options.sentenceOffset),
   "set sentence number offset if processing split corpus")
  ("UnknownWordLabel",
   po::value(&options.targetUnknownWordFile),
   "write unknown word labels to named file")
  ("SourceUnknownWordLabel",
   po::value(&options.sourceUnknownWordFile),
   "write source syntax unknown word labels to named file")
  ("UnknownWordMinRelFreq",
   po::value(&options.unknownWordMinRelFreq)->default_value(
     options.unknownWordMinRelFreq),
   "set minimum relative frequency for unknown word labels")
  ("UnknownWordSoftMatches",
   po::value(&options.unknownWordSoftMatchesFile),
   "write dummy value to unknown word label file, and mappings from dummy value to other labels to named file")
  ("UnknownWordUniform",
   "write uniform weights to unknown word label file")
  ("UnpairedExtractFormat",
   "do not pair non-terminals in extract files")
  ;

  // Declare the command line options that are hidden from the user
  // (these are used as positional options).
  po::options_description hidden("Hidden options");
  hidden.add_options()
  ("TargetFile",
   po::value(&options.targetFile),
   "target file")
  ("SourceFile",
   po::value(&options.sourceFile),
   "source file")
  ("AlignmentFile",
   po::value(&options.alignmentFile),
   "alignment file")
  ("ExtractFile",
   po::value(&options.extractFile),
   "extract file")
  ;

  // Compose the full set of command-line options.
  po::options_description cmdLineOptions;
  cmdLineOptions.add(visible).add(hidden);

  // Register the positional options.
  po::positional_options_description p;
  p.add("TargetFile", 1);
  p.add("SourceFile", 1);
  p.add("AlignmentFile", 1);
  p.add("ExtractFile", 1);

  // Process the command-line.
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).style(MosesOptionStyle()).
              options(cmdLineOptions).positional(p).run(), vm);
    po::notify(vm);
  } catch (const std::exception &e) {
    std::ostringstream msg;
    msg << e.what() << "\n\n" << visible << usageBottom.str();
    Error(msg.str());
  }

  if (vm.count("help")) {
    std::cout << visible << usageBottom.str() << std::endl;
    std::exit(0);
  }

  // Check all positional options were given.
  if (!vm.count("TargetFile") ||
      !vm.count("SourceFile") ||
      !vm.count("AlignmentFile") ||
      !vm.count("ExtractFile")) {
    std::ostringstream msg;
    std::cerr << visible << usageBottom.str() << std::endl;
    std::exit(1);
  }

  // Process Boolean options.
  if (vm.count("AllowUnary")) {
    options.allowUnary = true;
  }
  if (vm.count("ConditionOnTargetLHS")) {
    options.conditionOnTargetLhs = true;
  }
  if (vm.count("GZOutput")) {
    options.gzOutput = true;
  }
  if (vm.count("IncludeSentenceId")) {
    options.includeSentenceId = true;
  }
  if (vm.count("Minimal")) {
    options.minimal = true;
  }
  if (vm.count("PartsOfSpeech")) {
    options.partsOfSpeech = true;
  }
  if (vm.count("PartsOfSpeechFactor")) {
    options.partsOfSpeechFactor = true;
  }
  if (vm.count("PCFG")) {
    options.pcfg = true;
  }
  if (vm.count("PhraseOrientation")) {
    options.phraseOrientation = true;
  }
  if (vm.count("StripBitParLabels")) {
    options.stripBitParLabels = true;
  }
  if (vm.count("STSG")) {
    options.stsg = true;
  }
  if (vm.count("T2S")) {
    options.t2s = true;
  }
  if (vm.count("TreeFragments")) {
    options.treeFragments = true;
  }
  if (vm.count("SourceLabels")) {
    options.sourceLabels = true;
  }
  if (vm.count("UnknownWordUniform")) {
    options.unknownWordUniform = true;
  }
  if (vm.count("UnpairedExtractFormat")) {
    options.unpairedExtractFormat = true;
  }

  // Workaround for extract-parallel issue.
  if (options.sentenceOffset > 0) {
    options.targetUnknownWordFile.clear();
  }
  if (options.sentenceOffset > 0) {
    options.sourceUnknownWordFile.clear();
    options.unknownWordSoftMatchesFile.clear();
  }
}

std::vector<std::string> ExtractGHKM::ReadTokens(const std::string &s) const
{
  std::vector<std::string> tokens;

  std::string whitespace = " \t";

  std::string::size_type begin = s.find_first_not_of(whitespace);
  assert(begin != std::string::npos);
  while (true) {
    std::string::size_type end = s.find_first_of(whitespace, begin);
    std::string token;
    if (end == std::string::npos) {
      token = s.substr(begin);
    } else {
      token = s.substr(begin, end-begin);
    }
    tokens.push_back(token);
    if (end == std::string::npos) {
      break;
    }
    begin = s.find_first_not_of(whitespace, end);
    if (begin == std::string::npos) {
      break;
    }
  }

  return tokens;
}

void ExtractGHKM::WriteGlueGrammar(
  const std::set<std::string> &labelSet,
  const std::map<std::string, int> &topLabelSet,
  const std::map<std::string,size_t> &sourceLabels,
  const Options &options,
  std::ostream &out) const
{
  // choose a top label that is not already a label
  std::string topLabel = "QQQQQQ";
  for(size_t i = 1; i <= topLabel.length(); i++) {
    if (labelSet.find(topLabel.substr(0,i)) == labelSet.end() ) {
      topLabel = topLabel.substr(0,i);
      break;
    }
  }

  const size_t sourceLabelGlueTop = 0;
  const size_t sourceLabelGlueX = 1;
  const size_t sourceLabelSentenceStart = 2;
  const size_t sourceLabelSentenceEnd = 3;
//  const size_t partOfSpeechSentenceStart = 0;
//  const size_t partOfSpeechSentenceEnd = 1;

#ifndef BOS_
#define BOS_ "<s>" //Beginning of sentence symbol
#endif
#ifndef EOS_
#define EOS_ "</s>" //End of sentence symbol
#endif

  std::string sentenceStartSource = BOS_;
  std::string sentenceEndSource   = EOS_;
  std::string sentenceStartTarget = BOS_;
  std::string sentenceEndTarget   = EOS_;
  if (options.partsOfSpeech) {
    sentenceStartTarget = sentenceStartTarget + "|" + BOS_;
    sentenceEndTarget   = sentenceEndTarget   + "|" + EOS_;
  }
  if (options.partsOfSpeechFactor) {
    sentenceStartTarget = sentenceStartTarget + "|" + BOS_;
    sentenceEndTarget   = sentenceEndTarget   + "|" + EOS_;
  }

  // basic rules
  out << sentenceStartSource << " [X] ||| " << sentenceStartTarget << " [" << topLabel << "] ||| 1 ||| 0-0 ||| ||| |||";
  if (options.treeFragments) {
    out << " {{Tree [" << topLabel << " [SSTART <s>]]}}";
  }
//  if (options.partsOfSpeech) {
//    out << " {{POS " << partOfSpeechSentenceStart << "}}";
//  }
  if (options.sourceLabels) {
    out << " {{SourceLabels 2 1 " << sourceLabelSentenceStart << " 1 1 " << sourceLabelGlueTop << " 1}}";
  }
  if (options.phraseOrientation) {
    out << " {{Orientation 1 1 0.5 0.5 1 1 0.5 0.5}}";
  }
  out << std::endl;

  out << "[X][" << topLabel << "] " << sentenceEndSource << " [X] ||| [X][" << topLabel << "] " << sentenceEndTarget << " [" << topLabel << "] ||| 1 ||| 0-0 1-1 ||| ||| |||";
  if (options.treeFragments) {
    out << " {{Tree [" << topLabel << " [" << topLabel << "] [SEND </s>]]}}";
  }
//  if (options.partsOfSpeech) {
//    out << " {{POS " << partOfSpeechSentenceEnd << "}}";
//  }
  if (options.sourceLabels) {
    out << " {{SourceLabels 4 1 " << sourceLabelSentenceStart << " " << sourceLabelGlueTop << " " << sourceLabelSentenceEnd << " 1 1 " << sourceLabelGlueTop << " 1}}";
  }
  if (options.phraseOrientation) {
    out << " {{Orientation 1 1 0.5 0.5 1 1 0.5 0.5}}";
  }
  out << std::endl;

  // top rules
  for (std::map<std::string, int>::const_iterator i = topLabelSet.begin();
       i != topLabelSet.end(); ++i) {
    out << sentenceStartSource << " [X][" << i->first << "] " << sentenceEndSource << " [X] ||| " << sentenceStartTarget << " [X][" << i->first << "] " << sentenceEndTarget << " [" << topLabel << "] ||| 1 ||| 0-0 1-1 2-2 ||| ||| |||";
    if (options.treeFragments) {
      out << " {{Tree [" << topLabel << " [SSTART <s>] [" << i->first << "] [SEND </s>]]}}";
    }
//    if (options.partsOfSpeech) {
//      out << " {{POS " << partOfSpeechSentenceStart << " " << partOfSpeechSentenceEnd << "}}";
//    }
    if (options.sourceLabels) {
      out << " {{SourceLabels 4 1 " << sourceLabelSentenceStart << " " << sourceLabelGlueX << " " << sourceLabelSentenceEnd << " 1 1 " << sourceLabelGlueTop << " 1}}";
    }
    if (options.phraseOrientation) {
      out << " {{Orientation 1 1 0.5 0.5 1 1 0.5 0.5}}";
    }
    out << std::endl;
  }

  // glue rules
  for(std::set<std::string>::const_iterator i = labelSet.begin();
      i != labelSet.end(); i++ ) {
    out << "[X][" << topLabel << "] [X][" << *i << "] [X] ||| [X][" << topLabel << "] [X][" << *i << "] [" << topLabel << "] ||| 2.718 ||| 0-0 1-1 ||| ||| |||";
    if (options.treeFragments) {
      out << " {{Tree [" << topLabel << " ["<< topLabel << "] [" << *i << "]]}}";
    }
    if (options.sourceLabels) {
      out << " {{SourceLabels 3 1 " << sourceLabelGlueTop << " " << sourceLabelGlueX << " 1 1 " << sourceLabelGlueTop << " 1}}";
    }
    if (options.phraseOrientation) {
      out << " {{Orientation 1 1 0.5 0.5 1 1 0.5 0.5}}";
    }
    out << std::endl;
  }

  // glue rule for unknown word...
  out << "[X][" << topLabel << "] [X][X] [X] ||| [X][" << topLabel << "] [X][X] [" << topLabel << "] ||| 2.718 ||| 0-0 1-1 ||| ||| |||";
  if (options.treeFragments) {
    out << " {{Tree [" << topLabel << " [" << topLabel << "] [X]]}}";
  }
  if (options.sourceLabels) {
    out << " {{SourceLabels 3 1 " << sourceLabelGlueTop << " " << sourceLabelGlueX << " 1 1 " << sourceLabelGlueTop << " 1}}";
  }
  if (options.phraseOrientation) {
    out << " {{Orientation 1 1 0.5 0.5 1 1 0.5 0.5}}";
  }
  out << std::endl;
}

void ExtractGHKM::WriteSourceLabelSet(
  const std::map<std::string,size_t> &sourceLabels,
  std::ostream &out) const
{
  out << sourceLabels.size() << std::endl;
  for (std::map<std::string,size_t>::const_iterator iter=sourceLabels.begin();
       iter!=sourceLabels.end(); ++iter) {
    out << iter->first << " " << iter->second << std::endl;
  }
}

void ExtractGHKM::CollectWordLabelCounts(
  SyntaxTree &root,
  const Options &options,
  std::map<std::string, int> &wordCount,
  std::map<std::string, std::string> &wordLabel)
{
  for (SyntaxTree::ConstLeafIterator p(root);
       p != SyntaxTree::ConstLeafIterator(); ++p) {
    const SyntaxTree &leaf = *p;
    const std::string &word = leaf.value().label;
    const SyntaxTree *ancestor = leaf.parent();
    // If unary rule elimination is enabled and this word is at the end of a
    // chain of unary rewrites, e.g.
    //    PN-SB -> NE -> word
    // then record the constituent label at the top of the chain instead of
    // the part-of-speech label.
    while (!options.allowUnary &&
           ancestor->parent() &&
           ancestor->parent()->children().size() == 1) {
      ancestor = ancestor->parent();
    }
    const std::string &label = ancestor->value().label;
    ++wordCount[word];
    wordLabel[word] = label;
  }
}

std::vector<std::string> ExtractGHKM::ReadTokens(const SyntaxTree &root) const
{
  std::vector<std::string> tokens;
  for (SyntaxTree::ConstLeafIterator p(root);
       p != SyntaxTree::ConstLeafIterator(); ++p) {
    const SyntaxTree &leaf = *p;
    const std::string &word = leaf.value().label;
    tokens.push_back(word);
  }
  return tokens;
}

void ExtractGHKM::WriteUnknownWordLabel(
  const std::map<std::string, int> &wordCount,
  const std::map<std::string, std::string> &wordLabel,
  const Options &options,
  std::ostream &out,
  bool writeCounts) const
{
  if (!options.unknownWordSoftMatchesFile.empty()) {
    out << "UNK 1" << std::endl;
    return;
  }

  std::map<std::string, int> labelCount;
  int total = 0;
  for (std::map<std::string, int>::const_iterator p = wordCount.begin();
       p != wordCount.end(); ++p) {
    // Only consider singletons.
    if (p->second == 1) {
      std::map<std::string, std::string>::const_iterator q =
        wordLabel.find(p->first);
      assert(q != wordLabel.end());
      if (options.stripBitParLabels) {
        size_t pos = q->second.find('-');
        if (pos == std::string::npos) {
          ++labelCount[q->second];
        } else {
          ++labelCount[q->second.substr(0,pos)];
        }
      } else {
        ++labelCount[q->second];
      }
      ++total;
    }
  }
  if ( writeCounts ) {
    for (std::map<std::string, int>::const_iterator p = labelCount.begin();
         p != labelCount.end(); ++p) {
      out << p->first << " " << p->second << std::endl;
    }
  } else {
    for (std::map<std::string, int>::const_iterator p = labelCount.begin();
         p != labelCount.end(); ++p) {
      double ratio = static_cast<double>(p->second) / static_cast<double>(total);
      if (ratio >= options.unknownWordMinRelFreq) {
        float weight = options.unknownWordUniform ? 1.0f : ratio;
        out << p->first << " " << weight << std::endl;
      }
    }
  }
}

void ExtractGHKM::WriteUnknownWordSoftMatches(
  const std::set<std::string> &labelSet,
  std::ostream &out) const
{
  for (std::set<std::string>::const_iterator p = labelSet.begin(); p != labelSet.end(); ++p) {
    std::string label = *p;
    out << "UNK " << label << std::endl;
  }
}

void ExtractGHKM::StripBitParLabels(
  const std::set<std::string> &labelSet,
  const std::map<std::string, int> &topLabelSet,
  std::set<std::string> &outLabelSet,
  std::map<std::string, int> &outTopLabelSet) const
{
  for (std::set<std::string>::const_iterator it=labelSet.begin();
       it!=labelSet.end(); ++it) {
    size_t pos = it->find('-');
    if (pos == std::string::npos) {
      outLabelSet.insert(*it);
    } else {
      outLabelSet.insert(it->substr(0,pos));
    }
  }
  for (std::map<std::string,int>::const_iterator it=topLabelSet.begin();
       it!=topLabelSet.end(); ++it) {
    size_t pos = it->first.find('-');
    std::string stripped;
    if (pos == std::string::npos) {
      stripped = it->first;
    } else {
      stripped = it->first.substr(0,pos);
    }
    std::map<std::string, int>::iterator found=outTopLabelSet.find(stripped);
    if (found != outTopLabelSet.end()) {
      found->second += it->second;
    } else {
      outTopLabelSet.insert(std::pair<std::string,int>(stripped,it->second));
    }
  }
}

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining
