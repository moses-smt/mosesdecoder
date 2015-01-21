#include "tokenizer.h"
#include <sstream>
#include <iterator>
#include <memory>
#include <vector>
#include <algorithm>

namespace {

// frequently used regexp's are pre-compiled thus:

RE2 genl_tags_x("<[/!\\p{L}]+[^>]*>");
RE2 mult_spc_x(" +"); // multiple spaces
RE2 tag_line_x("^<.+>$"); // lines beginning and ending with open/close angle-bracket pairs
RE2 white_line_x("^\\s*$"); // lines entirely composed of whitespace
RE2 ctrls_x("[\\000-\\037]*"); // match any control characters
RE2 head_spc_x("^ "); // match a leading space on a line
RE2 tail_spc_x(" $"); // match a trailing space on a line
RE2 genl_spc_x("\\s+"); // any sequence of one or more whitespace characters
RE2 specials_x("([^_\\p{L}\\p{N}\\s\\.\\'\\`\\,\\-])"); // any surely non-token character
RE2 hyphen_x("([\\p{L}\\p{N}])(-)([\\p{L}\\p{N}])"); // any hyphenated pronouncable sequence
RE2 slash_x("([\\p{L}\\p{N}])(/)([\\p{L}\\p{N}])"); // and slash-conjoined " "
RE2 final_x("([^.])([.])([\\]\\)}>\"']*) ?$"); // sentence-final punctuation sequence (non qm em)
RE2 qx_x("([?!])"); // one qm/em mark
RE2 braces_x("([\\]\\[\\(\\){}<>])"); // any open or close of a pair
RE2 endq_x("([^'])' "); // post-token single-quote or doubled single-quote
RE2 postncomma_x("([^\\p{N}]),"); // comma after non-number
RE2 prencomma_x(",([^\\p{N}])"); // comma before non-number
RE2 nanaapos_x("([^\\p{L}])'([^\\p{L}])"); // non-letter'non-letter  contraction form
RE2 nxpaapos_x("([^\\p{L}\\p{N}])'([\\p{L}])"); // alnum'non-letter contraction form
RE2 napaapos_x("([^\\p{L}])'([\\p{L}])"); // non-letter'letter contraction form
RE2 panaapos_x("([\\p{L}])'([^\\p{L}])"); // letter'non-letter contraction form
RE2 papaapos_x("([\\p{L}])'([\\p{L}])"); // letter'letter contraction form
RE2 pnsapos_x("([\\p{N}])[']s"); // plural number
RE2 letter_x("\\p{L}"); // a letter
RE2 lower_x("^\\p{Ll}"); // a lower-case letter
RE2 sinteger_x("^\\p{N}"); // not a digit mark
RE2 dotskey_x("MANYDOTS(\\d+)"); // token for a dot sequence parameterized by seq length
RE2 numprefixed_x("[-+/.@\\\\#\\%&\\p{Sc}\\p{N}]*[\\p{N}]+-[-'`\"\\p{L}]*\\p{L}");
RE2 quasinumeric_x("[-.;:@\\\\#\%&\\p{Sc}\\p{So}\\p{N}]*[\\p{N}]+");
RE2 numscript_x("([\\p{N}\\p{L}])([\\p{No}]+)(\\p{Ll})");
RE2 nonbreak_x("-\\p{L}"); // where not to break a protected form

RE2 x1_v_d("([ ([{<])\""); // a valid non-letter preceeding a double-quote
RE2 x1_v_gg("([ ([{<])``"); // a valid non-letter preceeding directional doubled open single-quote
RE2 x1_v_g("([ ([{<])`([^`])"); //  a valid non-letter preceeding directional unitary single-quote
RE2 x1_v_q("([ ([{<])'"); //  a valid non-letter preceeding undirected embedded quotes
RE2 ndndcomma_x("([^\\p{N}]),([^\\p{N}])"); // non-digit,non-digit
RE2 pdndcomma_x("([\\p{N}]),([^\\p{N}])"); // digit,non-digit
RE2 ndpdcomma_x("([^\\p{N}]),([\\p{N}])"); // non-digit,digit
RE2 symbol_x("([;:@\\#\\$%&\\p{Sc}\\p{So}])"); // usable punctuation mark not a quote or a brace
RE2 contract_x("'([sSmMdD]) "); // english single letter contraction forms
RE2 right_x("[\\p{Sc}({¿¡]+"); //
RE2 left_x("[,.?!:;\\%})]+"); // 
RE2 curr_en_x("^[\'][\\p{L}]"); //
RE2 pre_en_x("[\\p{L}\\p{N}]$"); //
RE2 curr_fr_x("[\\p{L}\\p{N}][\']$"); //
RE2 post_fr_x("^[\\p{L}\\p{N}]"); // 
RE2 quotes_x("^[\'\"]+$"); //
RE2 endnum_x("[-\'\"]"); //

// anything rarely used will just be given as a string and compiled on demand by RE2 

}; // end anonymous namespace


