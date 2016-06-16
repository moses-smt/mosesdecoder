#include "probing_hash_utils.hh"

namespace Moses2
{

//Read table from disk, return memory map location
char * readTable(const char * filename, size_t size)
{
  //Initial position of the file is the end of the file, thus we know the size
  int fd;
  char * map;

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("Error opening file for reading");
    exit(EXIT_FAILURE);
  }

  map = (char *) mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);

  if (map == MAP_FAILED) {
    close(fd);
    perror("Error mmapping the file");
    exit(EXIT_FAILURE);
  }

  return map;
}

void serialize_table(char *mem, size_t size, const std::string &filename)
{
  std::ofstream os(filename.c_str(), std::ios::binary);
  os.write((const char*) &mem[0], size);
  os.close();

}

uint64_t getKey(const uint64_t source_phrase[], size_t size)
{
  //TOO SLOW
  //uint64_t key = util::MurmurHashNative(&source_phrase[0], source_phrase.size());
  uint64_t key = 0;
  for (size_t i = 0; i < size; i++) {
    key += (source_phrase[i] << i);
  }
  return key;
}

}

