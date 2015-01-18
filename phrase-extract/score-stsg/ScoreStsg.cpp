#include "ScoreStsg.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

#include <boost/program_options.hpp>

#include "util/string_piece.hh"
#include "util/string_piece_hash.hh"
#include "util/tokenize_piece.hh"

#include "InputFileStream.h"
#include "OutputFileStream.h"

#include "syntax-common/exception.h"

#include "LexicalTable.h"
#include "Options.h"
#include "RuleGroup.h"
#include "RuleTableWriter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

const int ScoreStsg::kCountOfCountsMax = 10;

ScoreStsg::ScoreStsg()
  : m_name("score-stsg")
  , m_lexTable(m_srcVocab, m_tgtVocab)
  , m_countOfCounts(kCountOfCountsMax, 0)
  , m_totalDistinct(0)
{
}

int ScoreStsg::Main(int argc, char *argv[])
{
  // Process command-line options.
  ProcessOptions(argc, argv, m_options);

  // Open input files.
  Moses::InputFileStream extractStream(m_options.extractFile);
  Moses::InputFileStream lexStream(m_options.lexFile);

  // Open output files.
  Moses::OutputFileStream outStream;
  Moses::OutputFileStream countOfCountsStream;
  OpenOutputFileOrDie(m_options.tableFile, outStream);
  if (m_options.goodTuring || m_options.kneserNey) {
    OpenOutputFileOrDie(m_options.tableFile+".coc", countOfCountsStream);
  }

  // Load lexical table.
  if (!m_options.noLex) {
    m_lexTable.Load(lexStream);
  }

  const util::MultiCharacter delimiter("|||");
  std::size_t lineNum = 0;
  std::size_t startLine= 0;
  std::string line;
  std::string tmp;
  RuleGroup ruleGroup;
  RuleTableWriter ruleTableWriter(m_options, outStream);

  while (std::getline(extractStream, line)) {
    ++lineNum;

    // Tokenize the input line.
    util::TokenIter<util::MultiCharacter> it(line, delimiter);
    StringPiece source = *it++;
    StringPiece target = *it++;
    StringPiece ntAlign = *it++;
    StringPiece fullAlign = *it++;
    it->CopyToString(&tmp);
    int count = std::atoi(tmp.c_str());
    double treeScore = 0.0f;
    if (m_options.treeScore && !m_options.inverse) {
      ++it;
      it->CopyToString(&tmp);
      treeScore = std::atof(tmp.c_str());
    }

    // If this is the first line or if source has changed since the last
    // line then process the current rule group and start a new one.
    if (source != ruleGroup.GetSource()) {
      if (lineNum > 1) {
        ProcessRuleGroupOrDie(ruleGroup, ruleTableWriter, startLine, lineNum-1);
      }
      startLine = lineNum;
      ruleGroup.SetNewSource(source);
    }

    // Add the rule to the current rule group.
    ruleGroup.AddRule(target, ntAlign, fullAlign, count, treeScore);
  }

  // Process the final rule group.
  ProcessRuleGroupOrDie(ruleGroup, ruleTableWriter, startLine, lineNum);

  // Write count of counts file.
  if (m_options.goodTuring || m_options.kneserNey) {
    // Kneser-Ney needs the total number of distinct rules.
    countOfCountsStream << m_totalDistinct << std::endl;
    // Write out counts of counts.
    for (int i = 1; i <= kCountOfCountsMax; ++i) {
      countOfCountsStream << m_countOfCounts[i] << std::endl;
    }
  }

  return 0;
}

void ScoreStsg::TokenizeRuleHalf(const std::string &s, TokenizedRuleHalf &half)
{
  // Copy s to half.string, but strip any leading or trailing whitespace.
  std::size_t start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    throw Exception("rule half is empty");
  }
  std::size_t end = s.find_last_not_of(" \t");
  assert(end != std::string::npos);
  half.string = s.substr(start, end-start+1);

  // Tokenize half.string.
  half.tokens.clear();
  for (TreeFragmentTokenizer p(half.string);
       p != TreeFragmentTokenizer(); ++p) {
    half.tokens.push_back(*p);
  }

  // Extract the frontier symbols.
  half.frontierSymbols.clear();
  const std::size_t numTokens = half.tokens.size();
  for (int i = 0; i < numTokens; ++i) {
    if (half.tokens[i].type != TreeFragmentToken_WORD) {
      continue;
    }
    if (i == 0 || half.tokens[i-1].type != TreeFragmentToken_LSB) {
      // A word is a terminal iff it doesn't follow '['
      half.frontierSymbols.resize(half.frontierSymbols.size()+1);
      half.frontierSymbols.back().value = half.tokens[i].value;
      half.frontierSymbols.back().isNonTerminal = false;
    } else if (i+1 < numTokens &&
               half.tokens[i+1].type == TreeFragmentToken_RSB) {
      // A word is a non-terminal iff it it follows '[' and is succeeded by ']'
      half.frontierSymbols.resize(half.frontierSymbols.size()+1);
      half.frontierSymbols.back().value = half.tokens[i].value;
      half.frontierSymbols.back().isNonTerminal = true;
      ++i;  // Skip over the ']'
    }
  }
}

