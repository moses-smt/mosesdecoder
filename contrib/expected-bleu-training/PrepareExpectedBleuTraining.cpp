/*
   Moses - statistical machine translation system
   Copyright (C) 2005-2015 University of Edinburgh

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include <vector>
#include <string>
#include <sstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/program_options.hpp>
#include "util/file_stream.hh"
#include "util/file.hh"
#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

namespace po = boost::program_options;


int main(int argc, char **argv)
{
  util::FileStream err(2);

  std::string filenameNBestListIn, filenameFeatureNamesOut, filenameIgnoreFeatureNames;
  size_t maxNBestSize;

  try {

    po::options_description descr("Usage");
    descr.add_options()
      ("help,h", "produce help message")
      ("n-best-list,n", po::value<std::string>(&filenameNBestListIn)->required(), 
       "input n-best list file")
      ("write-feature-names-file,f", po::value<std::string>(&filenameFeatureNamesOut)->required(), 
       "output file for mapping between feature names and indices")
      ("ignore-features-file,i", po::value<std::string>(&filenameIgnoreFeatureNames)->required(), 
       "input file containing list of feature names to be ignored")
      ("n-best-size-limit,l", po::value<size_t>(&maxNBestSize)->default_value(100), 
       "limit of n-best list entries to be considered")
      ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, descr), vm);

    if (vm.count("help")) {
      std::ostringstream os;
      os << descr;
      std::cout << os.str() << '\n';
      exit(0);
    }

    po::notify(vm);

  } catch(std::exception& e) {

    err << "Error: " << e.what() << '\n';
    err.flush();
    exit(1);
  }

  util::FilePiece ifsNBest(filenameNBestListIn.c_str());
  util::FilePiece ifsIgnoreFeatureNames(filenameIgnoreFeatureNames.c_str());
  util::scoped_fd fdFeatureNames(util::CreateOrThrow(filenameFeatureNamesOut.c_str()));
  util::FileStream ofsFeatureNames(fdFeatureNames.get());
  util::FileStream ofsNBest(1);

  boost::unordered_set<std::string> ignoreFeatureNames;
  StringPiece line;

  while ( ifsIgnoreFeatureNames.ReadLineOrEOF(line) )
  {
    if ( !line.empty() ) {
      util::TokenIter<util::AnyCharacter> item(line, " \t=");
      if ( item != item.end() )
      {
        ignoreFeatureNames.insert(item->as_string());
      }
      err << "ignoring " << *item << '\n';
    }
  }

  size_t maxFeatureNamesIdx = 0;
  boost::unordered_map<std::string, size_t> featureNames;

  size_t sentenceIndex = 0;
  size_t nBestSizeCount = 0;
  size_t globalIndex = 0;

  while ( ifsNBest.ReadLineOrEOF(line) )
  {
    util::TokenIter<util::MultiCharacter> item(line, " ||| ");

    if ( item == item.end() )
    {
      err << "Error: flawed content in " << filenameNBestListIn << '\n';
      exit(1);
    }

    size_t sentenceIndexCurrent = atol( item->as_string().c_str() );

    if ( sentenceIndex != sentenceIndexCurrent )
    {
      nBestSizeCount = 0;
      sentenceIndex = sentenceIndexCurrent;
    }

    if ( nBestSizeCount < maxNBestSize )
    {
      // process n-best list entry
      
      StringPiece scores;
      StringPiece decoderScore;
      for (size_t nItem=1; nItem<=3; ++nItem) 
      {
        if ( ++item == item.end() ) {
          err << "Error: flawed content in " << filenameNBestListIn << '\n';
          exit(1);
        }
        if (nItem == 2) {
          scores = *item;
        }
        if (nItem == 3) {
          decoderScore = *item;
        }
      }

      ofsNBest << sentenceIndex << ' ' 
               << decoderScore;

      util::TokenIter<util::SingleCharacter> token(scores, ' ');
      std::string featureNameCurrent("ERROR");
      std::string featureNameCurrentBase("ERROR");
      bool ignore = false;
      int scoreComponentIndex = 0;
      
      while ( token != token.end() )
      {
        if ( token->ends_with("=") )
        {
          scoreComponentIndex = 0;
          featureNameCurrent = token->substr(0,token->size()-1).as_string();
          size_t idx = featureNameCurrent.find_first_of('_');
          if ( idx == StringPiece::npos ) {
            featureNameCurrentBase = featureNameCurrent;
          } else {
            featureNameCurrentBase = featureNameCurrent.substr(0,idx+1);
          }
          ignore = false;
          if ( ignoreFeatureNames.find(featureNameCurrentBase) != ignoreFeatureNames.end() )
          {
            ignore = true;
          } else {
            if ( (featureNameCurrent.compare(featureNameCurrentBase)) &&
                 (ignoreFeatureNames.find(featureNameCurrent) != ignoreFeatureNames.end()) )
            {
              ignore = true;
            } 
          }
        }
        else
        {
          if ( !ignore )
          {
            float featureValueCurrent = atof( token->as_string().c_str() );;
            if ( scoreComponentIndex > 0 )
            {
              std::ostringstream oss;
              oss << scoreComponentIndex;
              featureNameCurrent.append("+");
            }
            if ( featureValueCurrent != 0 )
            {
              boost::unordered_map<std::string, size_t>::iterator featureName = featureNames.find(featureNameCurrent);

              if ( featureName == featureNames.end() )
              {
                std::pair< boost::unordered_map<std::string, size_t>::iterator, bool> inserted = 
                  featureNames.insert( std::make_pair(featureNameCurrent, maxFeatureNamesIdx) );
                ++maxFeatureNamesIdx;
                featureName = inserted.first;
              }

              ofsNBest << ' ' << featureName->second // feature name index
                       << ' ' << *token;             // feature value
            }
            ++scoreComponentIndex;
          }
        }
        ++token;
      }
      ofsNBest << '\n';
      ++nBestSizeCount;
    }
    ++globalIndex;
  }

  ofsFeatureNames << maxFeatureNamesIdx << '\n';
  for (boost::unordered_map<std::string, size_t>::const_iterator featureNamesIt=featureNames.begin();
       featureNamesIt!=featureNames.end(); ++featureNamesIt)
  {
    ofsFeatureNames << featureNamesIt->second << ' ' << featureNamesIt->first << '\n';
  }

}

