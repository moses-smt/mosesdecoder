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

#include "Parameters.h"

#ifdef TOKENIZER_NAMESPACE
namespace TOKENIZER_NAMESPACE {
#endif

//
// @about
// Tokenizer implements the process of Koehn's tokenizer.perl via RE2
//
class Tokenizer {

private:

    typedef enum {
        empty = 0,
        blank,
        upper, // upper case
        letta, // extended word class (includes number, hyphen)
        numba,
        hyphn,
        stops, // blank to stops are "extended word class" variants
        quote, // init & fini = {',"}
        pinit, // init (includes INVERT_*)
        pfini, // fini
        pfpct, // fini + pct
        marks,
        limit
    } charclass_t;

    std::size_t nthreads;
    std::size_t chunksize;
    std::string cfg_dir;

    // non-breaking prefixes (numeric) utf8
    std::set<std::string> nbpre_num_set;
    // non-breaking prefixes (other) utf8
    std::set<std::string> nbpre_gen_set;

    // non-breaking prefixes (numeric) ucs4
    std::set<std::wstring> nbpre_num_ucs4;
    // non-breaking prefixes (other) ucs4
    std::set<std::wstring> nbpre_gen_ucs4;

    // compiled protected patterns
    std::vector<re2::RE2 *> prot_pat_vec;

protected:

    // language
    std::string lang_iso;
    bool english_p; // is lang_iso "en"
    bool latin_p; // is lang_iso "fr" or "it"
    bool skip_xml_p;
    bool skip_alltags_p;
    bool entities_p;
    bool escape_p;
    bool unescape_p;
    bool aggressive_hyphen_p;
    bool supersub_p;
    bool url_p;
    bool downcase_p;
    bool normalize_p;
    bool penn_p;
    bool narrow_latin_p;
    bool narrow_kana_p;
    bool refined_p;
    bool drop_bad_p;
    bool splits_p;
    bool verbose_p;
    bool para_marks_p;
    bool split_breaks_p;

    // return counts of general and numeric prefixes loaded
    std::pair<int,int> load_prefixes(std::ifstream& ifs); // used by init(), parameterized by lang_iso

    // in-place 1 line tokenizer, replaces input string, depends on wrapper to set-up invariants
    void protected_tokenize(std::string& inplace);

    // used for boost::thread
    struct VectorTokenizerCallable {
        Tokenizer *tokenizer;
        std::vector<std::string>& in;
        std::vector<std::string>& out;

        VectorTokenizerCallable(Tokenizer *_tokenizer,
                                std::vector<std::string>& _in,
                                std::vector<std::string>& _out)
        : tokenizer(_tokenizer)
        , in(_in)
        , out(_out) {
        };

        void operator()() {
            out.resize(in.size());
            for (std::size_t ii = 0; ii < in.size(); ++ii)
                if (in[ii].empty())
                    out[ii] = in[ii];
                else if (tokenizer->penn_p)
                    out[ii] = tokenizer->penn_tokenize(in[ii]);
                else
                    out[ii] = tokenizer->quik_tokenize(in[ii]);
        };
    };

public:

    Tokenizer(); // UNIMPL

    // no throw
    Tokenizer(const Parameters& _params);

    // frees dynamically compiled expressions
    ~Tokenizer();

    // required before other methods, may throw
    void init(const char *cfg_dir_path = 0);

    void set_config_dir(const std::string& _cfg_dir);

    // required after processing a contiguous sequence of lines when sentence splitting is on
    void reset();

    // simultaneous sentence splitting not yet implemented
    bool splitting() const { return splits_p; }

    // escapes chars the set &|"'<> after tokenization (moses special characters)
    bool escape(std::string& inplace);

    // used in detokenizer, converts entities into characters
    // if escape_p is set, does not unescape moses special tokens, thus
    // escape_p and unescape_p can be used together usefully
    bool unescape(std::string& inplace);

    // streaming select-tokenizer reads from is, writes to os, preserving line breaks (unless splitting)
    std::size_t tokenize(std::istream& is, std::ostream& os);

    // quik-tokenize padded line buffer to return string
    std::string quik_tokenize(const std::string& buf);

    // penn-tokenize padded line buffer to return string // untested
    std::string penn_tokenize(const std::string& buf);

    // select-tokenize padded line buffer to return string
    std::string tokenize(const std::string& buf) {
        return penn_p ? penn_tokenize(buf) : quik_tokenize(buf);
    }

    // tokenize with output argument
    void tokenize(const std::string& buf, std::string& outs) {
        outs = tokenize(buf);
    }

    // tokenize to a vector
    std::vector<std::string> tokens(const std::string& in) {
        std::istringstream tokss(penn_p ? penn_tokenize(in) : tokenize(in));
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

    // split a string on sentence boundaries (approximately)
    std::vector<std::string> splitter(const std::string &istr,bool *continuation_p = 0);

    // split sentences from input stream and write one per line on output stream
    std::pair<std::size_t,std::size_t> splitter(std::istream& is, std::ostream& os);

}; // end class Tokenizer

#ifdef TOKENIZER_NAMESPACE
};
#endif
