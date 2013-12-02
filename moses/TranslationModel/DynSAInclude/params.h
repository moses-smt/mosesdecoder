#ifndef moses_DynSAInclude_params_h
#define moses_DynSAInclude_params_h

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include "FileHandler.h"
#include "utils.h"
#include "types.h"

#define NumOfParams(paramArray) (sizeof(paramArray)/sizeof((paramArray)[0]))

namespace Moses
{
typedef struct ParamDefs {
  std::string name;
  std::string value;
  std::string abbrev;
  int type;
  std::string description;
} ParamDefs;

//! @todo ask abby2
class Parameters
{
public:
  static const std::string kNotSetValue;
  static const int kBoolValue;
  static const int kIntValue;
  static const int kFloatValue;
  static const int kStringValue;
  static const int kUndefinedValue;
  static const std::string kFalseValue;
  static const std::string kTrueValue;

  Parameters(const ParamDefs * paramdefs, const count_t paramNum);
  Parameters(int argc, char** argv, const ParamDefs * paramdefs, const count_t paramNum);
  ~Parameters() {}
  bool loadParams(int argc, char ** argv);
  bool loadParams(const std::string& param_file, std::set<std::string>&);
  int getValueType(const std::string & name);
  bool setParamValue(const std::string& name, const std::string& value);
  bool verifyValueType(const std::string& name, const std::string& value);
  bool isValidParamName(const std::string & name);
  std::string getParamValue(const std::string& name);
  //void* getParam(const std::string& name);
  std::string getParam(const std::string& name);
  int getParamCount() const;
  /*
  int getCSVParams(const std::string & name, std::vector<std::string> & values);
  bool checkParamIsSet(const std::string& name);
  bool printParams();
  bool printHelp(const std::string & name);
  */
private:
  std::string normaliseParamName(const std::string &name);
  void initialize(const ParamDefs * paramdefs, const count_t paramNum);
  std::map<std::string, ParamDefs > params_;  // name->value,type,abbrev,desc
};

}
#endif //INC_PARAMS.H

