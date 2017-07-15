#include <sys/stat.h>
#include <boost/foreach.hpp>
#include "line_splitter.h"
#include "storing.h"
#include "StoreTarget.h"
#include "StoreVocab.h"
#include "moses2/legacy/Util2.h"
#include "InputFileStream.h"

using namespace std;

namespace probingpt
{

///////////////////////////////////////////////////////////////////////
void Node::Add(Table &table, const SourcePhrase &sourcePhrase, size_t pos)
{
  if (pos < sourcePhrase.size()) {
    uint64_t vocabId = sourcePhrase[pos];

    Node *child;
    Children::iterator iter = m_children.find(vocabId);
    if (iter == m_children.end()) {
      // New node. Write other children then discard them
      BOOST_FOREACH(Children::value_type &valPair, m_children) {
        Node &otherChild = valPair.second;
        otherChild.Write(table);
      }
      m_children.clear();

      // create new node
      child = &m_children[vocabId];
      assert(!child->done);
      child->key = key + (vocabId << pos);
    } else {
      child = &iter->second;
    }

    child->Add(table, sourcePhrase, pos + 1);
  } else {
    // this node was written previously 'cos it has rules
    done = true;
  }
}

void Node::Write(Table &table)
{
  //cerr << "START write " << done << " " << key << endl;
  BOOST_FOREACH(Children::value_type &valPair, m_children) {
    Node &child = valPair.second;
    child.Write(table);
  }

  if (!done) {
    // save
    Entry sourceEntry;
    sourceEntry.value = NONE;
    sourceEntry.key = key;

    //Put into table
    table.Insert(sourceEntry);
  }
}

///////////////////////////////////////////////////////////////////////
void createProbingPT(const std::string &phrasetable_path,
                     const std::string &basepath, int num_scores, int num_lex_scores,
                     bool log_prob, int max_cache_size, bool scfg)
{
#if defined(_WIN32) || defined(_WIN64)
  std::cerr << "Create not implemented for Windows" << std::endl;
#else
  std::cerr << "Starting..." << std::endl;

  //Get basepath and create directory if missing
  mkdir(basepath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  StoreTarget storeTarget(basepath);

  //Get uniq lines:
  unsigned long uniq_entries = countUniqueSource(phrasetable_path);

  //Source phrase vocabids
  StoreVocab<uint64_t> sourceVocab(basepath + "/source_vocabids");

  //Read the file
  util::FilePiece filein(phrasetable_path.c_str());

  //Init the probing hash table
  size_t size = Table::Size(uniq_entries, 1.2);
  char * mem = new char[size];
  memset(mem, 0, size);
  Table sourceEntries(mem, size);

  std::priority_queue<CacheItem*, std::vector<CacheItem*>, CacheItemOrderer> cache;
  float totalSourceCount = 0;

  //Keep track of the size of each group of target phrases
  size_t line_num = 0;

  //Read everything and processs
  std::string prevSource;

  Node sourcePhrases;
  sourcePhrases.done = true;
  sourcePhrases.key = 0;

  while (true) {
    try {
      //Process line read
      line_text line;
      line = splitLine(filein.ReadLine(), scfg);
      //cerr << "line=" << line.source_phrase << endl;

      ++line_num;
      if (line_num % 1000000 == 0) {
        std::cerr << line_num << " " << std::flush;
      }

      //Add source phrases to vocabularyIDs
      add_to_map(sourceVocab, line.source_phrase);

      if (prevSource.empty()) {
        // 1st line
        prevSource = line.source_phrase.as_string();
        storeTarget.Append(line, log_prob, scfg);
      } else if (prevSource == line.source_phrase) {
        //If we still have the same line, just append to it:
        storeTarget.Append(line, log_prob, scfg);
      } else {
        assert(prevSource != line.source_phrase);

        //Create a new entry even

        // save
        uint64_t targetInd = storeTarget.Save();

        // next line
        storeTarget.Append(line, log_prob, scfg);

        //Create an entry for the previous source phrase:
        Entry sourceEntry;
        sourceEntry.value = targetInd;
        //The key is the sum of hashes of individual words bitshifted by their position in the phrase.
        //Probably not entirerly correct, but fast and seems to work fine in practise.
        std::vector<uint64_t> vocabid_source = getVocabIDs(prevSource);
        if (scfg) {
          // storing prefixes?
          sourcePhrases.Add(sourceEntries, vocabid_source);
        }
        sourceEntry.key = getKey(vocabid_source);

        /*
        cerr << "prevSource=" << prevSource << flush
            << " vocabids=" << Debug(vocabid_source) << flush
            << " key=" << sourceEntry.key << endl;
        */
        //Put into table
        sourceEntries.Insert(sourceEntry);

        // update cache - CURRENT source phrase, not prev
        if (max_cache_size) {
          std::string countStr = line.counts.as_string();
          countStr = Moses2::Trim(countStr);
          if (!countStr.empty()) {
            std::vector<float> toks = Moses2::Tokenize<float>(countStr);
            //cerr << "CACHE:" << line.source_phrase << " " << countStr << " " << toks[1] << endl;

            if (toks.size() >= 2) {
              totalSourceCount += toks[1];

              // compute key for CURRENT source
              std::vector<uint64_t> currVocabidSource = getVocabIDs(line.source_phrase.as_string());
              uint64_t currKey = getKey(currVocabidSource);

              CacheItem *item = new CacheItem(
                Moses2::Trim(line.source_phrase.as_string()),
                currKey,
                toks[1]);
              cache.push(item);

              if (max_cache_size > 0 && cache.size() > max_cache_size) {
                cache.pop();
              }
            }
          }
        }

        //Set prevLine
        prevSource = line.source_phrase.as_string();
      }

    } catch (util::EndOfFileException e) {
      std::cerr
          << "Reading phrase table finished, writing remaining files to disk."
          << std::endl;

      //After the final entry is constructed we need to add it to the phrase_table
      //Create an entry for the previous source phrase:
      uint64_t targetInd = storeTarget.Save();

      Entry sourceEntry;
      sourceEntry.value = targetInd;

      //The key is the sum of hashes of individual words. Probably not entirerly correct, but fast
      std::vector<uint64_t> vocabid_source = getVocabIDs(prevSource);
      sourceEntry.key = getKey(vocabid_source);

      //Put into table
      sourceEntries.Insert(sourceEntry);

      break;
    }
  }

  sourcePhrases.Write(sourceEntries);

  storeTarget.SaveAlignment();

  serialize_table(mem, size, (basepath + "/probing_hash.dat"));

  sourceVocab.Save();

  serialize_cache(cache, (basepath + "/cache"), totalSourceCount);

  delete[] mem;

  //Write configfile
  std::ofstream configfile;
  configfile.open((basepath + "/config").c_str());
  configfile << "API_VERSION\t" << API_VERSION << '\n';
  configfile << "uniq_entries\t" << uniq_entries << '\n';
  configfile << "num_scores\t" << num_scores << '\n';
  configfile << "num_lex_scores\t" << num_lex_scores << '\n';
  configfile << "log_prob\t" << log_prob << '\n';
  configfile.close();
#endif
}

size_t countUniqueSource(const std::string &path)
{
  size_t ret = 0;
  InputFileStream strme(path);

  std::string line, prevSource;
  while (std::getline(strme, line)) {
    std::vector<std::string> toks = Moses2::TokenizeMultiCharSeparator(line, "|||");
    assert(toks.size() != 0);

    if (prevSource != toks[0]) {
      prevSource = toks[0];
      ++ret;
    }
  }

  return ret;
}

void serialize_cache(
  std::priority_queue<CacheItem*, std::vector<CacheItem*>, CacheItemOrderer> &cache,
  const std::string &path, float totalSourceCount)
{
  std::vector<const CacheItem*> vec(cache.size());

  size_t ind = cache.size() - 1;
  while (!cache.empty()) {
    const CacheItem *item = cache.top();
    vec[ind] = item;
    cache.pop();
    --ind;
  }

  std::ofstream os(path.c_str());

  os << totalSourceCount << std::endl;
  for (size_t i = 0; i < vec.size(); ++i) {
    const CacheItem *item = vec[i];
    os << item->count << "\t" << item->sourceKey << "\t" << item->source << std::endl;
    delete item;
  }

  os.close();
}

uint64_t getKey(const std::vector<uint64_t> &vocabid_source)
{
  return probingpt::getKey(vocabid_source.data(), vocabid_source.size());
}

std::vector<uint64_t> CreatePrefix(const std::vector<uint64_t> &vocabid_source, size_t endPos)
{
  assert(endPos < vocabid_source.size());

  std::vector<uint64_t> ret(endPos + 1);
  for (size_t i = 0; i <= endPos; ++i) {
    ret[i] = vocabid_source[i];
  }
  return ret;
}

}

