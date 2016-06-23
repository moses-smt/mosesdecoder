#include <sys/stat.h>
#include <boost/unordered_set.hpp>
#include "line_splitter.hh"
#include "storing.hh"
#include "StoreTarget.h"
#include "StoreVocab.h"
#include "../Util2.h"
#include "../InputFileStream.h"

using namespace std;

namespace Moses2
{

void createProbingPT(const std::string &phrasetable_path,
    const std::string &basepath, int num_scores, int num_lex_scores,
    bool log_prob, int max_cache_size, bool scfg)
{
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
  std::vector<uint64_t> prevVocabid_source;

  while (true) {
    try {
      //Process line read
      line_text line;
      line = splitLine(filein.ReadLine(), scfg);

      ++line_num;
      if (line_num % 1000000 == 0) {
        std::cerr << line_num << " " << std::flush;
      }

      //Add source phrases to vocabularyIDs
      add_to_map(sourceVocab, line.source_phrase);

      if (prevSource.empty()) {
        // 1st line
        prevSource = line.source_phrase.as_string();
        storeTarget.Append(line, log_prob);
      }
      else if (prevSource == line.source_phrase) {
        //If we still have the same line, just append to it:
        storeTarget.Append(line, log_prob);
      }
      else {
        assert(prevSource != line.source_phrase);

        //Create a new entry even

        // save
        uint64_t targetInd = storeTarget.Save();

        // next line
        storeTarget.Append(line, log_prob);

        //Create an entry for the previous source phrase:
        Entry sourceEntry;
        sourceEntry.value = targetInd;
        //The key is the sum of hashes of individual words bitshifted by their position in the phrase.
        //Probably not entirerly correct, but fast and seems to work fine in practise.
        std::vector<uint64_t> vocabid_source = getVocabIDs(prevSource);
        if (scfg) {
          // don't store the last non-term in the source phrase
          vocabid_source.erase(vocabid_source.begin() + vocabid_source.size() - 1);

          InsertPrefixes(vocabid_source, prevVocabid_source, sourceEntries);
        }
        sourceEntry.key = getKey(vocabid_source);

        //Put into table
        sourceEntries.Insert(sourceEntry);

        // update cache
        if (max_cache_size) {
          std::string countStr = line.counts.as_string();
          countStr = Trim(countStr);
          if (!countStr.empty()) {
            std::vector<float> toks = Tokenize<float>(countStr);

            if (toks.size() >= 2) {
              totalSourceCount += toks[1];
              CacheItem *item = new CacheItem(
                  Trim(line.source_phrase.as_string()), toks[1]);
              cache.push(item);

              if (max_cache_size > 0 && cache.size() > max_cache_size) {
                cache.pop();
              }
            }
          }
        }

        //Set prevLine
        prevSource = line.source_phrase.as_string();

        if (scfg){
          prevVocabid_source = vocabid_source;
        }
      }

    }
    catch (util::EndOfFileException e) {
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
}

size_t countUniqueSource(const std::string &path)
{
  size_t ret = 0;
  InputFileStream strme(path);

  std::string line, prevSource;
  while (std::getline(strme, line)) {
    std::vector<std::string> toks = TokenizeMultiCharSeparator(line, "|||");
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
    os << item->count << "\t" << item->source << std::endl;
    delete item;
  }

  os.close();
}

uint64_t getKey(const std::vector<uint64_t> &vocabid_source)
{
  return Moses2::getKey(vocabid_source.data(), vocabid_source.size());
}

void InsertPrefixes(
    const std::vector<uint64_t> &vocabid_source,
    const std::vector<uint64_t> &prevVocabid_source,
    Table &sourceEntries)
{
  size_t minSize = std::min(vocabid_source.size(), prevVocabid_source.size());
  size_t startPos = prevVocabid_source.size();

  for (size_t i = 0; i < minSize; ++i) {
    if (vocabid_source[i] != prevVocabid_source[i]) {
      startPos = i + 1;
      break;
    }
  }

  // loop through each prefix
  /*
  cerr << endl;
  cerr << "prev=" << Debug(prevVocabid_source) << endl;
  cerr << "curr=" << Debug(vocabid_source) << endl;
  cerr << "startPos=" << startPos << endl;
  */
  for (size_t i = startPos; i < vocabid_source.size() - 1; ++i) {
    std::vector<uint64_t> prefix = CreatePrefix(vocabid_source, i);
    //cerr << "pref=" << Debug(prefix) << endl;

    // save
    Entry sourceEntry;
    sourceEntry.value = NONE;
    sourceEntry.key = getKey(prefix);

    //Put into table
    sourceEntries.Insert(sourceEntry);

  }

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

