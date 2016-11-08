#include "querying.hh"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

QueryEngine::QueryEngine(const char * filepath)
{

  //Create filepaths
  std::string basepath(filepath);
  std::string path_to_config = basepath + "/config";
  std::string path_to_hashtable = basepath + "/probing_hash.dat";
  std::string path_to_source_vocabid = basepath + "/source_vocabids";
  std::string alignPath = basepath + "/Alignments.dat";

  if (!FileExists(path_to_config)) {
    UTIL_THROW2("Binary table doesn't exist is didn't finish binarizing: " << path_to_config);
  }

  ///Source phrase vocabids
  read_map(source_vocabids, path_to_source_vocabid.c_str());

  // alignments
  read_alignments(alignPath);

  //Read config file
  boost::unordered_map<std::string, std::string> keyValue;

  std::ifstream config(path_to_config.c_str());
  std::string line;
  while (getline(config, line)) {
    std::vector<std::string> toks = Tokenize(line, "\t");
    UTIL_THROW_IF2(toks.size() != 2, "Wrong config format:" << line);
    keyValue[ toks[0] ] = toks[1];
  }

  bool found;
  //Check API version:
  int version;
  found = Get(keyValue, "API_VERSION", version);
  if (!found) {
    std::cerr << "Old or corrupted version of ProbingPT. Please rebinarize your phrase tables." << std::endl;
  } else if (version != API_VERSION) {
    std::cerr << "The ProbingPT API has changed. " << version << "!="
              << API_VERSION << " Please rebinarize your phrase tables." << std::endl;
    exit(EXIT_FAILURE);
  }

  //Get tablesize.
  int tablesize;
  found = Get(keyValue, "uniq_entries", tablesize);
  if (!found) {
    std::cerr << "uniq_entries not found" << std::endl;
    exit(EXIT_FAILURE);
  }

  //Number of scores
  found = Get(keyValue, "num_scores", num_scores);
  if (!found) {
    std::cerr << "num_scores not found" << std::endl;
    exit(EXIT_FAILURE);
  }

  //How may scores from lex reordering models
  found = Get(keyValue, "num_lex_scores", num_lex_scores);
  if (!found) {
    std::cerr << "num_lex_scores not found" << std::endl;
    exit(EXIT_FAILURE);
  }

  // have the scores been log() and FloorScore()?
  found = Get(keyValue, "log_prob", logProb);
  if (!found) {
    std::cerr << "logProb not found" << std::endl;
    exit(EXIT_FAILURE);
  }

  config.close();

  //Read hashtable
  table_filesize = Table::Size(tablesize, 1.2);
  mem = readTable(path_to_hashtable.c_str(), table_filesize);
  Table table_init(mem, table_filesize);
  table = table_init;

  std::cerr << "Initialized successfully! " << std::endl;
}

QueryEngine::~QueryEngine()
{
  //Clear mmap content from memory.
  munmap(mem, table_filesize);

}

uint64_t QueryEngine::getKey(uint64_t source_phrase[], size_t size) const
{
  //TOO SLOW
  //uint64_t key = util::MurmurHashNative(&source_phrase[0], source_phrase.size());
  return Moses::getKey(source_phrase, size);
}

std::pair<bool, uint64_t> QueryEngine::query(uint64_t key)
{
  std::pair<bool, uint64_t> ret;

  const Entry * entry;
  ret.first = table.Find(key, entry);
  if (ret.first) {
    ret.second = entry->value;
  }
  return ret;
}

void QueryEngine::read_alignments(const std::string &alignPath)
{
  std::ifstream strm(alignPath.c_str());

  string line;
  while (getline(strm, line)) {
    vector<string> toks = Tokenize(line, "\t ");
    UTIL_THROW_IF2(toks.size() == 0, "Corrupt alignment file");

    uint32_t alignInd = Scan<uint32_t>(toks[0]);
    if (alignInd >= alignColl.size()) {
      alignColl.resize(alignInd + 1);
    }

    Alignments &aligns = alignColl[alignInd];
    for (size_t i = 1; i < toks.size(); ++i) {
      size_t pos = Scan<size_t>(toks[i]);
      aligns.push_back(pos);
    }
  }
}

}

