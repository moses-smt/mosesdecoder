#include <sys/stat.h>
#include "storing.hh"
#include "StoreTarget.h"
#include "../Util2.h"
#include "../InputFileStream.h"

namespace Moses2
{

BinaryFileWriter::BinaryFileWriter (std::string basepath) : os ((basepath + "/binfile.dat").c_str(), std::ios::binary)
{
  binfile.reserve(10000); //Reserve part of the vector to avoid realocation
  it = binfile.begin();
  dist_from_start = 0; //Initialize variables
  extra_counter = 0;
}

void BinaryFileWriter::write (std::vector<unsigned char> * bytes)
{
  binfile.insert(it, bytes->begin(), bytes->end()); //Insert the bytes
  //Keep track of the offsets
  it += bytes->size();
  dist_from_start = distance(binfile.begin(),it);
  //Flush the vector to disk every once in a while so that we don't consume too much ram
  if (dist_from_start > 9000) {
    flush();
  }
}

void BinaryFileWriter::flush ()
{
  //Cast unsigned char to char before writing...
  os.write((char *)&binfile[0], dist_from_start);
  //Clear the vector:
  binfile.clear();
  binfile.reserve(10000);
  extra_counter += dist_from_start; //Keep track of the total number of bytes.
  it = binfile.begin(); //Reset iterator
  dist_from_start = distance(binfile.begin(),it); //Reset dist from start
}

BinaryFileWriter::~BinaryFileWriter ()
{
  os.close();
  binfile.clear();
}

void createProbingPT(
		const std::string &phrasetable_path,
		const std::string &basepath,
        int num_scores,
		int num_lex_scores,
		bool log_prob,
		int max_cache_size)
{
  std::cerr << "Starting..." << std::endl;

  //Get basepath and create directory if missing
  mkdir(basepath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  //Set up huffman and serialize decoder maps.
  Huffman huffmanEncoder(phrasetable_path.c_str()); //initialize
  huffmanEncoder.assign_values();
  huffmanEncoder.produce_lookups();
  huffmanEncoder.serialize_maps(basepath.c_str());

  StoreTarget storeTarget(basepath);

  //Get uniq lines:
  unsigned long uniq_entries = countUniqueSource(phrasetable_path);

  //Source phrase vocabids
  std::map<uint64_t, std::string> source_vocabids;

  //Read the file
  util::FilePiece filein(phrasetable_path.c_str());

  //Init the probing hash table
  size_t size = Table::Size(uniq_entries, 1.2);
  char * mem = new char[size];
  memset(mem, 0, size);
  Table table(mem, size);

  BinaryFileWriter binfile(basepath); //Init the binary file writer.

  std::priority_queue<CacheItem*, std::vector<CacheItem*>, CacheItemOrderer> cache;
  float totalSourceCount = 0;

  //Keep track of the size of each group of target phrases
  uint64_t entrystartidx = 0;
  size_t line_num = 0;

  //Read everything and processs
  std::string prevSource;
  while(true) {
    try {
      //Process line read
      line_text line;
      line = splitLine(filein.ReadLine());

      ++line_num;
      if (line_num % 1000000 == 0) {
    	  std::cerr << line_num << " " << std::flush;
      }

      //Add source phrases to vocabularyIDs
      add_to_map(&source_vocabids, line.source_phrase);

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
        Entry pesho;
        pesho.value = entrystartidx;
        pesho.targetInd = targetInd;
        //The key is the sum of hashes of individual words bitshifted by their position in the phrase.
        //Probably not entirerly correct, but fast and seems to work fine in practise.
        pesho.key = 0;
        std::vector<uint64_t> vocabid_source = getVocabIDs(prevSource);
        for (int i = 0; i < vocabid_source.size(); i++) {
          pesho.key += (vocabid_source[i] << i);
        }
        pesho.bytes_toread = binfile.dist_from_start + binfile.extra_counter - entrystartidx;

        //Put into table
        table.Insert(pesho);

        entrystartidx = binfile.dist_from_start + binfile.extra_counter; //Designate start idx for new entry

        //Encode a line and write it to disk.
        std::vector<unsigned char> encoded_line = huffmanEncoder.full_encode_line(line, log_prob);
        binfile.write(&encoded_line);

        // update cache
        if (max_cache_size) {
			std::string countStr = line.counts.as_string();
			countStr = Trim(countStr);
			if (!countStr.empty()) {
				std::vector<float> toks = Tokenize<float>(countStr);

				if (toks.size() >= 2) {
					totalSourceCount += toks[1];
					CacheItem *item = new CacheItem(Trim(line.source_phrase.as_string()), toks[1]);
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
      std::cerr << "Reading phrase table finished, writing remaining files to disk." << std::endl;
      binfile.flush();

      //After the final entry is constructed we need to add it to the phrase_table
      //Create an entry for the previous source phrase:
      uint64_t targetInd = storeTarget.Save();

      Entry pesho;
      pesho.value = entrystartidx;
      pesho.targetInd = targetInd;

      //The key is the sum of hashes of individual words. Probably not entirerly correct, but fast
      pesho.key = 0;
      std::vector<uint64_t> vocabid_source = getVocabIDs(prevSource);
      for (int i = 0; i < vocabid_source.size(); i++) {
        pesho.key += (vocabid_source[i] << i);
      }
      pesho.bytes_toread = binfile.dist_from_start + binfile.extra_counter - entrystartidx;
      //Put into table
      table.Insert(pesho);

      break;
    }
  }

  serialize_table(mem, size, (basepath + "/probing_hash.dat"));

  serialize_map(&source_vocabids, (basepath + "/source_vocabids"));

  serialize_cache(cache, (basepath + "/cache"), totalSourceCount);

  delete[] mem;

  //Write configfile
  std::ofstream configfile;
  configfile.open((basepath + "/config").c_str());
  configfile << API_VERSION << '\n';
  configfile << uniq_entries << '\n';
  configfile << num_scores << '\n';
  configfile << num_lex_scores << '\n';
  configfile << log_prob << '\n';
  configfile.close();
}

size_t countUniqueSource(const std::string &path)
{
	size_t ret = 0;
	InputFileStream strme(path);

	std::string line, prevSource;
	while (std::getline(strme, line)) {
		std::vector<std::string> toks = TokenizeMultiCharSeparator(line, "|||");
		assert(toks.size() == 0);

		if (prevSource != toks[0]) {
			prevSource = toks[0];
			++ret;
		}
	}

	return ret;
}

void serialize_cache(std::priority_queue<CacheItem*, std::vector<CacheItem*>, CacheItemOrderer> &cache,
		const std::string &path,
		float totalSourceCount)
{
  std::vector<const CacheItem*> vec(cache.size());

  size_t ind = cache.size() - 1;
  while (!cache.empty()) {
	  const CacheItem *item = cache.top();
	  vec[ind] = item;
	  cache.pop();
	  --ind;
  }

  std::ofstream os (path.c_str());

  os << totalSourceCount << std::endl;
  for (size_t i = 0; i < vec.size(); ++i) {
	  const CacheItem *item = vec[i];
	  os << item->count << "\t" << item->source << std::endl;
	  delete item;
  }

  os.close();
}

}

