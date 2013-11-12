
#pragma once

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include "Weights.h"
#include "Timer.h"

namespace  Moses
{
class InputFileStream;
}

class FeatureFunction;

class Global
{
public:
  static const Global &Instance() {
    return s_instance;
  }
  static Global &InstanceNonConst() {
    return s_instance;
  }

  Global();
  virtual ~Global();
  void Init(int argc, char** argv);

  std::istream &GetInputStream() const {
    return *m_inputStrme;
  }

  size_t stackSize;
  int maxDistortion;

  Weights weights;
  mutable Moses::Timer timer;

protected:
  static Global s_instance;
  std::string m_iniPath, m_inputPath;
  
  mutable std::istream *m_inputStrme;

  typedef std::vector<std::string> ParamList;
  typedef std::map<std::string, ParamList> Params;
  Params m_params;

  void InitParams();
  void InitFF();
  void InitWeight();
  void Load();

  bool ParamExist(const std::string &key) const;
};