void ScoreStsg::ProcessRuleGroupOrDie(const RuleGroup &group,
                                      RuleTableWriter &writer,
                                      std::size_t start,
                                      std::size_t end)
{
  try {
    ProcessRuleGroup(group, writer);
  } catch (const Exception &e) {
    std::ostringstream msg;
    msg << "failed to process rule group at lines " << start << "-" << end
        << ": " << e.msg();
    Error(msg.str());
  } catch (const std::exception &e) {
    std::ostringstream msg;
    msg << "failed to process rule group at lines " << start << "-" << end
        << ": " << e.what();
    Error(msg.str());
  }
}

void ScoreStsg::ProcessRuleGroup(const RuleGroup &group,
                                 RuleTableWriter &writer)
{
  const std::size_t totalCount = group.GetTotalCount();
  const std::size_t distinctCount = group.GetSize();

  TokenizeRuleHalf(group.GetSource(), m_sourceHalf);

  const bool fullyLexical = m_sourceHalf.IsFullyLexical();

  // Process each distinct rule in turn.
  for (RuleGroup::ConstIterator p = group.Begin(); p != group.End(); ++p) {
    const RuleGroup::DistinctRule &rule = *p;

    // Update count of count statistics.
    if (m_options.goodTuring || m_options.kneserNey) {
      ++m_totalDistinct;
      int countInt = rule.count + 0.99999;
      if (countInt <= kCountOfCountsMax) {
        ++m_countOfCounts[countInt];
      }
    }

    // If the rule is not fully lexical then discard it if the count is below
    // the threshold value.
    if (!fullyLexical && rule.count < m_options.minCountHierarchical) {
      continue;
    }

    TokenizeRuleHalf(rule.target, m_targetHalf);

    // Find the most frequent alignment (if there's a tie, take the first one).
    std::vector<std::pair<std::string, int> >::const_iterator q =
      rule.alignments.begin();
    const std::pair<std::string, int> *bestAlignmentAndCount = &(*q++);
    for (; q != rule.alignments.end(); ++q) {
      if (q->second > bestAlignmentAndCount->second) {
        bestAlignmentAndCount = &(*q);
      }
    }
    const std::string &bestAlignment = bestAlignmentAndCount->first;
    ParseAlignmentString(bestAlignment, m_targetHalf.frontierSymbols.size(),
                         m_tgtToSrc);

    // Compute the lexical translation probability.
    double lexProb = ComputeLexProb(m_sourceHalf.frontierSymbols,
                                    m_targetHalf.frontierSymbols, m_tgtToSrc);

    // Write a line to the rule table.
    writer.WriteLine(m_sourceHalf, m_targetHalf, bestAlignment, lexProb,
                     rule.treeScore, p->count, totalCount, distinctCount);
  }
}

void ScoreStsg::ParseAlignmentString(const std::string &s, int numTgtWords,
                                     ALIGNMENT &tgtToSrc)
{
  tgtToSrc.clear();
  tgtToSrc.resize(numTgtWords);

  const std::string digits = "0123456789";

  std::string::size_type begin = 0;
  while (true) {
    std::string::size_type end = s.find("-", begin);
    if (end == std::string::npos) {
      return;
    }
    int src = std::atoi(s.substr(begin, end-begin).c_str());
    if (end+1 == s.size()) {
      throw Exception("Target index missing");
    }
    begin = end+1;
    end = s.find_first_not_of(digits, begin+1);
    int tgt;
    if (end == std::string::npos) {
      tgt = std::atoi(s.substr(begin).c_str());
      tgtToSrc[tgt].insert(src);
      return;
    } else {
      tgt = std::atoi(s.substr(begin, end-begin).c_str());
      tgtToSrc[tgt].insert(src);
    }
    begin = end+1;
  }
}

