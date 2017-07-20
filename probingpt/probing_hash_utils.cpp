#include <iostream>
#include "probing_hash_utils.h"
#include "util/file.hh"

namespace probingpt
{

//Read table from disk, return memory map location
char * readTable(const char * filename, util::LoadMethod load_method, util::scoped_fd &file, util::scoped_memory &memory)
{
  //std::cerr << "filename=" << filename << std::endl;
  file.reset(util::OpenReadOrThrow(filename));
  uint64_t total_size_ = util::SizeFile(file.get());

  MapRead(load_method, file.get(), 0, total_size_, memory);

  return (char*) memory.get();
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

