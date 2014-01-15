// -*- c++ -*-
// (c) 2009 Ulrich Germann
// boilerplate code to declutter my usual interpret_args() routine
#include "ug_get_options.h"
#include <fstream>

#include <string>
#include <iostream>

namespace ugdiss
{
  using namespace std;

  void 
  get_options(int ac, char* av[], progopts& o, posopts& a, optsmap& vm,
              char const* cfgFileParam)
  {
    // only get named parameters from command line
    po::store(po::command_line_parser(ac,av).options(o).run(),vm);

    if (cfgFileParam && vm.count(cfgFileParam))
      {
        string cfgFile = vm[cfgFileParam].as<string>();
        if (!cfgFile.empty())
          {
            if (!access(cfgFile.c_str(),F_OK))
              {
                ifstream cfg(cfgFile.c_str());
                po::store(po::parse_config_file(cfg,o),vm);
              }
            else
              {
                cerr << "Error: cannot find config file '" 
		     << cfgFile << "'!" << endl;
                exit(1);
              }
          }
      }
    
    // process positional args, ignoring those set in the config file
    if (a.max_total_count())
      po::store(po::command_line_parser(ac,av)
                .options(o).positional(a).run(),vm);  
    po::notify(vm); // IMPORTANT
  }
}