double ScoreStsg::ComputeLexProb(const std::vector<RuleSymbol> &sourceFrontier,
                                 const std::vector<RuleSymbol> &targetFrontier,
                                 const ALIGNMENT &tgtToSrc)
{
  double lexScore = 1.0;
  for (std::size_t i = 0; i < targetFrontier.size(); ++i) {
    if (targetFrontier[i].isNonTerminal) {
      continue;
    }
    Vocabulary::IdType tgtId = m_tgtVocab.Lookup(targetFrontier[i].value,
                               StringPieceCompatibleHash(),
                               StringPieceCompatibleEquals());
    const std::set<std::size_t> &srcIndices = tgtToSrc[i];
    if (srcIndices.empty()) {
      // Explain unaligned word by NULL.
      lexScore *= m_lexTable.PermissiveLookup(Vocabulary::NullId(), tgtId);
    } else {
      double thisWordScore = 0.0;
      for (std::set<std::size_t>::const_iterator p = srcIndices.begin();
           p != srcIndices.end(); ++p) {
        Vocabulary::IdType srcId =
          m_srcVocab.Lookup(sourceFrontier[*p].value,
                            StringPieceCompatibleHash(),
                            StringPieceCompatibleEquals());
        thisWordScore += m_lexTable.PermissiveLookup(srcId, tgtId);
      }
      lexScore *= thisWordScore / static_cast<double>(srcIndices.size());
    }
  }
  return lexScore;
}

void ScoreStsg::OpenOutputFileOrDie(const std::string &filename,
                                    Moses::OutputFileStream &stream)
{
  bool ret = stream.Open(filename);
  if (!ret) {
    std::ostringstream msg;
    msg << "failed to open output file: " << filename;
    Error(msg.str());
  }
}

void ScoreStsg::ProcessOptions(int argc, char *argv[], Options &options) const
{
  namespace po = boost::program_options;
  namespace cls = boost::program_options::command_line_style;

  // Construct the 'top' of the usage message: the bit that comes before the
  // options list.
  std::ostringstream usageTop;
  usageTop << "Usage: " << GetName()
           << " [OPTION]... EXTRACT LEX TABLE\n\n"
           << "STSG rule scorer\n\n"
           << "Options";

  // Construct the 'bottom' of the usage message.
  std::ostringstream usageBottom;
  usageBottom << "TODO";

  // Declare the command line options that are visible to the user.
  po::options_description visible(usageTop.str());
  visible.add_options()
  ("GoodTuring",
   "apply Good-Turing smoothing to relative frequency probability estimates")
  ("Hierarchical",
   "ignored (included for compatibility with score)")
  ("Inverse",
   "use inverse mode")
  ("KneserNey",
   "apply Kneser-Ney smoothing to relative frequency probability estimates")
  ("LogProb",
   "output log probabilities")
  ("MinCountHierarchical",
   po::value(&options.minCountHierarchical)->
   default_value(options.minCountHierarchical),
   "filter out rules with frequency < arg (except fully lexical rules)")
  ("NegLogProb",
   "output negative log probabilities")
  ("NoLex",
   "do not compute lexical translation score")
  ("NoWordAlignment",
   "do not output word alignments")
  ("PCFG",
   "synonym for TreeScore (included for compatibility with score)")
  ("TreeScore",
   "include pre-computed tree score from extract")
  ("UnpairedExtractFormat",
   "ignored (included for compatibility with score)")
  ;

  // Declare the command line options that are hidden from the user
  // (these are used as positional options).
  po::options_description hidden("Hidden options");
  hidden.add_options()
  ("ExtractFile",
   po::value(&options.extractFile),
   "extract file")
  ("LexFile",
   po::value(&options.lexFile),
   "lexical probability file")
  ("TableFile",
   po::value(&options.tableFile),
   "output file")
  ;

  // Compose the full set of command-line options.
  po::options_description cmdLineOptions;
  cmdLineOptions.add(visible).add(hidden);

  // Register the positional options.
  po::positional_options_description p;
  p.add("ExtractFile", 1);
  p.add("LexFile", 1);
  p.add("TableFile", 1);

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
  if (!vm.count("ExtractFile") ||
      !vm.count("LexFile") ||
      !vm.count("TableFile")) {
    std::ostringstream msg;
    std::cerr << visible << usageBottom.str() << std::endl;
    std::exit(1);
  }

  // Process Boolean options.
  if (vm.count("GoodTuring")) {
    options.goodTuring = true;
  }
  if (vm.count("Inverse")) {
    options.inverse = true;
  }
  if (vm.count("KneserNey")) {
    options.kneserNey = true;
  }
  if (vm.count("LogProb")) {
    options.logProb = true;
  }
  if (vm.count("NegLogProb")) {
    options.negLogProb = true;
  }
  if (vm.count("NoLex")) {
    options.noLex = true;
  }
  if (vm.count("NoWordAlignment")) {
    options.noWordAlignment = true;
  }
  if (vm.count("TreeScore") || vm.count("PCFG")) {
    options.treeScore = true;
  }
}

void ScoreStsg::Error(const std::string &msg) const
{
  std::cerr << GetName() << ": " << msg << std::endl;
  std::exit(1);
}

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining
