#include "quering.hh"

namespace Moses2
{

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

QueryEngine::QueryEngine(const char * filepath)
{

  //Create filepaths
  std::string basepath(filepath);
  std::string path_to_hashtable = basepath + "/probing_hash.dat";
  std::string path_to_data_bin = basepath + "/binfile.dat";
  std::string path_to_source_vocabid = basepath + "/source_vocabids";

  ///Source phrase vocabids
  read_map(&source_vocabids, path_to_source_vocabid.c_str());

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
  //How may scores from lex reordering models
  getline(config, line);
  num_lex_scores = atoi(line.c_str());
  // have the scores been log() and FloorScore()?
  getline(config, line);
  logProb = atoi(line.c_str());

  config.close();

  //Mmap binary table
  struct stat filestatus;
  stat(path_to_data_bin.c_str(), &filestatus);
  binary_filesize = filestatus.st_size;
  binary_mmaped = read_binary_file(path_to_data_bin.c_str(), binary_filesize);

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
  munmap(binary_mmaped, binary_filesize);
  munmap(mem, table_filesize);

}

uint64_t QueryEngine::getKey(uint64_t source_phrase[], size_t size) const
{
  //TOO SLOW
  //uint64_t key = util::MurmurHashNative(&source_phrase[0], source_phrase.size());
  uint64_t key = 0;
  for (size_t i = 0; i < size; i++) {
	key += (source_phrase[i] << i);
  }
  return key;
}

std::pair<bool, uint64_t> QueryEngine::query(uint64_t key)
{
	std::pair<bool, uint64_t> ret;

  const Entry * entry;
  ret.first = table.Find(key, entry);
  ret.second = entry->targetInd;
  return ret;
}

}

