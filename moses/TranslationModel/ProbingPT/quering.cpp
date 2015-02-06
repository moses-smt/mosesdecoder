#include "quering.hh"

unsigned char * read_binary_file(const char * filename, size_t filesize)
{
  //Get filesize
  int fd;
  unsigned char * map;

  fd = open(filename, O_RDONLY);

  if (fd == -1) {
    perror("Error opening file for reading");
    exit(EXIT_FAILURE);
  }

  map = (unsigned char *)mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    close(fd);
    perror("Error mmapping the file");
    exit(EXIT_FAILURE);
  }

  return map;
}

QueryEngine::QueryEngine(const char * filepath) : decoder(filepath)
{

  //Create filepaths
  std::string basepath(filepath);
  std::string path_to_hashtable = basepath + "/probing_hash.dat";
  std::string path_to_data_bin = basepath + "/binfile.dat";
  std::string path_to_source_vocabid = basepath + "/source_vocabids";

  ///Source phrase vocabids
  read_map(&source_vocabids, path_to_source_vocabid.c_str());

  //Target phrase vocabIDs
  vocabids = decoder.get_target_lookup_map();

  //Read config file
  std::string line;
  std::ifstream config ((basepath + "/config").c_str());
  //Check API version:
  getline(config, line);
  if (atoi(line.c_str()) != API_VERSION) {
    std::cerr << "The ProbingPT API has changed, please rebinarize your phrase tables." << std::endl;
    exit(EXIT_FAILURE);
  }
  //Get tablesize.
  getline(config, line);
  int tablesize = atoi(line.c_str());
  //Number of scores
  getline(config, line);
  num_scores = atoi(line.c_str());
  //do we have a reordering table
  getline(config, line);
  std::transform(line.begin(), line.end(), line.begin(), ::tolower); //Get the boolean in lowercase
  is_reordering = false;
  if (line == "true") {
    is_reordering = true;
    std::cerr << "WARNING. REORDERING TABLES NOT SUPPORTED YET." << std::endl;
  }
  config.close();

  //Mmap binary table
  struct stat filestatus;
  stat(path_to_data_bin.c_str(), &filestatus);
  binary_filesize = filestatus.st_size;
  binary_mmaped = read_binary_file(path_to_data_bin.c_str(), binary_filesize);

  //Read hashtable
  size_t table_filesize = Table::Size(tablesize, 1.2);
  mem = readTable(path_to_hashtable.c_str(), table_filesize);
  Table table_init(mem, table_filesize);
  table = table_init;

  std::cerr << "Initialized successfully! " << std::endl;
}

QueryEngine::~QueryEngine()
{
  //Clear mmap content from memory.
  munmap(binary_mmaped, binary_filesize);
  munmap(mem, table_filesize);

}

std::pair<bool, std::vector<target_text> > QueryEngine::query(std::vector<uint64_t> source_phrase)
{
  bool found;
  std::vector<target_text> translation_entries;
  const Entry * entry;
  //TOO SLOW
  //uint64_t key = util::MurmurHashNative(&source_phrase[0], source_phrase.size());
  uint64_t key = 0;
  for (int i = 0; i < source_phrase.size(); i++) {
    key += (source_phrase[i] << i);
  }


  found = table.Find(key, entry);

  if (found) {
    //The phrase that was searched for was found! We need to get the translation entries.
    //We will read the largest entry in bytes and then filter the unnecesarry with functions
    //from line_splitter
    uint64_t initial_index = entry -> GetValue();
    unsigned int bytes_toread = entry -> bytes_toread;

    //ASK HIEU FOR MORE EFFICIENT WAY TO DO THIS!
    std::vector<unsigned char> encoded_text; //Assign to the vector the relevant portion of the array.
    encoded_text.reserve(bytes_toread);
    for (int i = 0; i < bytes_toread; i++) {
      encoded_text.push_back(binary_mmaped[i+initial_index]);
    }

    //Get only the translation entries necessary
    translation_entries = decoder.full_decode_line(encoded_text, num_scores);

  }

  std::pair<bool, std::vector<target_text> > output (found, translation_entries);

  return output;

}

std::pair<bool, std::vector<target_text> > QueryEngine::query(StringPiece source_phrase)
{
  bool found;
  std::vector<target_text> translation_entries;
  const Entry * entry;
  //Convert source frase to VID
  std::vector<uint64_t> source_phrase_vid = getVocabIDs(source_phrase);
  //TOO SLOW
  //uint64_t key = util::MurmurHashNative(&source_phrase_vid[0], source_phrase_vid.size());
  uint64_t key = 0;
  for (int i = 0; i < source_phrase_vid.size(); i++) {
    key += (source_phrase_vid[i] << i);
  }

  found = table.Find(key, entry);


  if (found) {
    //The phrase that was searched for was found! We need to get the translation entries.
    //We will read the largest entry in bytes and then filter the unnecesarry with functions
    //from line_splitter
    uint64_t initial_index = entry -> GetValue();
    unsigned int bytes_toread = entry -> bytes_toread;
    //At the end of the file we can't readd + largest_entry cause we get a segfault.
    std::cerr << "Entry size is bytes is: " << bytes_toread << std::endl;

    //ASK HIEU FOR MORE EFFICIENT WAY TO DO THIS!
    std::vector<unsigned char> encoded_text; //Assign to the vector the relevant portion of the array.
    encoded_text.reserve(bytes_toread);
    for (int i = 0; i < bytes_toread; i++) {
      encoded_text.push_back(binary_mmaped[i+initial_index]);
    }

    //Get only the translation entries necessary
    translation_entries = decoder.full_decode_line(encoded_text, num_scores);

  }

  std::pair<bool, std::vector<target_text> > output (found, translation_entries);

  return output;

}

void QueryEngine::printTargetInfo(std::vector<target_text> target_phrases)
{
  int entries = target_phrases.size();

  for (int i = 0; i<entries; i++) {
    std::cout << "Entry " << i+1 << " of " << entries << ":" << std::endl;
    //Print text
    std::cout << getTargetWordsFromIDs(target_phrases[i].target_phrase, &vocabids) << "\t";

    //Print probabilities:
    for (int j = 0; j<target_phrases[i].prob.size(); j++) {
      std::cout << target_phrases[i].prob[j] << " ";
    }
    std::cout << "\t";

    //Print word_all1
    for (int j = 0; j<target_phrases[i].word_all1.size(); j++) {
      if (j%2 == 0) {
        std::cout << (short)target_phrases[i].word_all1[j] << "-";
      } else {
        std::cout << (short)target_phrases[i].word_all1[j] << " ";
      }
    }
    std::cout << std::endl;
  }
}
