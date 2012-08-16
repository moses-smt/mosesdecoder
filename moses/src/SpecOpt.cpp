#include "SpecOpt.h"
#include "StaticData.h"


namespace Moses
{
  std::string GetAndStripTag(std::string &input, std::string open, std::string close, bool internal = true)
  {
    std::string output;
    size_t exStart = input.find(open);
    if(exStart != std::string::npos) {
      size_t inStart = exStart + open.size();
      size_t inEnd = input.find(close, inStart);
      if(inEnd != std::string::npos) {
        size_t exEnd = inEnd + close.size();
        if(internal)
          output = input.substr(inStart, inEnd - inStart);
        else
          output = input.substr(exStart, exEnd - exStart);
        input.erase(exStart, exEnd - exStart);
      }
    }
    return output;
  }

  #ifdef WITH_THREADS
  #ifdef BOOST_HAS_PTHREADS
  boost::mutex SpecOpt::s_namedMutex;
  #endif
  #endif
  
  std::map<std::string, SpecOpt*> SpecOpt::s_named;
  
  void SpecOpt::InitializeFromFile(std::string filename) {
    std::ifstream in;
    in.open(filename.c_str());
    if(!in.fail()) {
      std::string line;
      std::cerr << "Reading from SpecOpt file  " << filename << std::endl;
      while(std::getline(in, line))
        SpecOpt().ProcessAndStripSpecificOptions(line);
      in.close();
    }
    else {
      std::cerr << "Could not open SpecOpt file " << filename << std::endl;
    }
  }
  
  WeightInfos SpecOpt::parseWeights(std::string s) {
    WeightInfos wis;
    
    std::map<std::string, std::string> weightNames;
    weightNames["d"] = "d";
    weightNames["weight-d"] = "d";
    weightNames["g"] = "g";
    weightNames["weight-generation"] = "g";
    weightNames["lm"] = "lm";
    weightNames["weight-l"] = "lm";
    weightNames["lr"] = "lr";
    weightNames["weight-lr"] = "lr";
    weightNames["tm"] = "tm";
    weightNames["weight-t"] = "tm";
    weightNames["w"] = "w";
    weightNames["weight-w"] = "w";
    
    bool addValue = false;
    size_t valueCount = 0;
    WeightInfo current;
    
    std::vector<std::string> tokens = Tokenize(s);
    for(std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); it++) {
      std::string t = *it;
      if(t.length() > 1 && t[0] == '-' && (t[1] >= 'a' && t[1] <= 'z')) {
        size_t end = t.find(":", 1);
        std::string name = t.substr(1, end - 1);
        
        if(weightNames.count(name)) {
          WeightInfo wi;
          wi.name = weightNames[name];
          wi.ffIndex = 0;
          wi.ffWeightIndex = 0;
          
          if(end != std::string::npos) {
            size_t end2 = t.find(":", end+1);
            
            std::string ffIndex = t.substr(end+1, end2-end-1);
            if(ffIndex.length())
              wi.ffIndex = Scan<size_t>(ffIndex);
            
            if(end2 != std::string::npos) {
              std::string ffWeightIndex = t.substr(end2+1);
              if(ffWeightIndex.length())
                wi.ffWeightIndex = Scan<size_t>(ffWeightIndex);
            }
          }
          
          current = wi;
          addValue = true;
          valueCount = 0;
        }
        else {
          addValue = false;
        }
      }
      else {
        if(addValue) {
          float value = Scan<float>(t);
          WeightInfo newWeight = current;
          newWeight.value = value;
          newWeight.ffWeightIndex += valueCount;
          wis.push_back(newWeight);
          valueCount++;
        }
      }
    }
    return wis;
  }
  
  void SpecOpt::ProcessAndStripSpecificOptions(std::string &line)
  {
    std::string xml = Trim(GetAndStripTag(line, "<specOpt", "/>"));
    if(xml.size()) {
      m_hasSpecificOptions = true;
      
      size_t lastSize = std::string::npos;
      while(xml.size() != lastSize) {
        lastSize = xml.size();
        
        std::string nameStr = Trim(GetAndStripTag(xml, "name=\"", "\""));
        if(nameStr.size())
          m_name = nameStr;
        
        std::string systemStr = Trim(GetAndStripTag(xml, "system=\"", "\""));
        if(systemStr.size()) {
          m_translationSystemId = systemStr;
          m_changed = true;
        }
        
        std::string weightsStr = Trim(GetAndStripTag(xml, "weights=\"", "\""));
        if(weightsStr.size())
          m_changed = true;
        
        WeightInfos wi1 = parseWeights(weightsStr);
        m_weights.insert(m_weights.end(), wi1.begin(), wi1.end());
        
        std::string switchesStr = Trim(GetAndStripTag(xml, "switches=\"", "\""));
        if(switchesStr.size())
          m_changed = true;
        
        WeightInfos wi2 = parseWeights(switchesStr);
        m_weights.insert(m_weights.end(), wi2.begin(), wi2.end());
        
        std::vector<std::string> switchesVec = Tokenize(switchesStr);
        if(switchesVec.size())
          m_changed = true;
        
        size_t argc = switchesVec.size();
        char** argv = new char*[argc];
        for (size_t i = 0; i < switchesVec.size(); i++) {
          argv[i] = new char[switchesVec[i].size()+1];
          strncpy(argv[i], switchesVec[i].c_str(), switchesVec[i].size()+1);
        }
        m_switches.LoadAnyParam(argc, argv);
      }
      line = Trim(line);
    }
    
    if(m_name.size() && m_changed) {
      #ifdef WITH_THREADS
      #ifdef BOOST_HAS_PTHREADS
      boost::mutex::scoped_lock lock(s_namedMutex);
      #endif
      #endif

      if(s_named.count(m_name))
        delete s_named[m_name];
      s_named[m_name] = new SpecOpt(*this);
    }
  }
  
  
}
