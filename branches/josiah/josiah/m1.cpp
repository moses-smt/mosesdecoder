#include <fstream>
#include <sstream>
#include <string>
#include <boost/program_options.hpp>
#include "Model1.h"

namespace po = boost::program_options;
using namespace Josiah;

// read a vocabulary in GIZA++ format
void read_giza_vcb(std::ifstream& in, vocabulary& v){
  std::string line;
  std::cerr << "Reading GIZA++ vocabulary file..." << std::endl;
  while (std::getline(in,line)){
    std::istringstream linebuf(line);
    std::string e;
    int eid;
    linebuf >> eid >> e;  // n.b. ignores frequency
    v.insert(vocabulary::value_type(eid,e));
  }
  v.insert(vocabulary::value_type(0,std::string("NULL"))); //implicit
}

/// read a ttable in GIZA++ format; as a sanity check, the 
/// input e and f vocabularies are consulted to insure that the
/// vocabulary ids are valid, throwing an exception if not.
/// \param in input stream to read.
/// \param f_vocab verified f vocabulary.
/// \param e_vocab verified e vocabulary.
/// \param reverse_keys should be true if input ids are in e, f order.
/// \param do_filter if true, only create entries for f vocab items in filter.
/// \param filter the set of f vocab items to create ttable entries for if do_filter==true.
/// \param table table to populate.
void read_giza_ttable(std::ifstream& in, const vocabulary& f_vocab,
  const vocabulary& e_vocab, const bool reverse_keys,
  const int col, bool do_filter, const std::set<std::string>& filter, 
  internal_model1_table& table){
  std::string line;
  std::cerr << "Reading " << (reverse_keys ? "p(f|e)" : "p(e|f)") << " GIZA++ ttable..." << std::endl;
  while (std::getline(in,line)){
    std::istringstream linebuf(line);
    int id0, id1;
    float score;
    linebuf >> id0 >> id1 >> score;
    int fid = reverse_keys ? id1 : id0;
    int eid = reverse_keys ? id0 : id1;
    if (f_vocab.left.find(fid) == f_vocab.left.end()){
      std::cerr << "Found unknown id " << fid << " in: " << line <<  std::endl;
      throw std::runtime_error("Inconsistent data");    
    } else {
      if (e_vocab.left.find(eid) == e_vocab.left.end()){
        std::cerr << "Skipping unknown id " << eid << "in: " << line << std::endl;
        throw std::runtime_error("Inconsistent data");
      }
      if (!do_filter || filter.find(f_vocab.left.find(fid)->second) != filter.end()){
        table[fid][eid][col] = score;
      }
    }
  }
}

void verify_giza_ttable(std::ifstream& in, const vocabulary& f_vocab,
  const vocabulary& e_vocab, const bool reverse_keys,
  const int col, bool do_filter, const std::set<std::string>& filter, 
  external_model1_table& table){
  std::string line;
  std::cerr << "Reading " << (reverse_keys ? "p(f|e)" : "p(e|f)") << " GIZA++ ttable..." << std::endl;
  while (std::getline(in,line)){
    std::istringstream linebuf(line);
    int id0, id1;
    float score;
    linebuf >> id0 >> id1 >> score;
    int fid = reverse_keys ? id1 : id0;
    int eid = reverse_keys ? id0 : id1;
    if (!do_filter || filter.find(f_vocab.left.find(fid)->second) != filter.end()){
      if (table.score(fid, eid, col) != score)
        std::cerr << "Mismatched score (" << fid << "," << eid << ")= " << score << ", " << table.score(fid, eid, col) << std::endl;
    }
  }
}

void usage(const po::options_description& options, const char* const progname){
  std::cerr << "Usage: " << std::endl << std::endl <<
    std::string(progname) << 
    " <f.vcb> <e.vcb> <p(e|f).t1> <p(f|e).t1> <output file>" << std::endl <<
    "    where vcb and t1 files are giza++ output files." << std::endl << std::endl << 
    options << std::endl;
}

int main(int argc, char **argv) {

  po::options_description opts("Options");
  opts.add_options()
    ("help", "Display this message and exit")
    ("filter,f", po::value<std::string>(), "Filter statistics for source file")
    ("verify,v", 
      po::value<bool>()->zero_tokens()->default_value(false),
      "Verify that an existing model 1 data file matches input GIZA++ files ")
  ;
  po::options_description args("positional arguments");
  args.add_options()
    ("f_vcb", "<f_vcb>")
    ("e_vcb", "<e_vcb>")
    ("pef", "<pef>")
    ("pfe", "<pfe>")
    ("out", "<out>")
  ;
  po::options_description cmdline_opts;
  cmdline_opts.add(opts);
  cmdline_opts.add(args);
  po::positional_options_description p;
  p.add("f_vcb", 1).add("e_vcb", 1).add("pef", 1).add("pfe", 1).add("out", 1);

  po::variables_map options;
  try {
    po::store(po::command_line_parser(argc, argv).
      options(cmdline_opts).positional(p).run(), options);
    po::notify(options); 
    if (options["out"].empty()) 
      throw po::too_few_positional_options_error("No output file specified");
  } catch (po::error e){
    std::cerr << "Error: " << e.what() << std::endl << std::endl;
    usage(opts, argv[0]);
    return 1;
  }
  if (!options["help"].empty()) { 
    usage(opts, argv[0]); 
    return 1;
  }
  std::set<std::string> filter;
  bool do_filter = false;
  if (!options["filter"].empty()){
    do_filter = true;
    std::ifstream test_file(options["filter"].as<std::string>().c_str());
    for(std::istream_iterator<std::string> i(test_file); i!=
      std::istream_iterator<std::string>(); ++i){
      filter.insert(*i);
    }
  }      
  vocabulary f_vocab;
  std::ifstream f_vocab_file(options["f_vcb"].as<std::string>().c_str());
  read_giza_vcb(f_vocab_file, f_vocab);
  f_vocab_file.close();

  vocabulary e_vocab;
  std::ifstream e_vocab_file(options["e_vcb"].as<std::string>().c_str());
  read_giza_vcb(e_vocab_file, e_vocab);
  e_vocab_file.close();

  if (options["verify"].as<bool>()){
    std::cerr << "Verifying model 1 table in " << options["out"].as<std::string>() << std::endl;
    external_model1_table emt(options["out"].as<std::string>());
    
    std::ifstream pef_file(options["pef"].as<std::string>().c_str());
    verify_giza_ttable(pef_file, f_vocab, e_vocab, false, 0, do_filter, filter, emt);
    pef_file.close();
    
    std::ifstream pfe_file(options["pfe"].as<std::string>().c_str());
    verify_giza_ttable(pfe_file, f_vocab, e_vocab, true, 1, do_filter, filter, emt);
    pfe_file.close();

  } else {
    internal_model1_table imt;
    std::ifstream pef_file(options["pef"].as<std::string>().c_str());
    read_giza_ttable(pef_file, f_vocab, e_vocab, false, 0, do_filter, filter, imt);
    pef_file.close();
    
    std::ifstream pfe_file(options["pfe"].as<std::string>().c_str());
    read_giza_ttable(pfe_file, f_vocab, e_vocab, true, 1, do_filter, filter, imt);
    pfe_file.close();

    external_model1_table emt(imt, f_vocab, e_vocab, 
      options["out"].as<std::string>());
  }
}
