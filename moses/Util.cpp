// $Id$

/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2006 University of Edinburgh

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
 ***********************************************************************/

#ifdef WIN32
#include <windows.h>
#else
#include <sys/times.h>
#include <sys/resource.h>
#endif

#include <cstring>
#include <cctype>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <boost/algorithm/string/predicate.hpp>
#include "TypeDef.h"
#include "Util.h"
//#include "Timer.h"
#include "util/exception.hh"
#include "util/file.hh"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/StaticData.h"

using namespace std;
using namespace boost::algorithm;

namespace Moses
{


string GetTempFolder()
{
#ifdef _WIN32
  char *tmpPath = getenv("TMP");
  string str(tmpPath);
  if (!ends_with(str, "\\"))
    str += "\\";
  return str;
#else
  return "/tmp/";
#endif
}

const std::string ToLower(const std::string& str)
{
  std::string lc(str);
  std::transform(lc.begin(), lc.end(), lc.begin(), (int(*)(int))std::tolower);
  return lc;
}

class BoolValueException : public util::Exception {};

template<>
bool Scan<bool>(const std::string &input)
{
  std::string lc = ToLower(input);
  if (lc == "yes" || lc == "y" || lc == "true" || lc == "1")
    return true;
  if (lc == "no" || lc == "n" || lc =="false" || lc == "0")
    return false;
  UTIL_THROW(BoolValueException, "Could not interpret " << input << " as a boolean.  After lowercasing, valid values are yes, y, true, 1, no, n, false, and 0.");
}

bool FileExists(const std::string& filePath)
{
  ifstream ifs(filePath.c_str());
  return !ifs.fail();
}

std::vector< std::map<std::string, std::string> > ProcessAndStripDLT(std::string &line)
{
  std::vector< std::map<std::string, std::string> > meta;
  std::string lline = ToLower(line);
  bool check_dlt = true;

  //allowed format of dlt tag
  //<dlt type="name" id="name" attr="value"/>
  //the type attribute is mandatory; the name should not contain any double quotation mark
  //the id attribute is optional; if present, the name should not contain any double quotation mark
  //only one additional attribute is possible; value can contain double quotation marks
  //both name and value must be surrounded by double quotation mark

//		std::cerr << "GLOBAL START" << endl;
  while (check_dlt) {
    size_t start = lline.find("<dlt");
    if (start == std::string::npos) {
      //no more dlt tags
      check_dlt = false;
      continue;
    }
    size_t close = lline.find("/>");
    if (close == std::string::npos) {
      // error: dlt tag is not ended
      check_dlt = false;
      continue;
    }
    //std::string dlt = Trim(lline.substr(start+4, close-start-4));
    std::string dlt = Trim(line.substr(start+4, close-start-4));

    line.erase(start,close-start+2);
    lline.erase(start,close-start+2);

    if (dlt != "") {
      std::map<std::string, std::string> tmp_meta;

      //check if type is present and store it
      size_t start_type = dlt.find("type=");
      size_t len_type=4;
      if (start_type != std::string::npos) {
        //type is present
        //required format type="value"
        //double quotation mark is required

        std::string val_type;
        std::string label_type = dlt.substr(start_type, len_type);
        if (dlt[start_type+len_type+1] == '"') {
          val_type = dlt.substr(start_type+len_type+2);
          size_t close_type = val_type.find('"');
          val_type = val_type.substr(0, close_type);
          dlt.erase(start_type,start_type+len_type+2+close_type+1);
        } else {
          TRACE_ERR("DLT parse error: missing character \" for type \n");
        }
        label_type = Trim(label_type);
        dlt = Trim(dlt);

        tmp_meta[label_type] = val_type;
      } else {
        //type is not present
        UTIL_THROW(util::Exception, "ProcessAndStripDLT(std::string &line): Attribute type for dlt tag is mandatory.");
      }

      //check if id is present and store it
      size_t start_id = dlt.find("id=");
      size_t len_id=2;
      if (start_id != std::string::npos) {
        //id is present
        //required format id="name"
        //double quotation mark is required

        std::string val_id;
        std::string label_id = dlt.substr(start_id, len_id);
        if (dlt[start_id+len_id+1] == '"') {
          val_id = dlt.substr(start_id+len_id+2);
          size_t close_id = val_id.find('"');
          val_id = val_id.substr(0, close_id);
          dlt.erase(start_id,start_id+len_id+2+close_id+1);
        } else {
          TRACE_ERR("DLT parse error: missing character \" for id \n");
        }
        label_id = Trim(label_id);
        dlt = Trim(dlt);

        tmp_meta[label_id] = val_id;
      } else {
        //id is not present
        //do nothing
      }

      for (size_t i = 1; i < dlt.size(); i++) {
        if (dlt[i] == '=') {
          std::string label = dlt.substr(0, i);
          std::string val = dlt.substr(i+1);
          if (val[0] == '"') {

            val = val.substr(1);
            // it admits any double quotation mark (but is attribute) in the value of the attribute
            // it assumes that just one attribute (besides id attribute) is present in the tag,
            // it assumes that the value starts and ends with double quotation mark
            size_t close = val.rfind('"');
            if (close == std::string::npos) {
              TRACE_ERR("SGML parse error: missing \"\n");
              dlt = "";
              i = 0;
            } else {
              dlt = val.substr(close+1);
              val = val.substr(0, close);
              i = 0;
            }
          } else {
            size_t close = val.find(' ');
            if (close == std::string::npos) {
              dlt = "";
              i = 0;
            } else {
              dlt = val.substr(close+1);
              val = val.substr(0, close);
            }
          }
          label = Trim(label);
          dlt = Trim(dlt);

          tmp_meta[label] = val;
        }
      }

      meta.push_back(tmp_meta);
    }
  }
//		std::cerr << "GLOBAL END" << endl;
  return meta;
}

std::map<std::string, std::string> ProcessAndStripSGML(std::string &line)
{
  std::map<std::string, std::string> meta;
  std::string lline = ToLower(line);
  if (lline.find("<seg")!=0) return meta;
  size_t close = lline.find(">");
  if (close == std::string::npos) return meta; // error
  size_t end = lline.find("</seg>");
  std::string seg = Trim(lline.substr(4, close-4));
  std::string text = line.substr(close+1, end - close - 1);
  for (size_t i = 1; i < seg.size(); i++) {
    if (seg[i] == '=' && seg[i-1] == ' ') {
      std::string less = seg.substr(0, i-1) + seg.substr(i);
      seg = less;
      i = 0;
      continue;
    }
    if (seg[i] == '=' && seg[i+1] == ' ') {
      std::string less = seg.substr(0, i+1);
      if (i+2 < seg.size()) less += seg.substr(i+2);
      seg = less;
      i = 0;
      continue;
    }
  }
  line = Trim(text);
  if (seg == "") return meta;
  for (size_t i = 1; i < seg.size(); i++) {
    if (seg[i] == '=') {
      std::string label = seg.substr(0, i);
      std::string val = seg.substr(i+1);
      if (val[0] == '"') {
        val = val.substr(1);
        size_t close = val.find('"');
        if (close == std::string::npos) {
          TRACE_ERR("SGML parse error: missing \"\n");
          seg = "";
          i = 0;
        } else {
          seg = val.substr(close+1);
          val = val.substr(0, close);
          i = 0;
        }
      } else {
        size_t close = val.find(' ');
        if (close == std::string::npos) {
          seg = "";
          i = 0;
        } else {
          seg = val.substr(close+1);
          val = val.substr(0, close);
        }
      }
      label = Trim(label);
      seg = Trim(seg);
      meta[label] = val;
    }
  }
  return meta;
}

std::string PassthroughSGML(std::string &line, const std::string tagName, const std::string& lbrackStr, const std::string& rbrackStr)
{
  string lbrack = lbrackStr; // = "<";
  string rbrack = rbrackStr; // = ">";

  std::string meta = "";

  std::string lline = ToLower(line);
  size_t open = lline.find(lbrack+tagName);
  //check whether the tag exists; if not return the empty string
  if (open == std::string::npos) return meta;

  size_t close = lline.find(rbrack, open);
  //check whether the tag is closed with '/>'; if not return the empty string
  if (close == std::string::npos) {
    TRACE_ERR("PassthroughSGML error: the <passthrough info/> tag does not end properly\n");
    return meta;
  }
  // extract the tag
  std::string tmp = line.substr(open, close - open + 1);
  meta = line.substr(open, close - open + 1);

  // strip the tag from the line
  line = line.substr(0, open) + line.substr(close + 1, std::string::npos);

  TRACE_ERR("The input contains a <passthrough info/> tag:" << meta << std::endl);

  lline = ToLower(line);
  open = lline.find(lbrack+tagName);
  if (open != std::string::npos) {
    TRACE_ERR("PassthroughSGML error: there are two <passthrough> tags\n");
  }
  return meta;
}

void PrintFeatureWeight(const FeatureFunction* ff)
{
  cout << ff->GetScoreProducerDescription() << "=";
  size_t numScoreComps = ff->GetNumScoreComponents();
  vector<float> values = StaticData::Instance().GetAllWeights().GetScoresForProducer(ff);
  for (size_t i = 0; i < numScoreComps; ++i) {
    if (ff->IsTuneableComponent(i)) {
      cout << " " << values[i];
    } else {
      cout << " UNTUNEABLECOMPONENT";
    }
  }
  cout << endl;

}

void ShowWeights()
{
  FixPrecision(cout,6);
  const vector<const StatelessFeatureFunction*>& slf = StatelessFeatureFunction::GetStatelessFeatureFunctions();
  const vector<const StatefulFeatureFunction*>& sff = StatefulFeatureFunction::GetStatefulFeatureFunctions();

  for (size_t i = 0; i < sff.size(); ++i) {
    const StatefulFeatureFunction *ff = sff[i];
    if (ff->IsTuneable()) {
      PrintFeatureWeight(ff);
    } else {
      cout << ff->GetScoreProducerDescription() << " UNTUNEABLE" << endl;
    }
  }
  for (size_t i = 0; i < slf.size(); ++i) {
    const StatelessFeatureFunction *ff = slf[i];
    if (ff->IsTuneable()) {
      PrintFeatureWeight(ff);
    } else {
      cout << ff->GetScoreProducerDescription() << " UNTUNEABLE" << endl;
    }
  }
}

} // namespace


