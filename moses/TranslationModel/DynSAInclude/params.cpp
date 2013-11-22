#include "params.h"

namespace Moses
{
// parameter constants
const std::string Parameters::kNotSetValue = "__NOT_SET__";

const int Parameters::kBoolValue = 0;
const int Parameters::kIntValue = 1;
const int Parameters::kFloatValue = 2;
const int Parameters::kStringValue = 3;
const int Parameters::kUndefinedValue = -1;

const std::string Parameters::kTrueValue = "1";
const std::string Parameters::kFalseValue = "0";

Parameters::Parameters(const ParamDefs * paramdefs, const count_t paramNum)
{
  initialize(paramdefs, paramNum);
}

Parameters::Parameters(int argc, char ** argv, const ParamDefs * paramdefs,
                       const count_t paramNum)
{
  initialize(paramdefs, paramNum);
  loadParams(argc, argv);
}

void Parameters::initialize(const ParamDefs * paramdefs, const count_t paramNum)
{
  for( count_t i = 0; i < paramNum; i++ ) {
    params_[paramdefs[i].name] = paramdefs[i]; // assign name
  }
  cerr << "Default parameter values:\n";
  iterate(params_, itr)
  cerr << "\t" << itr->first << " --> " << itr->second.value << endl;
}

bool Parameters::loadParams(int argc, char ** argv)
{
  // load params from commandline args
  //if( argc < 3 ) {
  //  fprintf(stderr, "ERROR: No parameters. Use \"-config\" or \"-f\" to specify configuration file.\n");
  //  return false;
  //}
  bool load_from_file = false;
  std::set<std::string> setParams;
  int jumpBy = 0;
  for( int i = 1; i < argc; i += jumpBy ) {
    std::string param = argv[i];
    if(param[0] != '-') {
      std::cerr << "Unknown parameter: " << param << std::endl;
      return false;
    }
    Utils::ltrim(param, "- ");
    // normalise parameter to long name
    param = normaliseParamName(param);
    // check if valid param name
    if(!isValidParamName(param)) {
      std::cerr << "Unknown param option \"" << param << "\"\n";
      exit(EXIT_FAILURE);
    }
    setParams.insert(param);  // needed to not overwrite param value if file is specified
    //if the parameter is of type booL no corresponding value
    if( getValueType(param) == kBoolValue ) {
      jumpBy = 1;
      UTIL_THROW_IF2(!setParamValue(param, kTrueValue),
    		  "Couldn't set parameter " << param);
    } else { //not of type bool so must have corresponding value
      UTIL_THROW_IF2(i+1 >= argc,
    		  "Out of bound error: " << i+1);

      jumpBy = 2;
      std::string val = argv[i+1];
      Utils::trim(val);
      if( param == "config" )
        load_from_file = true;
      if(!setParamValue(param, val)) {
        std::cerr << "Invalid Param name->value " << param << "->" << val << std::endl;
        return false;
      }
    }
  }
  bool success = true;
  // load from file if specified
  if (load_from_file)
    success = loadParams(getParamValue("config"), setParams);
  return success;
}

std::string Parameters::normaliseParamName(const std::string & name)
{
  // Map valid abbreviations to long names. Retain other names.
  if( params_.find(name) == params_.end() )
    iterate(params_, i)
    if( i->second.abbrev == name )
      return i->first;
  return name;
}

int Parameters::getValueType(const std::string& name)
{
  if(params_.find(name) != params_.end())
    return params_[name].type;
  return Parameters::kUndefinedValue;
}

bool Parameters::isValidParamName(const std::string & name)
{
  return params_.find(name) != params_.end();
}

bool Parameters::setParamValue(const std::string& name, const std::string& val)
{
  // TODO: Add basic type checking w verifyValueType()
  bool set = isValidParamName(name);
  if(set) {
    params_[name].value = val;
    std::cerr << "PARAM SET: "<< name << "=" << val << std::endl;
  }
  return( set );
}
std::string Parameters::getParamValue(const std::string& name)
{
  std::string value = Parameters::kNotSetValue;
  if(isValidParamName(name))
    if(params_.find(name) != params_.end())
      value = params_[name].value;
    else if(getValueType(name) == kBoolValue)
      value = kFalseValue;
  return value;
}
std::string Parameters::getParam(const std::string& name)
{
  return getParamValue(name);
  /*void* Parameters::getParam(const std::string& name) {
    void* paramVal = 0;
    int type = getValueType(name);
    const char* sval = getParamValue(name).c_str();
    switch(type) {
      case kIntValue: {
        int ival = atoi(sval);
        paramVal = (void*)&ival;
        break;
      }
      case kFloatValue: {
        float fval = atof(sval);
        paramVal = (void*)&fval;
        break;
      }
      case kStringValue: {
        paramVal = (void*)sval;
        break;
      }
      case kBoolValue: {
        bool bval = sval == Parameters::kTrueValue ? true : false;
        paramVal = (void*)&bval;
        break;
      }
      default: // --> Parameters::kUndefinedValue
        paramVal = (void*)sval; // will set to Parameters::kNotSetValue
    }
    return paramVal;*/
}
bool Parameters::verifyValueType(const std::string& name, const std::string& val)
{
  // Implement basic type checking
  return true;
}

int Parameters::getParamCount() const
{
  return params_.size();
}

/*
 * HAVE TO CHANGE loadParams() from file to not overwrite command lines but
 * override default if different*/
bool Parameters::loadParams(const std::string & file_path,
                            std::set<std::string>& setParams)
{
  // parameters loaded from file don't override cmd line paramters
  /*std::set<std::string>::iterator end = setParams.end();
  FileHandler file(file_path.c_str(), std::ios::in);
  std::string line, param;
  while ( getline(file, line) ) {
    Utils::trim(line);
    //ignore comments (lines beginning with '#') and empty lines
    if( line[0] == '#' || line.empty() ) continue;
    if( line[0] == '[' ) {
      Utils::trim(line, "-[]"); //remove brackets
      // normalise parameter names
      param = normaliseParamName(line);
      //handle boolean type parameters
      if(getValueType(param) == kBoolValue && setParams.find(param) == end)
        setParamValue(param, kTrueValue);
    } else {
      // TODO: verify that this works as intended
      if(setParams.find(param) == end) { // if param hasn't already been set in cmd line
        if(!setParamValue(param, line)) {
          std::cerr << "Invalid Param name->value " << param << "->" << line << std::endl;
          return false;
        }
      }
    }
  }*/
  return true;
}
/*
int Parameters::getCSVParams(const std::string & name, std::vector<std::string> & values) {
  // get param values(s) may be more than one separated by commas
  values.clear();
  if( isValidParamName(name) )
    if( params_.find(name) != params_.end() )
      return Utils::tokenizeToStr(params_.find(name)->second.c_str(), values, ",");
  return 0;
}

bool Parameters::checkParamIsSet(const std::string & name) {
  // Returns true for non-bool parameter that is set to anything.
  // Returns true for bool parameter only if set to true.
  if (getValueType(name) == kBoolValue)  // boolean value so check whether true
    return getParamValue(name) == kTrueValue;
  return (getParamValue(name) != kNotSetValue);
}

bool Parameters::printHelp(const std::string & name) {
  return true;
}

bool Parameters::printParams() {
  // print out parameters and values
  std::map<std::string, std::string>::iterator it;
  std::cerr << "User defined parameter settings:\n";
  for (it = params_.begin(); it != params_.end(); ++it)
    std::cerr << "\t" << it->first << "\t" << it->second << "\n";
  return true;
}
*/
}
