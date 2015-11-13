#include "storing.hh"

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

void createProbingPT(const char * phrasetable_path, const char * target_path,
                     const char * num_scores, const char * is_reordering)
{
  //Get basepath and create directory if missing
  std::string basepath(target_path);
  mkdir(basepath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  //Set up huffman and serialize decoder maps.
  Huffman huffmanEncoder(phrasetable_path); //initialize
  huffmanEncoder.assign_values();
  huffmanEncoder.produce_lookups();
  huffmanEncoder.serialize_maps(target_path);

  //Get uniq lines:
  unsigned long uniq_entries = huffmanEncoder.getUniqLines();

  //Source phrase vocabids
  std::map<uint64_t, std::string> source_vocabids;

  //Read the file
  util::FilePiece filein(phrasetable_path);

  //Init the probing hash table
  size_t size = Table::Size(uniq_entries, 1.2);
  char * mem = new char[size];
  memset(mem, 0, size);
  Table table(mem, size);

  BinaryFileWriter binfile(basepath); //Init the binary file writer.

  line_text prev_line; //Check if the source phrase of the previous line is the same

  //Keep track of the size of each group of target phrases
  uint64_t entrystartidx = 0;
  //uint64_t line_num = 0;


  //Read everything and processs
  while(true) {
    try {
      //Process line read
      line_text line;
      line = splitLine(filein.ReadLine());
      //Add source phrases to vocabularyIDs
      add_to_map(&source_vocabids, line.source_phrase);

      if ((binfile.dist_from_start + binfile.extra_counter) == 0) {
        prev_line = line; //For the first iteration assume the previous line is
      } //The same as this one.

      if (line.source_phrase != prev_line.source_phrase) {

        //Create a new entry even

        //Create an entry for the previous source phrase:
        Entry pesho;
        pesho.value = entrystartidx;
        //The key is the sum of hashes of individual words bitshifted by their position in the phrase.
        //Probably not entirerly correct, but fast and seems to work fine in practise.
        pesho.key = 0;
        std::vector<uint64_t> vocabid_source = getVocabIDs(prev_line.source_phrase);
        for (int i = 0; i < vocabid_source.size(); i++) {
          pesho.key += (vocabid_source[i] << i);
        }
        pesho.bytes_toread = binfile.dist_from_start + binfile.extra_counter - entrystartidx;

        //Put into table
        table.Insert(pesho);

        entrystartidx = binfile.dist_from_start + binfile.extra_counter; //Designate start idx for new entry

        //Encode a line and write it to disk.
        std::vector<unsigned char> encoded_line = huffmanEncoder.full_encode_line(line);
        binfile.write(&encoded_line);

        //Set prevLine
        prev_line = line;

      } else {
        //If we still have the same line, just append to it:
        std::vector<unsigned char> encoded_line = huffmanEncoder.full_encode_line(line);
        binfile.write(&encoded_line);
      }

    } catch (util::EndOfFileException e) {
      std::cerr << "Reading phrase table finished, writing remaining files to disk." << std::endl;
      binfile.flush();

      //After the final entry is constructed we need to add it to the phrase_table
      //Create an entry for the previous source phrase:
      Entry pesho;
      pesho.value = entrystartidx;
      //The key is the sum of hashes of individual words. Probably not entirerly correct, but fast
      pesho.key = 0;
      std::vector<uint64_t> vocabid_source = getVocabIDs(prev_line.source_phrase);
      for (int i = 0; i < vocabid_source.size(); i++) {
        pesho.key += (vocabid_source[i] << i);
      }
      pesho.bytes_toread = binfile.dist_from_start + binfile.extra_counter - entrystartidx;
      //Put into table
      table.Insert(pesho);

      break;
    }
  }

  serialize_table(mem, size, (basepath + "/probing_hash.dat").c_str());

  serialize_map(&source_vocabids, (basepath + "/source_vocabids").c_str());

  delete[] mem;

  //Write configfile
  std::ofstream configfile;
  configfile.open((basepath + "/config").c_str());
  configfile << API_VERSION << '\n';
  configfile << uniq_entries << '\n';
  configfile << num_scores << '\n';
  configfile << is_reordering << '\n';
  configfile.close();
}
