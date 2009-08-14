#include "Utils.h"
#include "GainFunction.h"
#include "SentenceBleu.h"
#include "Model1.h"
#include "Pos.h"
#include "Dependency.h"
#include "SourceToTargetRatio.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace Josiah {
  
  void LoadReferences(const vector<string>& ref_files, string input_file, GainFunctionVector* g, float bp_scale, bool use_bp_denum_hack) {
    assert(ref_files.size() > 0);
    vector<ifstream*> ifs(ref_files.size(), NULL);
    for (unsigned i = 0; i < ref_files.size(); ++i) {
      cerr << "Reference " << (i+1) << ": " << ref_files[i] << endl;
      ifs[i] = new ifstream(ref_files[i].c_str());
      assert(ifs[i]->good());
    }
    
    
    ifstream infiles(input_file.c_str());
    assert (infiles);
    
    size_t ctr = 0;
    while(!ifs[0]->eof() && !infiles.eof()) {
      vector<string> refs(ref_files.size());
      for (unsigned int i = 0; i < refs.size(); ++i) {
        getline(*ifs[i], refs[i]);
      }
      string line;
      getline(infiles, line);
      
      if (refs[0].empty() && ifs[0]->eof()) break;
      if (line.empty() && infiles.eof()) break;
      g->push_back(new SentenceBLEU(4, refs, line, bp_scale, use_bp_denum_hack));
      ctr++;
    }
    for (unsigned i=0; i < ifs.size(); ++i) delete ifs[i];
    cerr << "Loaded reference translations for " << g->size() << " sentences." << endl;
  }
  
  

  
  void configure_features_from_file(const std::string& filename, feature_vector& fv){
    std::cerr << "Reading extra features from " << filename << std::endl;
    std::ifstream in(filename.c_str());
    if (!in) {
      throw std::runtime_error("Unable to open feature configuration file");
    }
    // todo: instead of having this function know about all required options of
    // each feature, have features populate options / read variable maps /
    // populate feature_vector using static functions.
    po::options_description desc;
    bool useApproxPef = false;
    bool useApproxPfe = false;
    bool useVerbDiff = false;
    bool useCherry = false;
    bool useDepDist = false;
    bool useSrcTgtRatio = false;
    size_t dependencyFactor;
    desc.add_options()
    ("model1.table", "Model 1 table")
    ("model1.pef_column", "Column containing p(e|f) score")
    ("model1.pfe_column", "Column containing p(f|e) score")
    ("model1.approx_pef",po::value<bool>(&useApproxPef)->default_value(false), "Approximate the p(e|f), and use importance sampling")
    ("model1.approx_pfe",po::value<bool>(&useApproxPfe)->default_value(false), "Approximate the p(f|e), and use importance sampling")
    ("pos.verbdiff", po::value<bool>(&useVerbDiff)->default_value(false), "Verb difference feature")
    ("dependency.cherry", po::value<bool>(&useCherry)->default_value(false), "Use Colin Cherry's syntactic cohesiveness feature")
    ("dependency.distortion", po::value<bool>(&useDepDist)->default_value(false), "Use the dependency distortion feature")
    ("dependency.factor", po::value<size_t>(&dependencyFactor)->default_value(1), "Factor representing the dependency tree")
    ("srctgtratio.useFeat", po::value<bool>(&useSrcTgtRatio)->default_value(false), "Use source length to target length ratio feature");
    
    po::variables_map vm;
    po::store(po::parse_config_file(in,desc,true), vm);
    notify(vm);
    if (!vm["model1.pef_column"].empty() || !vm["model1.pfe_column"].empty()){
      boost::shared_ptr<external_model1_table> ptable;
      boost::shared_ptr<moses_factor_to_vocab_id> p_evocab_mapper;
      boost::shared_ptr<moses_factor_to_vocab_id> p_fvocab_mapper;
      if (vm["model1.table"].empty())
        throw std::runtime_error("Requesting Model 1 features, but no Model 1 table given");
      else {
        ptable.reset(new external_model1_table(vm["model1.table"].as<std::string>()));
        p_fvocab_mapper.reset(new moses_factor_to_vocab_id(ptable->f_vocab(), Moses::Input, 0, Moses::FactorCollection::Instance())); 
        p_evocab_mapper.reset(new moses_factor_to_vocab_id(ptable->e_vocab(), Moses::Output, 0, Moses::FactorCollection::Instance())); 
      }
      if (!vm["model1.pef_column"].empty()) {
        if (useApproxPef) {
          cerr << "Using approximation for model1" << endl;
          fv.push_back(feature_handle(new ApproximateModel1(ptable, p_fvocab_mapper, p_evocab_mapper)));
        } else {
          fv.push_back(feature_handle(new model1(ptable, p_fvocab_mapper, p_evocab_mapper)));
        }
      }
      if (!vm["model1.pfe_column"].empty()) {
        if (useApproxPfe) {
          cerr << "Using approximation for model1 inverse" << endl;
          fv.push_back(feature_handle(new ApproximateModel1Inverse(ptable, p_fvocab_mapper, p_evocab_mapper)));
        } else {
          fv.push_back(feature_handle(new model1_inverse(ptable, p_fvocab_mapper, p_evocab_mapper)));
        }
      }
      
    }
    if (useVerbDiff) {
      //FIXME: Should be configurable
      fv.push_back(feature_handle(new VerbDifferenceFeature(1,1)));
    }
    if (useCherry) {
      fv.push_back(feature_handle(new CherrySyntacticCohesionFeature(dependencyFactor)));
    }
    if (useSrcTgtRatio) {
      fv.push_back(feature_handle(new SourceToTargetRatio));
    }
    if (useDepDist) {
      fv.push_back(feature_handle(new DependencyDistortionFeature(dependencyFactor)));
    }
    in.close();
  }
  
}


