#include "tokenizer.h"
#include <re2/stringpiece.h>
#include <sstream>
#include <iterator>
#include <memory>
#include <vector>
#include <algorithm>
#include <cstring>
#include <glib.h>

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

const char *SPC_BYTE = " ";
//const char *URL_VALID_SYM_CHARS = "-._~:/?#[]@!$&'()*+,;=";

inline bool
class_follows_p(gunichar *s, gunichar *e, GUnicodeType gclass) {
    while (s < e) {
        GUnicodeType tclass = g_unichar_type(*s);
        if (tclass == gclass)
            return true;
        switch (tclass) {
        case G_UNICODE_SPACING_MARK:
        case G_UNICODE_LINE_SEPARATOR:
        case G_UNICODE_PARAGRAPH_SEPARATOR:
        case G_UNICODE_SPACE_SEPARATOR:
            ++s;
            continue;
            break;
        default:
            return false;
        }
    }
    return false;
}

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
                     bool _supersub_p,
                     bool _url_p,
                     bool _downcase_p,
                     bool _normalize_p,
                     bool _penn_p,
                     bool _verbose_p)
        : lang_iso(_lang_iso)
        , english_p(_lang_iso.compare("en")==0)
        , latin_p((!english_p) && (_lang_iso.compare("fr")==0 || _lang_iso.compare("it")==0))
        , skip_xml_p(_skip_xml_p)
        , skip_alltags_p(_skip_alltags_p)
        , non_escape_p(_non_escape_p)
        , aggressive_hyphen_p(_aggressive_hyphen_p)
        , supersub_p(_supersub_p)
        , url_p(_url_p)
        , downcase_p(_downcase_p)
        , normalize_p(_normalize_p)
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
        if (!line.empty() && line[0] != '#') {
            std::string prefix;
            if (RE2::PartialMatch(line,numonly,&prefix)) {
                nbpre_num_set.insert(prefix);
                gunichar * x=g_utf8_to_ucs4_fast((const gchar *)prefix.c_str(),prefix.size(),0);
                nbpre_num_ucs4.insert(std::wstring((wchar_t *)x));
                g_free(x);
                nnum++;
            } else {
                nbpre_gen_set.insert(line);
                gunichar * x=g_utf8_to_ucs4_fast((const gchar *)line.c_str(),line.size(),0);
                nbpre_gen_ucs4.insert(std::wstring((wchar_t *)x));
                g_free(x);
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
// assumes protections are applied already, some invariants are in place, 
// e.g. that successive chars <= ' ' have been normalized to a single ' '
//
void
Tokenizer::protected_tokenize(std::string& text) {
    std::vector<re2::StringPiece> words;
    re2::StringPiece textpc(text);
    int pos = 0;
    if (textpc[pos] == ' ')
        ++pos;
    size_t next = text.find(' ',pos);
    while (next != std::string::npos) {
        if (next - pos)
            words.push_back(textpc.substr(pos,next-pos));
        pos = next + 1;
        while (pos < textpc.size() && textpc[pos] == ' ')
            ++pos;
        next = textpc.find(' ',pos);
    }
    if (pos < textpc.size() && textpc[pos] != ' ')
        words.push_back(textpc.substr(pos,textpc.size()-pos));
    
    // regurgitate words with look-ahead handling for tokens with final mumble
    std::string outs;
    std::size_t nwords(words.size());
    for (size_t ii = 0; ii < nwords; ++ii) {
        bool more_p = ii < nwords - 1;
        size_t len = words[ii].size();
        bool sentence_break_p = len > 1 && words[ii][len-1] == '.';

        // suppress break if it is an non-breaking prefix
        if (sentence_break_p) {
            re2::StringPiece pfx(words[ii].substr(0,len-1));
            std::string pfxs(pfx.as_string());
            if (nbpre_gen_set.find(pfxs) != nbpre_gen_set.end()) {
                // general non-breaking prefix
                sentence_break_p = false;
            } else if (more_p && nbpre_num_set.find(pfxs) != nbpre_num_set.end() && RE2::PartialMatch(words[ii+1],sinteger_x)) {
                // non-breaking before numeric
                sentence_break_p = false;
            } else if (pfxs.find('.') != std::string::npos && RE2::PartialMatch(pfx,letter_x)) {
                // terminal isolated letter does not break
                sentence_break_p = false;
            } else if (more_p && RE2::PartialMatch(words[ii+1],lower_x)) {
                // lower-case look-ahead does not break
                sentence_break_p = false;
            }
        } 

        outs.append(words[ii].data(),len);
        if (sentence_break_p)
            outs.append(" .");
        if (more_p)
            outs.append(SPC_BYTE,1);
    }
    text.assign(outs.begin(),outs.end());
}


bool
Tokenizer::escape(std::string& text) {
    bool mod_p = false;
    std::string outs;

    static const char *replacements[] = {
        "&#124;", // | 0
        "&#91;", // [ 1
        "&#93;",  // ] 2
        "&amp;", // & 3
        "&lt;", // < 4
        "&gt;", // > 5
        "&apos;", // ' 6
        "&quot;", // " 7
    };
    
    const char *pp = text.c_str(); // from pp to pt is uncopied
    const char *ep = pp + text.size();
    const  char *pt = pp;

    while (pt < ep) {
        if (*pt & 0x80) {
            const char *mk = (const char *)g_utf8_find_next_char((const gchar *)pt,(const gchar *)ep);
            if (!mk) {
                if (mod_p)
                    outs.append(pp,pt-pp+1);
            } else {
                if (mod_p) 
                    outs.append(pp,mk-pp);
                pt = --mk;
            }
            pp = ++pt;
            continue;
        }

        const char *sequence_p = 0;
        if (*pt < '?') {
            if (*pt == '&') {
                sequence_p = replacements[3];
            } else if (*pt == '\'') {
                sequence_p = replacements[6];
            } else if (*pt == '"') {
                sequence_p = replacements[7];
            }
        } else if (*pt > ']') {
            if (*pt =='|') { // 7c
                sequence_p = replacements[0];
            } 
        } else if (*pt > 'Z') {
            if (*pt == '<') { // 3e
                sequence_p = replacements[4];
            } else if (*pt == '>') { // 3c
                sequence_p = replacements[5];
            } else if (*pt == '[') { // 5b
                sequence_p = replacements[1];
            } else if (*pt == ']') { // 5d
                sequence_p = replacements[2];
            } 
        }

        if (sequence_p) {
            if (pt > pp) 
                outs.append(pp,pt-pp);
            outs.append(sequence_p);
            mod_p = true;
            pp = ++pt;
        } else {
            ++pt;
        }
    }
    
    if (mod_p) {
        if (pp < pt) {
            outs.append(pp,pt-pp);
        }
        text.assign(outs.begin(),outs.end());
    }

    return mod_p;
}


std::string
Tokenizer::tokenize(const std::string& buf)
{
    static const char *comma_refs = "\\1 , \\2";
    static const char *isolate_ref = " \\1 ";
    static const char *special_refs = "\\1 @\\2@ \\3";

    std::string text(buf);
    std::string outs;
    if (skip_alltags_p) 
        RE2::GlobalReplace(&text,genl_tags_x,SPC_BYTE);

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
        
        const char *pt(text.c_str());
        const char *ep(pt + text.size());
        while (pt < ep && *pt >= 0 && *pt <= ' ')
            ++pt;
        glong ulen(0);
        gunichar *usrc(g_utf8_to_ucs4_fast((const gchar *)pt,ep - pt, &ulen)); // g_free
        gunichar *ucs4(usrc);
        gunichar *lim4(ucs4 + ulen);

        gunichar *nxt4 = ucs4;
        gunichar *ubuf(g_new0(gunichar,ulen*6+1)); // g_free
        gunichar *uptr(ubuf);

        gunichar prev_uch(0L);
        gunichar next_uch(*ucs4);
        gunichar curr_uch(0L);

        GUnicodeType curr_type(G_UNICODE_UNASSIGNED);
        GUnicodeType next_type((ucs4 && *ucs4) ? g_unichar_type(*ucs4) : G_UNICODE_UNASSIGNED);
        GUnicodeType prev_type(G_UNICODE_UNASSIGNED);

        bool post_break_p = false;
        bool in_num = next_uch <= gunichar('9') && next_uch >= gunichar('0');
        bool in_url_p = false;
        bool final_p = false;
        int since_start = 0;
        int alpha_prefix = 0;

        while (ucs4 < lim4) {
            prev_uch = curr_uch;
            prev_type = curr_type;
            curr_uch = next_uch;
            curr_type = next_type;

            final_p = ++nxt4 >= lim4;

            if (final_p) {
                next_uch = gunichar(0L);
                next_type = G_UNICODE_UNASSIGNED;
            } else {
                next_uch = *nxt4;
                next_type = g_unichar_type(next_uch);
            }

            bool is_basic = *ucs4 < 0x80L;

            if (url_p) {
                if (!in_url_p) {
                    if (!since_start) {
                        if (is_basic && std::isalpha(char(*ucs4)))
                            alpha_prefix++;
                    } else if (alpha_prefix == since_start && is_basic && char(*ucs4) == ':' && next_type != G_UNICODE_SPACE_SEPARATOR) {
                        in_url_p = true;
                    }
                }
            }

            bool break_p = false;
            const wchar_t *substitute_p = 0;

            if (post_break_p) {
                *uptr++ = gunichar(' ');
                since_start = 0;
                in_url_p = in_num = post_break_p = false;
            }

            switch (curr_type) {
            case G_UNICODE_MODIFIER_LETTER:
            case G_UNICODE_OTHER_LETTER:
            case G_UNICODE_TITLECASE_LETTER:
                if (in_url_p || in_num)
                    break_p = true;
                // fallthough
            case G_UNICODE_UPPERCASE_LETTER:
            case G_UNICODE_LOWERCASE_LETTER:
                if (downcase_p && curr_type == G_UNICODE_UPPERCASE_LETTER) 
                    curr_uch = g_unichar_tolower(*ucs4);
                break;
            case G_UNICODE_SPACING_MARK:
                break_p = true;
                in_num = false;
                curr_uch = gunichar(0L);
                break;
            case G_UNICODE_DECIMAL_NUMBER:
            case G_UNICODE_LETTER_NUMBER:
            case G_UNICODE_OTHER_NUMBER:
                if (!in_num && !in_url_p) {
                    switch (prev_type) {
                    case G_UNICODE_DASH_PUNCTUATION:
                    case G_UNICODE_FORMAT:
                    case G_UNICODE_OTHER_PUNCTUATION:
                    case G_UNICODE_UPPERCASE_LETTER:
                    case G_UNICODE_LOWERCASE_LETTER:
                    case G_UNICODE_DECIMAL_NUMBER:
                        break;
                    default:
                        break_p = true;
                    }
                }
                in_num = true;
                break;
            case G_UNICODE_CONNECT_PUNCTUATION:
                if (curr_uch != gunichar(L'_')) {
                    if (in_url_p) {
                        in_url_p = false;
                        post_break_p = break_p = true;
                    }
                }
                if (in_num) {
                    post_break_p = break_p = true;
                } else {
                    switch (next_type) {
                    case G_UNICODE_LOWERCASE_LETTER:
                    case G_UNICODE_MODIFIER_LETTER:
                    case G_UNICODE_OTHER_LETTER:
                    case G_UNICODE_TITLECASE_LETTER:
                        break;
                    default:
                        post_break_p = break_p = true;
                    }
                    switch (prev_type) {
                    case G_UNICODE_LOWERCASE_LETTER:
                    case G_UNICODE_MODIFIER_LETTER:
                    case G_UNICODE_OTHER_LETTER:
                    case G_UNICODE_TITLECASE_LETTER:
                        break;
                    default:
                        post_break_p = break_p = true;
                    }
                }
                break;
            case G_UNICODE_DASH_PUNCTUATION:
            case G_UNICODE_FORMAT:
                if (aggressive_hyphen_p) {
                    substitute_p = L"@-@";
                    break_p = post_break_p = !in_url_p;
                } else if (next_type == G_UNICODE_SPACE_SEPARATOR) {
                } else if (prev_type == curr_type) {
                    if (next_type != curr_type) {
                        post_break_p = !in_url_p;
                    }
                } else if (next_type == curr_type) {
                    break_p = !in_url_p;
                } else if ((prev_type == G_UNICODE_UPPERCASE_LETTER ||
                            prev_type == G_UNICODE_LOWERCASE_LETTER) &&
                           next_type == G_UNICODE_DECIMAL_NUMBER) {
                    in_num = false;
                } else if (in_num || since_start == 0) {
                    switch (next_type) {
                    case G_UNICODE_UPPERCASE_LETTER:
                    case G_UNICODE_LOWERCASE_LETTER:
                    case G_UNICODE_MODIFIER_LETTER:
                    case G_UNICODE_OTHER_LETTER:
                    case G_UNICODE_TITLECASE_LETTER:
                    case G_UNICODE_DECIMAL_NUMBER:
                    case G_UNICODE_LETTER_NUMBER:
                    case G_UNICODE_OTHER_NUMBER:
                    case G_UNICODE_SPACE_SEPARATOR:
                        break;
                    default:
                        post_break_p = break_p = prev_uch != curr_uch;
                    }
                } else if (in_url_p) {
                    break_p = curr_uch != gunichar('-');
                } else {
                    switch (prev_type) {
                    case G_UNICODE_UPPERCASE_LETTER:
                    case G_UNICODE_LOWERCASE_LETTER:
                    case G_UNICODE_MODIFIER_LETTER:
                    case G_UNICODE_OTHER_LETTER:
                    case G_UNICODE_TITLECASE_LETTER:
                    case G_UNICODE_DECIMAL_NUMBER:
                    case G_UNICODE_LETTER_NUMBER:
                    case G_UNICODE_OTHER_NUMBER:
                    case G_UNICODE_OTHER_PUNCTUATION:
                        switch (next_type) {
                        case G_UNICODE_UPPERCASE_LETTER:
                        case G_UNICODE_LOWERCASE_LETTER:
                        case G_UNICODE_MODIFIER_LETTER:
                        case G_UNICODE_OTHER_LETTER:
                        case G_UNICODE_TITLECASE_LETTER:
                        case G_UNICODE_DECIMAL_NUMBER:
                        case G_UNICODE_LETTER_NUMBER:
                        case G_UNICODE_OTHER_NUMBER:
                            break;
                        default:
                            post_break_p = break_p = prev_uch != curr_uch;
                        }
                        break;
                    default:
                        post_break_p = break_p = prev_uch != curr_uch;
                        break;
                    } 
                }
                break;
            case G_UNICODE_OTHER_PUNCTUATION:
                switch (curr_uch) {
                case gunichar('!'):
                case gunichar('#'):
                case gunichar('/'):
                case gunichar(':'):
                case gunichar(';'):
                case gunichar('?'):
                case gunichar('@'):
                    post_break_p = break_p = !in_url_p || next_type != G_UNICODE_SPACE_SEPARATOR;
                    break;
                case gunichar('+'):
                    post_break_p = break_p = !in_num && since_start > 0;
                    in_num = in_num || since_start == 0;
                    break;
                case gunichar('&'):
                    post_break_p = break_p = !in_url_p || next_type != G_UNICODE_SPACE_SEPARATOR;
                    if (!non_escape_p) 
                        substitute_p = L"&amp;";
                    break;
                case gunichar('\''):
                    if (english_p) {
                        if (!in_url_p) {
                            break_p = true;
                            post_break_p = since_start == 0 || 
                                (next_type != G_UNICODE_LOWERCASE_LETTER && next_type != G_UNICODE_UPPERCASE_LETTER && next_type != G_UNICODE_DECIMAL_NUMBER);
                        }
                    } else if (latin_p) {
                        post_break_p = !in_url_p;
                        break_p = !in_url_p && prev_type != G_UNICODE_LOWERCASE_LETTER && prev_type != G_UNICODE_UPPERCASE_LETTER;
                    } else {
                        post_break_p = break_p = !in_url_p;
                    }
                    if (!non_escape_p) 
                        substitute_p = L"&apos;";
                    break;
                case gunichar('"'):
                    post_break_p = break_p = true;
                    if (!non_escape_p) 
                        substitute_p = L"&quot;";
                    break;
                case gunichar(','):
                    break_p = !in_num || next_type != G_UNICODE_DECIMAL_NUMBER;
                    break;
                case gunichar('.'):
                    if (prev_uch != '.') {
                        if (!in_num) {
                            switch (next_type) {
                            case G_UNICODE_DECIMAL_NUMBER:
                            case G_UNICODE_LOWERCASE_LETTER:
                            case G_UNICODE_UPPERCASE_LETTER:
                                break;
                            default:
                                if (since_start > 0) {
                                    switch (prev_type) {
                                    case G_UNICODE_LOWERCASE_LETTER:
                                    case G_UNICODE_UPPERCASE_LETTER: {
                                        std::wstring k((wchar_t *)(uptr-since_start),since_start);
                                        if (nbpre_gen_ucs4.find(k) != nbpre_gen_ucs4.end()) {
                                            // general non-breaking prefix
                                        } else if (nbpre_num_ucs4.find(k) != nbpre_num_ucs4.end() && class_follows_p(nxt4,lim4,G_UNICODE_DECIMAL_NUMBER)) {
                                            // non-breaking before numeric
                                        } else if (k.find(curr_uch) != std::wstring::npos) {
                                            if (since_start > 1) {
                                                GUnicodeType tclass = g_unichar_type(*(uptr-2));
                                                switch (tclass) {
                                                case G_UNICODE_UPPERCASE_LETTER:
                                                case G_UNICODE_LOWERCASE_LETTER:
                                                    break_p = true;
                                                    break;
                                                default:
                                                    break;
                                                }
                                            }
                                            // terminal isolated letter does not break
                                        } else if (class_follows_p(nxt4,lim4,G_UNICODE_LOWERCASE_LETTER) || 
                                                   g_unichar_type(*nxt4) == G_UNICODE_DASH_PUNCTUATION) {
                                            // lower-case look-ahead does not break
                                        } else {
                                            break_p = true;
                                        }
                                        break;
                                    }
                                    default:
                                        break_p = true;
                                        break;
                                    }
                                } 
                                break;
                            }
                        } else {
                            switch (next_type) {
                            case G_UNICODE_DECIMAL_NUMBER:
                            case G_UNICODE_LOWERCASE_LETTER:
                                break;
                            default:
                                break_p = true;
                            }
                        }
                    } else if (next_uch != '.') {
                        post_break_p = true;
                    }
                    break;
                default:
                    post_break_p = break_p = true;
                    break;
                }
                break;
            case G_UNICODE_CLOSE_PUNCTUATION:
            case G_UNICODE_FINAL_PUNCTUATION:
            case G_UNICODE_INITIAL_PUNCTUATION:
            case G_UNICODE_OPEN_PUNCTUATION:
                switch (curr_uch) {
                case gunichar('('):
                case gunichar(')'):
                    break;
                case gunichar('['):
                    if (!non_escape_p) 
                        substitute_p = L"&#91;";
                    break;
                case gunichar(']'):
                    if (!non_escape_p) 
                        substitute_p = L"&#93;";
                    break;
                default:
                    in_url_p = false;
                }
                post_break_p = break_p = !in_url_p;
                break;
            case G_UNICODE_CURRENCY_SYMBOL:
                post_break_p = in_num; // was in number, so break it
                break_p = !in_num;
                in_num = in_num || next_type == G_UNICODE_DECIMAL_NUMBER || next_uch == gunichar('.') || next_uch == gunichar(',');
                if (curr_uch != gunichar('$'))
                    in_url_p = false;
                break;
            case G_UNICODE_MODIFIER_SYMBOL:
            case G_UNICODE_MATH_SYMBOL:
                switch (curr_uch) {
                case gunichar('`'):
                    if (english_p) {
                        if (!in_url_p) {
                            break_p = true;
                            post_break_p = since_start == 0 || 
                                (next_type != G_UNICODE_LOWERCASE_LETTER && next_type != G_UNICODE_UPPERCASE_LETTER && next_type != G_UNICODE_DECIMAL_NUMBER);
                        }
                    } else if (latin_p) {
                        post_break_p = !in_url_p;
                        break_p = !in_url_p && prev_type != G_UNICODE_LOWERCASE_LETTER && prev_type != G_UNICODE_UPPERCASE_LETTER;
                    } else {
                        post_break_p = break_p = !in_url_p;
                    }
                    if (!non_escape_p) 
                        substitute_p = L"&apos;";
                    else 
                        curr_uch = gunichar('\'');
                    break;
                case gunichar('|'):
                    if (!non_escape_p) 
                        substitute_p = L"&#124;";
                    post_break_p = break_p = true;
                    break;
                case gunichar('<'):
                    if (!non_escape_p) 
                        substitute_p = L"&lt;";
                    post_break_p = break_p = true;
                    break;
                case gunichar('>'):
                    if (!non_escape_p) 
                        substitute_p = L"&gt;";
                    post_break_p = break_p = true;
                    break;
                case gunichar('%'):
                    post_break_p = in_num;
                    break_p = !in_num && !in_url_p;
                    in_num = false;
                    break;
                case gunichar('='):
                case gunichar('~'):
                    in_num = false;
                    post_break_p = break_p = !in_url_p; 
                    break;
                case gunichar('+'):
                    in_num = in_num || since_start == 0;
                    post_break_p = break_p = !in_url_p; 
                    break;
                default:
                    post_break_p = break_p = true;
                    break;
                }
                break;
            case G_UNICODE_OTHER_SYMBOL:
                post_break_p = break_p = true;
                break;
            case G_UNICODE_LINE_SEPARATOR:
                curr_uch = gunichar(' ');
                in_url_p = in_num = false;
                break;
            case G_UNICODE_SPACE_SEPARATOR:
                curr_uch = gunichar(' ');
                in_url_p = in_num = false;
                break;
            default:
                curr_uch = 0;
                in_url_p = in_num = false;
                break;
            }
            
            if ((break_p || curr_uch == gunichar(' '))) {
                if (since_start) {
                    *uptr++ = gunichar(' ');
                    in_url_p = false;
                    in_num = in_num && !post_break_p;
                    since_start = 0;
                }
                if (curr_uch == gunichar(' '))
                    curr_uch = gunichar(0L);
            } 
            
            if (substitute_p) {
                for (gunichar *sptr = (gunichar *)substitute_p; *sptr; ++sptr) {
                    *uptr++ = *sptr;
                    since_start++;
                }
                in_url_p = in_num = false;
            } else if (curr_uch) {
                *uptr++ = curr_uch;
                since_start++;
            }

            ucs4 = nxt4;
        }

        glong nbytes = 0;
        gchar *utf8 = g_ucs4_to_utf8(ubuf,uptr-ubuf,0,&nbytes,0); // g_free
        if (utf8[nbytes-1] == ' ') 
            --nbytes;
        text.assign((const char *)utf8,(const char *)(utf8 + nbytes));
        g_free(utf8);
        g_free(usrc);
        g_free(ubuf);

        // terminate token at superscript or subscript sequence when followed by lower-case
        if (supersub_p)
            RE2::GlobalReplace(&text,numscript_x,"\\1\\2 \\3");

        // restore prefix-protected strings
        num = 0;
        for (auto& prot : prot_stack) {
            char subst[32];
            snprintf(subst,sizeof(subst),"THISISPROTECTED%.3d",num++);
            size_t loc = text.find(subst);
            while (loc != std::string::npos) {
                text.replace(loc,18,prot.data(),prot.size());
                loc = text.find(subst,loc+18);
            }
        }

        // return value
        outs.assign(text);

    } else {
        // tokenize_penn case

        // directed quote patches
        size_t len = text.size();
        if (len > 2 && text.substr(0,2) == "``") 
            text.replace(0,2,"`` ",3); 
        else if (text[0] == '"')
            text.replace(0,1,"`` ",3);
        else if (text[0] == '`' || text[0] == '\'')
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
        while (len > 1 && text[len-1] == ' ') --len;
        if (len < text.size())
            text.assign(text.substr(0,len));
        if (len > 2 && text[len-1] == '.') {
            if (text[len-2] != ' ') {
                text.assign(text.substr(0,len-1));
                text.append(" . ");
            } else {
                text.assign(text.substr(0,len-1));
                text.append(". ");
            }
        } else {
            text.append(SPC_BYTE,1);
        }
        std::string ntext(SPC_BYTE);
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
        RE2::GlobalReplace(&ntext,mult_spc_x,SPC_BYTE);

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
        if (skip_xml_p && (RE2::FullMatch(istr,tag_line_x) || RE2::FullMatch(istr,white_line_x))) {
            os << istr << std::endl;
        } else {
            std::string bstr(SPC_BYTE);
            bstr.append(istr).append(SPC_BYTE);
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
    std::string prepends(SPC_BYTE);

    std::ostringstream oss;
    
    std::size_t nwords = words.size();
    std::size_t iword = 0;

    for (auto word: words) {
        if (RE2::FullMatch(word,right_x)) {
            oss << prepends << word;
            prepends.clear();
        } else if (RE2::FullMatch(word,left_x)) {
            oss << word;
            prepends = SPC_BYTE;
        } else if (english_p && iword && RE2::FullMatch(word,curr_en_x) && RE2::FullMatch(words[iword-1],pre_en_x)) {
            oss << word;
            prepends = SPC_BYTE;
        } else if (latin_p && iword < nwords - 2 && RE2::FullMatch(word,curr_fr_x) && RE2::FullMatch(words[iword+1],post_fr_x)) {
            oss << prepends << word;
            prepends.clear();
        } else if (word.size() == 1) {
            if ((word.at(0) == '\'' && ((squotes % 2) == 0 )) ||
                (word.at(0) == '"' && ((dquotes % 2) == 0))) {
                if (english_p && iword && word.at(0) == '\'' && words[iword-1].at(words[iword-1].size()-1) == 's') {
                    oss << word;
                    prepends = SPC_BYTE;
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
                prepends = SPC_BYTE;
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
    RE2::GlobalReplace(&text," +",SPC_BYTE);
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
        if (skip_xml_p && (RE2::FullMatch(istr,tag_line_x) || RE2::FullMatch(istr,white_line_x))) {
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

