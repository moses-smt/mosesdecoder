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

#include "Alignment.h"
#include "AlignmentGraph.h"
#include "Exception.h"
#include "InputFileStream.h"
#include "Node.h"
#include "OutputFileStream.h"
#include "Options.h"
#include "ParseTree.h"
#include "ScfgRule.h"
#include "ScfgRuleWriter.h"
#include "Span.h"
#include "XmlTreeParser.h"

#include <boost/program_options.hpp>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

namespace Moses
{
namespace GHKM
{

int ExtractGHKM::Main(int argc, char *argv[])
{
  // Process command-line options.
  Options options;
  ProcessOptions(argc, argv, options);

  // Open input files.
  InputFileStream targetStream(options.targetFile);
  InputFileStream sourceStream(options.sourceFile);
  InputFileStream alignmentStream(options.alignmentFile);

  // Open output files.
  OutputFileStream fwdExtractStream;
  OutputFileStream invExtractStream;
  std::ofstream glueGrammarStream;
  std::ofstream unknownWordStream;
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
  if (!options.unknownWordFile.empty()) {
    OpenOutputFileOrDie(options.unknownWordFile, unknownWordStream);
  }

  // Target label sets for producing glue grammar.
  std::set<std::string> labelSet;
  std::map<std::string, int> topLabelSet;

  // Word count statistics for producing unknown word labels.
  std::map<std::string, int> wordCount;
  std::map<std::string, std::string> wordLabel;

  std::string targetLine;
  std::string sourceLine;
  std::string alignmentLine;
  Alignment alignment;
  XmlTreeParser xmlTreeParser(labelSet, topLabelSet);
  ScfgRuleWriter writer(fwdExtractStream, invExtractStream, options);
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
    std::auto_ptr<ParseTree> t;
    try {
      t = xmlTreeParser.Parse(targetLine);
      assert(t.get());
    } catch (const Exception &e) {
      std::ostringstream s;
      s << "Failed to parse XML tree at line " << lineNum;
      if (!e.GetMsg().empty()) {
        s << ": " << e.GetMsg();
      }
      Error(s.str());
    }

    // Read source tokens.
    std::vector<std::string> sourceTokens(ReadTokens(sourceLine));

    // Read word alignments.
    try {
      ReadAlignment(alignmentLine, alignment);
    } catch (const Exception &e) {
      std::ostringstream s;
      s << "Failed to read alignment at line " << lineNum << ": ";
      s << e.GetMsg();
      Error(s.str());
    }
    if (alignment.size() == 0) {
      std::cerr << "skipping line " << lineNum << " without alignment points\n";
      continue;
    }

    // Record word counts.
    if (!options.unknownWordFile.empty()) {
      CollectWordLabelCounts(*t, options, wordCount, wordLabel);
    }

    // Form an alignment graph from the target tree, source words, and
    // alignment.
    AlignmentGraph graph(t.get(), sourceTokens, alignment);

    // Extract minimal rules, adding each rule to its root node's rule set.
    graph.ExtractMinimalRules(options);

    // Extract composed rules.
    if (!options.minimal) {
      graph.ExtractComposedRules(options);
    }

    // Write the rules, subject to scope pruning.
    const std::vector<Node *> &targetNodes = graph.GetTargetNodes();
    for (std::vector<Node *>::const_iterator p = targetNodes.begin();
         p != targetNodes.end(); ++p) {
      const std::vector<const Subgraph *> &rules = (*p)->GetRules();
      for (std::vector<const Subgraph *>::const_iterator q = rules.begin();
           q != rules.end(); ++q) {
        ScfgRule r(**q);
        // TODO Can scope pruning be done earlier?
        if (r.Scope() <= options.maxScope) {
          if (!options.treeFragments) {
            writer.Write(r);
          } else {
            writer.Write(r,**q);
          }
        }
      }
    }
  }

  if (!options.glueGrammarFile.empty()) {
    WriteGlueGrammar(labelSet, topLabelSet, glueGrammarStream);
  }

  if (!options.unknownWordFile.empty()) {
    WriteUnknownWordLabel(wordCount, wordLabel, options, unknownWordStream);
  }

