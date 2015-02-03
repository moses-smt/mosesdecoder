#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <set>
#include <vector>
#include <iterator>
#include <stdexcept>

#include <re2/re2.h>
#include <unistd.h>

#ifdef TOKENIZER_NAMESPACE
namespace TOKENIZER_NAMESPACE {
#endif

//
// @about
// Tokenizer implements the process of Koehn's tokenizer.perl via RE2
//
class Tokenizer {

private:

    static std::string cfg_dir;

    std::set<std::string> nbpre_num_set;
    std::set<std::string> nbpre_gen_set;
    std::set<std::wstring> nbpre_num_ucs4;
    std::set<std::wstring> nbpre_gen_ucs4;
    std::vector<re2::RE2 *> prot_pat_vec;

protected:

    // language
    std::string lang_iso;
    bool english_p; // is lang_iso "en"
    bool latin_p; // is lang_iso "fr" or "it"
    bool skip_xml_p;
    bool skip_alltags_p;
    bool non_escape_p;
    bool aggressive_hyphen_p;
    bool supersub_p;
    bool url_p;
    bool downcase_p;
    bool normalize_p;
    bool penn_p;
    bool verbose_p;

    std::pair<int,int> load_prefixes(std::ifstream& ifs); // used by init(), parameterized by lang_iso

    // escapes specials into entities from the set &|"'[] (after tokenization, when enabled)
    bool escape(std::string& inplace);

    // in-place 1 line tokenizer, replaces input string, depends on wrapper to set-up invariants
    void protected_tokenize(std::string& inplace);

public:

    // cfg_dir is assumed shared by all languages
    static void set_config_dir(const std::string& _cfg_dir);

    // no throw
    Tokenizer(const std::string& _lang_iso = "en",
              bool _skip_xml_p = true, // skips isolated (linewise) tags in any case
              bool _skip_alltags_p = true, // skip all xml style tags
              bool _non_escape_p = false, // default is to call escape method before return
              bool _aggressive_hyphen_p = false, // hyphens become tokens when true
              bool _supersub_p = false, // handle super/subscript numerics
              bool _url_p = true,
              bool _downcase_p = false,
              bool _normalize_p = true,
              bool _penn_p = false,  // Treebank-3 compatible tokenization when true
              bool _verbose_p = false);

    // frees dynamically compiled expressions
    ~Tokenizer();

    // required before other methods, may throw
    void init();

    // streaming tokenizer reads from is, writes to os, preserving line breaks
    std::size_t tokenize(std::istream& is, std::ostream& os);

    // tokenize padded line buffer to return string
    std::string tokenize(const std::string& buf);

    void tokenize(const std::string& buf, std::string& outs) {
        outs = tokenize(buf);
    }

    // tokenize to a vector
    std::vector<std::string> tokens(const std::string& in) {
        std::istringstream tokss(tokenize(in));
        std::vector<std::string> outv;
        std::copy(std::istream_iterator<std::string>(tokss),
                  std::istream_iterator<std::string>(),
                  std::back_inserter(outv));
        return outv;
    }

    // streaming detokenizer reads from is, writes to os, preserving breaks
    std::size_t detokenize(std::istream& is, std::ostream &os);

    // detokenize padded line buffer to return string
    std::string detokenize(const std::string& buf);

    void detokenize(const std::string& buf, std::string& outs) {
        outs = detokenize(buf);
    }

    // detokenize from a vector
    std::string detokenize(const std::vector<std::string>& inv) {
        std::ostringstream oss;
        std::copy(inv.begin(), inv.end(), std::ostream_iterator<std::string>(oss," "));
        return detokenize(oss.str());
    }

}; // end class Tokenizer

#ifdef TOKENIZER_NAMESPACE
};
#endif
