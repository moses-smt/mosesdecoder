// $Id: prune-lm.cpp 27 2010-05-03 14:33:51Z nicolabertoldi $

/******************************************************************************
 IrstLM: IRST Language Model Toolkit, prune LM
 Copyright (C) 2008 Fabio Brugnara, FBK-irst Trento, Italy

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

******************************************************************************/

using namespace std;

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include "util.h"
#include "math.h"
#include "lmtable.h"


/* GLOBAL OPTIONS ***************/
std::string	spthr = "0";
int		aflag=0;
/********************************/

void usage(const char *msg = 0) {
  if (msg) { std::cerr << msg << std::endl; }
  std::cerr << "Usage: prune-lm [--threshold=th2,th3,...] [--abs=1|0] input-file [output-file]" << std::endl << std::endl;
  std::cerr << "    prune-lm reads a LM in either ARPA or compiled format and" << std::endl;
  std::cerr << "    prunes out n-grams (n=2,3,..) for which backing-off to the" << std::endl;
  std::cerr << "    lower order n-gram results in a small difference in probability." << std::endl;
  std::cerr << "    The pruned LM is saved in ARPA format" << std::endl << std::endl;
  std::cerr << "    Options:" << std::endl;
  std::cerr << "    --threshold=th2,th3,th4,... (pruning threshods for 2-grams, 3-grams, 4-grams,..." << std::endl;
  std::cerr << "                                 If less thresholds are specified, the last one is  " << std::endl;
  std::cerr << "                                 applied to all following n-gram levels.            " << std::endl << std::endl;
  std::cerr << "    --abs=1|0 	if 1, use absolute value of weighted difference"<< std::endl;

}

bool starts_with(const std::string &s, const std::string &pre) {
  if (pre.size() > s.size()) return false;

  if (pre == s) return true;
  std::string pre_equals(pre+'=');
  if (pre_equals.size() > s.size()) return false;
  return (s.substr(0,pre_equals.size()) == pre_equals);
}

std::string get_param(const std::string& opt, int argc, const char **argv, int& argi)
{
  std::string::size_type equals = opt.find_first_of('=');
  if (equals != std::string::npos && equals < opt.size()-1) {
    return opt.substr(equals+1);
  }
  std::string nexto;
  if (argi + 1 < argc) {
    nexto = argv[++argi];
  } else {
    usage((opt + " requires a value!").c_str());
    exit(1);
  }
  return nexto;
}

void handle_option(const std::string& opt, int argc, const char **argv, int& argi)
{
  if (opt == "--help" || opt == "-h") { usage(); exit(1); }

  if (starts_with(opt, "--threshold") || starts_with(opt, "-t"))
    spthr = get_param(opt, argc, argv, argi);
  else if (starts_with(opt, "--abs"))
    aflag = atoi(get_param(opt, argc, argv, argi).c_str());

  else {
    usage(("Don't understand option " + opt).c_str());
    exit(1);
  }
}

void s2t(string	cps,
         float	*thr)
{
	int	i;
	char	*s=strdup(cps.c_str()),
		*tk;

	thr[0]=0;
	for(i=1,tk=strtok(s, ","); tk; tk=strtok(0, ","),i++) thr[i]=atof(tk);
	for(; i<MAX_NGRAM; i++) thr[i]=thr[i-1];
}

int main(int argc, const char **argv)
{
	float		thr[MAX_NGRAM];

	if (argc < 2) { usage(); exit(1); }
	std::vector<std::string> files;
	for(int i=1; i < argc; i++) {
		std::string opt = argv[i];
		if(opt[0] == '-') handle_option(opt, argc, argv, i);
		else files.push_back(opt);
	}
	if (files.size() > 2) { usage("Too many arguments"); exit(1); }
	if (files.size() < 1) { usage("Please specify a LM file to read from"); exit(1); }
	memset(thr, 0, sizeof(thr));
	if(spthr != "") s2t(spthr, thr);
	std::string infile = files[0];
	std::string outfile= "";

  if (files.size() == 1) {
    outfile=infile;

    //remove path information
    std::string::size_type p = outfile.rfind('/');
    if (p != std::string::npos && ((p+1) < outfile.size()))
      outfile.erase(0,p+1);

    //eventually strip .gz
    if (outfile.compare(outfile.size()-3,3,".gz")==0)
      outfile.erase(outfile.size()-3,3);

    outfile+=".plm";
  }
  else
    outfile = files[1];


	lmtable lmt;
	inputfilestream inp(infile.c_str());
	if (!inp.good()) {
		std::cerr << "Failed to open " << infile << "!" << std::endl;
		exit(1);
	}

	lmt.load(inp,infile.c_str(),outfile.c_str(),0,NONE);
  std::cerr << "pruning LM with thresholds: \n";

  for (int i=1;i<lmt.maxlevel();i++) std::cerr<< " " << thr[i];
  std::cerr << "\n";
	lmt.wdprune((float*)thr, aflag);
	lmt.savetxt(outfile.c_str());
	return 0;
}