#ifdef TOKENIZER_NAMESPACE
namespace TOKENIZER_NAMESPACE {
#endif

// where to load nonbreaking_prefix.XX files
// and protected_pattern.XX files

std::string Tokenizer::cfg_dir(".");


// static method
void
Tokenizer::set_config_dir(const std::string& dir) {
    if (dir.empty()) {
        cfg_dir = ".";
    } else {
        cfg_dir.assign(dir);
    }
}


Tokenizer::Tokenizer(const std::string& _lang_iso,
                     bool _skip_xml_p,
                     bool _skip_alltags_p,
                     bool _non_escape_p,
                     bool _aggressive_hyphen_p,
                     bool _penn_p,
                     bool _verbose_p)
        : lang_iso(_lang_iso)
        , english_p(_lang_iso.compare("en")==0)
        , latin_p((!english_p) && (_lang_iso.compare("fr")==0 || _lang_iso.compare("it")==0))
        , skip_xml_p(_skip_xml_p)
        , skip_alltags_p(_skip_alltags_p)
        , non_escape_p(_non_escape_p)
        , aggressive_hyphen_p(_aggressive_hyphen_p)
        , penn_p(_penn_p)
        , verbose_p(_verbose_p)
{
}


//
// dtor deletes dynamically allocated per-language RE2 compiled expressions
//
Tokenizer::~Tokenizer() 
{
    for (auto& ptr : prot_pat_vec) {
        if (ptr == &numprefixed_x || ptr == &quasinumeric_x)
            continue;
        delete ptr;
    }
}


//
// stuffs numeric-only prefixes into nbpre_num_set,
// others into nbpre_gen_set
//
std::pair<int,int>
Tokenizer::load_prefixes(std::ifstream& ifs) 
{
    RE2 numonly("(.*)[\\s]+(\\#NUMERIC_ONLY\\#)");
    std::string line;
    int nnon = 0;
    int nnum = 0;

    while (std::getline(ifs,line)) {
        if (!line.empty() && line.at(0) != '#') {
            std::string prefix;
            if (RE2::PartialMatch(line,numonly,&prefix)) {
                nbpre_num_set.insert(prefix);
                nnum++;
            } else {
                nbpre_gen_set.insert(line);
                nnon++;
            }
        }
    }
    return std::make_pair(nnon,nnum);
}


//
// load files (make sure to call set_config_dir before, if ever
// for nonbreaking prefixes and protected patterns
//
void
Tokenizer::init() {
    std::string nbpre_path(cfg_dir);
    nbpre_path.append("/nonbreaking_prefix.").append(lang_iso);
    // default to generic version
    if (::access(nbpre_path.c_str(),R_OK)) 
        nbpre_path = nbpre_path.substr(0,nbpre_path.size()-lang_iso.size()-1);

    if (::access(nbpre_path.c_str(),R_OK) == 0) {
        std::ifstream cfg(nbpre_path.c_str());
        try {
            std::pair<int,int> counts = load_prefixes(cfg);
            if (verbose_p) {
                std::cerr << "loaded " << counts.first << " non-numeric, " 
                          << counts.second << " numeric prefixes from "
                          << nbpre_path << std::endl;
            }
        } catch (...) {
            std::ostringstream ess;
            ess << "I/O error reading " << nbpre_path << " in " << __FILE__ << " at " << __LINE__;
            throw std::runtime_error(ess.str());
        }
    } else if (verbose_p) {
        std::cerr << "no prefix file found: " << nbpre_path << std::endl;
    }

    if (nbpre_gen_set.empty() && nbpre_num_set.empty()) {
        std::ostringstream ess;
        ess << "Error at " << __FILE__ << ":" << __LINE__ << " : "
            << "No known abbreviations for language " << lang_iso;
        throw std::runtime_error(ess.str());
    }

    std::string protpat_path(cfg_dir);
    protpat_path.append("/protected_pattern.").append(lang_iso);
    // default to generic version
    if (::access(protpat_path.c_str(),R_OK)) 
        protpat_path = protpat_path.substr(0,protpat_path.size()-lang_iso.size()-1);

    prot_pat_vec.push_back(&numprefixed_x);
    prot_pat_vec.push_back(&quasinumeric_x);

    if (::access(protpat_path.c_str(),R_OK) == 0) {
        std::ifstream cfg(protpat_path.c_str());
        char linebuf[1028];
        int npat = 0;
        try {
            linebuf[0]='(';
            while (cfg.good()) {
                cfg.getline(linebuf+1,1024);
                if (linebuf[1] && linebuf[1] != '#') {
                    strcat(linebuf,")");
                    prot_pat_vec.push_back(new RE2(linebuf));
                    npat++;
                }
            }
        } catch (...) {
            std::ostringstream ess;
            ess << "I/O error reading " << protpat_path << " in " << __FILE__ << " at " << __LINE__;
            throw std::runtime_error(ess.str());
        }
        if (verbose_p) {
            std::cerr << "loaded " << npat << " protected patterns from " 
                      << protpat_path << std::endl;
        }
    } else if (verbose_p) {
        std::cerr << "no protected file found: " << protpat_path << std::endl;
    }
}


//
// apply ctor-selected tokenization to a string, in-place, no newlines allowed,
// assumes protections are applied already, some invariants are in place
//
void
Tokenizer::protected_tokenize(std::string& text) {
    std::vector<std::string> words;
    size_t pos = 0;
    if (text.at(pos) == ' ')
        ++pos;
    size_t next = text.find(' ',pos);
    while (next != std::string::npos) {
        if (next - pos)
            words.push_back(text.substr(pos,next-pos));
        pos = next + 1;
        while (pos < text.size() && text.at(pos) == ' ')
            ++pos;
        next = text.find(' ',pos);
    }
    if (pos < text.size() && text.at(pos) != ' ')
        words.push_back(text.substr(pos,text.size()-pos));
    
    text.clear();

    // regurgitate words with look-ahead handling for tokens with final .
    for (size_t ii = 0; ii < words.size(); ++ii) {
        size_t len = words[ii].size();

        if (len > 1 && words[ii].at(len-1) == '.') {
            std::string prefix(words[ii].substr(0,len-1));
            bool gen_prefix_p = nbpre_gen_set.find(prefix) != nbpre_gen_set.end();
            bool embeds_p = prefix.find('.') != std::string::npos;
            bool letter_p = RE2::PartialMatch(prefix.c_str(),letter_x);
            bool more_p = ii < words.size() - 1;
            bool nlower_p = more_p && RE2::PartialMatch(words[ii+1].c_str(),lower_x);
            bool num_prefix_p = (!gen_prefix_p) && nbpre_num_set.find(prefix) != nbpre_num_set.end();
            bool nint_p = more_p && RE2::PartialMatch(words[ii+1].c_str(),sinteger_x);
            bool isolate_p = true;
            if (gen_prefix_p) {
                isolate_p = false;
            } else if (num_prefix_p && nint_p) {
                isolate_p = false;
            } else if (embeds_p && letter_p) {
                isolate_p = false;
            } else if (nlower_p) {
                isolate_p = false;
            }
            if (isolate_p) {
                words[ii].assign(prefix);
                words[ii].append(" .");
            }
        } 

        text.append(words[ii]);
        if (ii < words.size() - 1)
            text.append(" ");
    }
}


bool
Tokenizer::escape(std::string& text) {
    static const char escaping[] = "&|<>'\"[]";
    static const char *replacements[] = {
        "&amp;",
        "&#124;",
        "&lt;",
        "&gt;",
        "&apos;",
        "&quot;",
        "&#91;",
        "&#93;"
    };
    bool modified = false;
    const char *next = escaping;
    
    for (int ii = 0; *next; ++ii, ++next) {
        size_t pos = 0;
        for (pos = text.find(*next,pos); pos != std::string::npos; 
             pos = (++pos < text.size() ? text.find(*next,pos) : std::string::npos)) {
            std::string replacement(replacements[ii]);
            if (*next != '\'') {
                if (pos > 0 && text.at(pos-1) == ' ' && pos < text.size()-1 && text.at(pos+1) != ' ') 
                    replacement.append(" ");
            }
            text.replace(pos,1,replacement);
            modified = true;
        }
    }
    
    return modified;
}


std::string
Tokenizer::tokenize(const std::string& buf)
{
    static const char *apos_refs = "\\1 ' \\2";
    static const char *right_refs = "\\1 '\\2";
    static const char *left_refs = "\\1' \\2";
    static const char *comma_refs = "\\1 , \\2";
    static const char *isolate_ref = " \\1 ";
    static const char *special_refs = "\\1 @\\2@ \\3";

    std::string outs;
    std::string text(buf);

    if (skip_alltags_p) {
        RE2::GlobalReplace(&text,genl_tags_x," ");
    }

    RE2::GlobalReplace(&text,genl_spc_x," ");
    RE2::GlobalReplace(&text,ctrls_x,"");

    size_t pos;
    int num = 0;

    if (!penn_p) {
        // this is the main moses-compatible tokenizer
        
        // push all the prefixes matching protected patterns
        std::vector<std::string> prot_stack;
        std::string match;
        for (auto& pat : prot_pat_vec) {
            pos = 0;
            while (RE2::PartialMatch(text.substr(pos),*pat,&match)) {
                pos = text.find(match,pos);
                if (pos == std::string::npos)
                    break;
                size_t len = match.size();
                if (text[pos-1] == ' ' || text[pos-1] == '\'' || text[pos-1] == '`'|| text[pos-1] == '"') {
                    char subst[32];
                    int nsubst = snprintf(subst,sizeof(subst)," THISISPROTECTED%.3d ",num++);
                    text.replace(pos,len,subst,nsubst);
                    prot_stack.push_back(match);
                    pos += nsubst;
                } else {
                    pos += len;
                }
            }
        }
        
        // collapse spaces
        RE2::GlobalReplace(&text,mult_spc_x," ");

        // strip leading space
        if (text.at(0) == ' ')
            text = text.substr(1);

        // strip trailing space
        if (text.at(text.size()-1) == ' ')
            text = text.substr(0,text.size()-1);

        // isolate hyphens, if non-default option is set
        if (aggressive_hyphen_p) 
            RE2::GlobalReplace(&text,hyphen_x,special_refs);

        // find successive dots, protect them
        pos = text.find("..");
        while (pos != std::string::npos && pos < text.size()) {
            char subst[12];
            size_t lim = pos + 2;
            while (lim < text.size() && text.at(lim) == '.') ++lim;
            snprintf(subst,sizeof(subst),"MANYDOTS%.3d",lim-pos);
            text.replace(pos,lim-pos,subst,11);
            pos = text.find("..",pos+11);
            
        }

        // terminate token at superscript or subscript sequence when followed by lower-case
        RE2::GlobalReplace(&text,numscript_x,"\\1\\2 \\3");

        // isolate commas after non-digits
        RE2::GlobalReplace(&text,postncomma_x,"\\1 , ");

        // isolate commas before non-digits
        RE2::GlobalReplace(&text,prencomma_x," , \\1");

        // replace backtick with single-quote
        pos = text.find("`");
        while (pos != std::string::npos) {
            text.replace(pos,1,"'",1);
            pos = text.find("`");
        }

        // replace doubled single-quotes with double-quotes
        pos = text.find("''");
        while (pos != std::string::npos) {
            text.replace(pos,2,"\"",1);
            pos = text.find("''",pos+1);
        }

        // isolate special characters
        RE2::GlobalReplace(&text,specials_x,isolate_ref);

        if (english_p) {
            // english contractions to the right
            RE2::GlobalReplace(&text,nanaapos_x,apos_refs);
            RE2::GlobalReplace(&text,nxpaapos_x,apos_refs);
            RE2::GlobalReplace(&text,panaapos_x,apos_refs);
            RE2::GlobalReplace(&text,papaapos_x,right_refs);
            RE2::GlobalReplace(&text,pnsapos_x,"\\1 's");
        } else if (latin_p) {
            // italian,french contractions to the left 
            RE2::GlobalReplace(&text,nanaapos_x,apos_refs);
            RE2::GlobalReplace(&text,napaapos_x,apos_refs);
            RE2::GlobalReplace(&text,panaapos_x,apos_refs);
            RE2::GlobalReplace(&text,papaapos_x,left_refs);
        }

        protected_tokenize(text);

        // restore prefix-protected strings
        num = 0;
        for (auto& prot : prot_stack) {
            char subst[32];
            snprintf(subst,sizeof(subst),"THISISPROTECTED%.3d",num++);
            size_t loc = text.find(subst);
            while (loc != std::string::npos) {
                text.replace(loc,18,prot);
                loc = text.find(subst,loc+18);
            }
        }

        // restore dot-sequences with correct length
        std::string numstr;
        pos = 0;
        while (RE2::PartialMatch(text,dotskey_x,&numstr)) {
            int count = std::strtoul(numstr.c_str(),0,0);
            int loc = text.find("MANYDOTS",pos);
            std::ostringstream fss;
            fss << text.substr(0,loc);
            if (loc > 0 && text.at(loc-1) != ' ')
                fss << ' ';
            for (int ii = 0; ii < count; ++ii) 
                fss << '.';
            int sublen = 8 + numstr.size();
            pos = loc + sublen;
            if (pos < text.size() && text.at(pos) != ' ')
                fss << ' ';
            fss << text.substr(pos);
            pos = loc;
            text.assign(fss.str());
        }
        
        // escape moses mark-up
        if (!non_escape_p) 
            escape(text);

        // return value
        outs.assign(text);

    } else {
        // tokenize_penn case

        // directed quote patches
        size_t len = text.size();
        if (len > 2 && text.substr(0,2) == "``") 
            text.replace(0,2,"`` ",3); 
        else if (text.at(0) == '"')
            text.replace(0,1,"`` ",3);
        else if (text.at(0) == '`' || text.at(0) == '\'')
            text.replace(0,1,"` ",2);
        static char one_gg[] = "\\1 ``";
        RE2::GlobalReplace(&text,x1_v_d,one_gg);
        RE2::GlobalReplace(&text,x1_v_gg,one_gg);
        RE2::GlobalReplace(&text,x1_v_g,"\\1 ` \\2");
        RE2::GlobalReplace(&text,x1_v_q,"\\1 ` ");
        
        // protect ellipsis
        for (size_t pos = text.find("..."); pos != std::string::npos; pos = text.find("...",pos+11)) 
            text.replace(pos,3,"MANYELIPSIS",11);

        // numeric commas
        RE2::GlobalReplace(&text,ndndcomma_x,comma_refs);
        RE2::GlobalReplace(&text,pdndcomma_x,comma_refs);
        RE2::GlobalReplace(&text,ndpdcomma_x,comma_refs);

        // isolable symbols
        RE2::GlobalReplace(&text,symbol_x,isolate_ref);

        // isolable slash
        RE2::GlobalReplace(&text,slash_x,special_refs);
        
        // isolate final period
        RE2::GlobalReplace(&text,final_x,"\\1 \\2\\3");
        
        // isolate q.m., e.m.
        RE2::GlobalReplace(&text,qx_x,isolate_ref);
 
        // isolate braces
        RE2::GlobalReplace(&text,braces_x,isolate_ref);

        // convert open/close punctuation
        RE2::GlobalReplace(&text,"\\(","-LRB-");
        RE2::GlobalReplace(&text,"\\[","-LSB-");
        RE2::GlobalReplace(&text,"\\{","-LCB-");
        RE2::GlobalReplace(&text,"\\)","-RRB-");
        RE2::GlobalReplace(&text,"\\]","-RSB-");
        RE2::GlobalReplace(&text,"\\}","-RCB-");

        // isolate double-dash hyphen
        RE2::GlobalReplace(&text,"--"," -- ");

        // insure leading and trailing space on line, to simplify exprs
        // also make sure final . has one space on each side
        len = text.size();
        while (len > 1 && text.at(len-1) == ' ') --len;
        if (len < text.size())
            text.assign(text.substr(0,len));
        if (len > 2 && text.at(len-1) == '.') {
            if (text.at(len-2) != ' ') {
                text.assign(text.substr(0,len-1));
                text.append(" . ");
            } else {
                text.assign(text.substr(0,len-1));
                text.append(". ");
            }
        } else {
            text.append(" ");
        }
        std::string ntext(" ");
        ntext.append(text);
        
        // convert double quote to paired single-quotes
        RE2::GlobalReplace(&ntext,"\""," '' ");

        // deal with contractions in penn style
        RE2::GlobalReplace(&ntext,endq_x,"\\1 ' ");
        RE2::GlobalReplace(&ntext,contract_x," '\\1 ");
        RE2::GlobalReplace(&ntext,"'ll "," 'll ");
        RE2::GlobalReplace(&ntext,"'re "," 're ");
        RE2::GlobalReplace(&ntext,"'ve "," 've ");
        RE2::GlobalReplace(&ntext,"n't "," n't ");
        RE2::GlobalReplace(&ntext,"'LL "," 'LL ");
        RE2::GlobalReplace(&ntext,"'RE "," 'RE ");
        RE2::GlobalReplace(&ntext,"'VE "," 'VE ");
        RE2::GlobalReplace(&ntext,"N'T "," N'T ");
        RE2::GlobalReplace(&ntext," ([Cc])annot "," \\1an not ");
        RE2::GlobalReplace(&ntext," ([Dd])'ye "," \\1' ye ");
        RE2::GlobalReplace(&ntext," ([Gg])imme "," \\1im me ");
        RE2::GlobalReplace(&ntext," ([Gg])onna "," \\1on na ");
        RE2::GlobalReplace(&ntext," ([Gg])otta "," \\1ot ta ");
        RE2::GlobalReplace(&ntext," ([Ll])emme "," \\1em me ");
        RE2::GlobalReplace(&ntext," ([Mm])ore'n "," \\1ore 'n ");
        RE2::GlobalReplace(&ntext," '([Tt])is "," '\\1 is 'n ");
        RE2::GlobalReplace(&ntext," '([Tt])was "," '\\1 was 'n ");
        RE2::GlobalReplace(&ntext," '([Tt])were "," '\\1 were 'n ");
        RE2::GlobalReplace(&ntext," ([Ww])anna "," \\1an na ");

        protected_tokenize(ntext);
        
        // restore ellipsis
        RE2::GlobalReplace(&ntext,"MANYELIPSIS","...");

        // collapse spaces
        RE2::GlobalReplace(&ntext,mult_spc_x," ");

        // escape moses meta-characters
        if (!non_escape_p)
            escape(ntext);

        // strip out wrapping spaces from line in result string
        outs.assign(ntext.substr(1,ntext.size()-2));
    }

    return outs;
}


std::size_t 
Tokenizer::tokenize(std::istream& is, std::ostream& os)
{
    size_t line_no = 0;
    while (is.good() && os.good()) {
        std::string istr;
        std::getline(is,istr);
        line_no ++;
        if (istr.empty()) 
            continue;
        if (skip_xml_p && RE2::FullMatch(istr,tag_line_x) || RE2::FullMatch(istr,white_line_x)) {
            os << istr << std::endl;
        } else {
            std::string bstr(" ");
            bstr.append(istr).append(" ");
            os << tokenize(bstr) << std::endl;
        }
        if (verbose_p && ((line_no % 1000) == 0)) {
            std::cerr << line_no << ' ';
            std::cerr.flush();
        }
    }
    return line_no;
}


namespace {

std::string trim(const std::string& in)
{
    std::size_t start = 0;
    std::size_t limit = in.size();
    while (start < limit && in.at(start) < '!') ++start;
    while (start < limit && in.at(limit-1) < '!') --limit;
    if (start == limit) return std::string("");
    if (start > 0 || limit < in.size())
        return in.substr(start,limit-start);
    return std::string(in);
}


std::vector<std::string> split(const std::string& in)
{
    std::vector<std::string> outv;
    std::istringstream iss(in);
    std::copy(std::istream_iterator<std::string>(iss),
              std::istream_iterator<std::string>(),
              std::back_inserter(outv));
    return outv;
}

};


std::string
Tokenizer::detokenize(const std::string& buf)
{
    std::vector<std::string> words = split(trim(buf));
    
    std::size_t squotes = 0;
    std::size_t dquotes = 0;
    std::string prepends(" ");

    std::ostringstream oss;
    
    std::size_t nwords = words.size();
    std::size_t iword = 0;

    for (auto word: words) {
        if (RE2::FullMatch(word,right_x)) {
            oss << prepends << word;
            prepends.clear();
        } else if (RE2::FullMatch(word,left_x)) {
            oss << word;
            prepends = " ";
        } else if (english_p && iword && RE2::FullMatch(word,curr_en_x) && RE2::FullMatch(words[iword-1],pre_en_x)) {
            oss << word;
            prepends = " ";
        } else if (latin_p && iword < nwords - 2 && RE2::FullMatch(word,curr_fr_x) && RE2::FullMatch(words[iword+1],post_fr_x)) {
            oss << prepends << word;
            prepends.clear();
        } else if (word.size() == 1) {
            if ((word.at(0) == '\'' && ((squotes % 2) == 0 )) ||
                (word.at(0) == '"' && ((dquotes % 2) == 0))) {
                if (english_p && iword && word.at(0) == '\'' && words[iword-1].at(words[iword-1].size()-1) == 's') {
                    oss << word;
                    prepends = " ";
				} else {
                    oss << prepends << word;
                    prepends.clear();
                    if (word.at(0) == '\'')
                        squotes++;
                    else
                        dquotes++;
                }
			} else {
                oss << word;
                prepends = " ";
                if (word.at(0) == '\'')
                    squotes++;
                else if (word.at(0) == '"') 
                    dquotes++;
			}
		} else {
            oss << prepends << word;
            prepends.clear();
		}
        iword++;
	}
	
    
    std::string text(oss.str());
    RE2::GlobalReplace(&text," +"," ");
    RE2::GlobalReplace(&text,"\n ","\n");
    RE2::GlobalReplace(&text," \n","\n");
    return trim(text);
}


std::size_t
Tokenizer::detokenize(std::istream& is, std::ostream& os) 
{
    size_t line_no = 0;
    while (is.good() && os.good()) {
        std::string istr;
        std::getline(is,istr);
        line_no ++;
        if (istr.empty()) 
            continue;
        if (skip_xml_p && RE2::FullMatch(istr,tag_line_x) || RE2::FullMatch(istr,white_line_x)) {
            os << istr << std::endl;
        } else {
            os << detokenize(istr) << std::endl;
        }
    }
    return line_no;
}


#ifdef TOKENIZER_NAMESPACE
}; // namespace
#endif