  return 0;
}

void ExtractGHKM::OpenInputFileOrDie(const std::string &filename,
                                     std::ifstream &stream)
{
  stream.open(filename.c_str());
  if (!stream) {
    std::ostringstream msg;
    msg << "failed to open input file: " << filename;
    Error(msg.str());
  }
}

void ExtractGHKM::OpenOutputFileOrDie(const std::string &filename,
                                      std::ofstream &stream)
{
  stream.open(filename.c_str());
  if (!stream) {
    std::ostringstream msg;
    msg << "failed to open output file: " << filename;
    Error(msg.str());
  }
}

void ExtractGHKM::OpenOutputFileOrDie(const std::string &filename,
                                      OutputFileStream &stream)
{
  bool ret = stream.Open(filename);
  if (!ret) {
    std::ostringstream msg;
    msg << "failed to open output file: " << filename;
    Error(msg.str());
  }
}

void ExtractGHKM::ProcessOptions(int argc, char *argv[],
                                 Options &options) const
{
  namespace po = boost::program_options;
  namespace cls = boost::program_options::command_line_style;

  // Construct the 'top' of the usage message: the bit that comes before the
  // options list.
  std::ostringstream usageTop;
  usageTop << "Usage: " << GetName()
           << " [OPTION]... TARGET SOURCE ALIGNMENT EXTRACT\n\n"
           << "SCFG rule extractor based on the GHKM algorithm described in\n"
           << "Galley et al. (2004).\n\n"
           << "Options";

  // Construct the 'bottom' of the usage message.
  std::ostringstream usageBottom;
  usageBottom << "\nImplementation Notes:\n"
              << "\nThe parse tree is assumed to contain part-of-speech preterminal nodes.\n"
              << "\n"
              << "For the composed rule constraints: rule depth is the maximum distance from the\nrule's root node to a sink node, not counting preterminal expansions or word\nalignments.  Rule size is the measure defined in DeNeefe et al (2007): the\nnumber of non-part-of-speech, non-leaf constituent labels in the target tree.\nNode count is the number of target tree nodes (excluding target words).\n"
              << "\n"
              << "Scope pruning (Hopkins and Langmead, 2010) is applied to both minimal and\ncomposed rules.\n"
              << "\n"
              << "Unaligned source words are attached to the tree using the following heuristic:\nif there are aligned source words to both the left and the right of an unaligned\nsource word then it is attached to the lowest common ancestor of its nearest\nsuch left and right neighbours.  Otherwise, it is attached to the root of the\nparse tree.\n"
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
  ("PCFG",
   "include score based on PCFG scores in target corpus")
  ("TreeFragments",
   "output parse tree information")
  ("SentenceOffset",
   po::value(&options.sentenceOffset)->default_value(options.sentenceOffset),
   "set sentence number offset if processing split corpus")
  ("UnknownWordLabel",
   po::value(&options.unknownWordFile),
   "write unknown word labels to named file")
  ("UnknownWordMinRelFreq",
   po::value(&options.unknownWordMinRelFreq)->default_value(
     options.unknownWordMinRelFreq),
   "set minimum relative frequency for unknown word labels")
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
  const int optionStyle = cls::allow_long
                          | cls::long_allow_adjacent
                          | cls::long_allow_next;
  try {
    po::store(po::command_line_parser(argc, argv).style(optionStyle).
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
  if (vm.count("Minimal")) {
    options.minimal = true;
  }
  if (vm.count("PCFG")) {
    options.pcfg = true;
  }
  if (vm.count("TreeFragments")) {
    options.treeFragments = true;
  }
  if (vm.count("UnknownWordUniform")) {
    options.unknownWordUniform = true;
  }
  if (vm.count("UnpairedExtractFormat")) {
    options.unpairedExtractFormat = true;
  }

  // Workaround for extract-parallel issue.
  if (options.sentenceOffset > 0) {
    options.unknownWordFile.clear();
  }
}

void ExtractGHKM::Error(const std::string &msg) const
{
  std::cerr << GetName() << ": " << msg << std::endl;
  std::exit(1);
}

std::vector<std::string> ExtractGHKM::ReadTokens(const std::string &s)
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
  std::ostream &out)
{
  // chose a top label that is not already a label
  std::string topLabel = "QQQQQQ";
  for(size_t i = 1; i <= topLabel.length(); i++) {
    if (labelSet.find(topLabel.substr(0,i)) == labelSet.end() ) {
      topLabel = topLabel.substr(0,i);
      break;
    }
  }

  // basic rules
  out << "<s> [X] ||| <s> [" << topLabel << "] ||| 1 ||| ||| ||| ||| {{Tree [" << topLabel << " <s>]}}" << std::endl;
  out << "[X][" << topLabel << "] </s> [X] ||| [X][" << topLabel << "] </s> [" << topLabel << "] ||| 1 ||| 0-0 ||| ||| ||| {{Tree [" << topLabel << " [" << topLabel << "] </s>]}}" << std::endl;

  // top rules
  for (std::map<std::string, int>::const_iterator i = topLabelSet.begin();
       i != topLabelSet.end(); ++i) {
    out << "<s> [X][" << i->first << "] </s> [X] ||| <s> [X][" << i->first << "] </s> [" << topLabel << "] ||| 1 ||| 1-1 ||| ||| ||| {{Tree [" << topLabel << " <s> [" << i->first << "] </s>]}}" << std::endl;
  }

  // glue rules
  for(std::set<std::string>::const_iterator i = labelSet.begin();
      i != labelSet.end(); i++ ) {
    out << "[X][" << topLabel << "] [X][" << *i << "] [X] ||| [X][" << topLabel << "] [X][" << *i << "] [" << topLabel << "] ||| 2.718 ||| 0-0 1-1 ||| ||| ||| {{Tree [" << topLabel << " ["<< topLabel << "] [" << *i << "]]}}" << std::endl;
  }
  // glue rule for unknown word...
  out << "[X][" << topLabel << "] [X][X] [X] ||| [X][" << topLabel << "] [X][X] [" << topLabel << "] ||| 2.718 ||| 0-0 1-1 ||| ||| ||| {{Tree [" << topLabel << " [" << topLabel << "] [X]]}}" << std::endl;
}

void ExtractGHKM::CollectWordLabelCounts(
  ParseTree &root,
  const Options &options,
  std::map<std::string, int> &wordCount,
  std::map<std::string, std::string> &wordLabel)
{
  std::vector<const ParseTree*> leaves;
  root.GetLeaves(std::back_inserter(leaves));
  for (std::vector<const ParseTree *>::const_iterator p = leaves.begin();
       p != leaves.end(); ++p) {
    const ParseTree &leaf = **p;
    const std::string &word = leaf.GetLabel();
    const ParseTree *ancestor = leaf.GetParent();
    // If unary rule elimination is enabled and this word is at the end of a
    // chain of unary rewrites, e.g.
    //    PN-SB -> NE -> word
    // then record the constituent label at the top of the chain instead of
    // the part-of-speech label.
    while (!options.allowUnary &&
           ancestor->GetParent() &&
           ancestor->GetParent()->GetChildren().size() == 1) {
      ancestor = ancestor->GetParent();
    }
    const std::string &label = ancestor->GetLabel();
    ++wordCount[word];
    wordLabel[word] = label;
  }
}

void ExtractGHKM::WriteUnknownWordLabel(
  const std::map<std::string, int> &wordCount,
  const std::map<std::string, std::string> &wordLabel,
  const Options &options,
  std::ostream &out)
{
  std::map<std::string, int> labelCount;
  int total = 0;
  for (std::map<std::string, int>::const_iterator p = wordCount.begin();
       p != wordCount.end(); ++p) {
    // Only consider singletons.
    if (p->second == 1) {
      std::map<std::string, std::string>::const_iterator q =
        wordLabel.find(p->first);
      assert(q != wordLabel.end());
      ++labelCount[q->second];
      ++total;
    }
  }
  for (std::map<std::string, int>::const_iterator p = labelCount.begin();
       p != labelCount.end(); ++p) {
    double ratio = static_cast<double>(p->second) / static_cast<double>(total);
    if (ratio >= options.unknownWordMinRelFreq) {
      float weight = options.unknownWordUniform ? 1.0f : ratio;
      out << p->first << " " << weight << std::endl;
    }
  }
}

}  // namespace GHKM
}  // namespace Moses
