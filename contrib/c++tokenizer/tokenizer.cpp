#include "tokenizer.h"
#include <re2/stringpiece.h>
#include <sstream>
#include <iterator>
#include <memory>
#include <vector>
#include <algorithm>
#include <cstring>
#include <set>
#include <glib.h>
#include <stdexcept>
#include <boost/thread.hpp>

namespace { // anonymous namespace

// frequently used regexp's are pre-compiled thus:

RE2 genl_tags_x("<[/!\\p{L}]+[^>]*>");
RE2 mult_spc_x(" +"); // multiple spaces
RE2 tag_line_x("^<.+>$"); // lines beginning and ending with open/close angle-bracket pairs
RE2 white_line_x("^\\s*$"); // lines entirely composed of whitespace
RE2 slash_x("([\\p{L}\\p{N}])(/)([\\p{L}\\p{N}])"); // and slash-conjoined " "
RE2 final_x("([^.])([.])([\\]\\)}>\"']*) ?$"); // sentence-final punctuation sequence (non qm em)
RE2 qx_x("([?!])"); // one qm/em mark
RE2 braces_x("([\\]\\[\\(\\){}<>])"); // any open or close of a pair
RE2 endq_x("([^'])' "); // post-token single-quote or doubled single-quote
RE2 letter_x("\\p{L}"); // a letter
RE2 lower_x("^\\p{Ll}"); // a lower-case letter
RE2 sinteger_x("^\\p{N}"); // not a digit mark
RE2 numprefixed_x("[-+/.@\\\\#\\%&\\p{Sc}\\p{N}]*[\\p{N}]+-[-'`\"\\p{L}]*\\p{L}");
RE2 quasinumeric_x("[-.;:@\\\\#\%&\\p{Sc}\\p{So}\\p{N}]*[\\p{N}]+");
RE2 numscript_x("([\\p{N}\\p{L}])([\\p{No}]+)(\\p{Ll})");

RE2 x1_v_d("([ ([{<])\""); // a valid non-letter preceeding a double-quote
RE2 x1_v_gg("([ ([{<])``"); // a valid non-letter preceeding directional doubled open single-quote
RE2 x1_v_g("([ ([{<])`([^`])"); //  a valid non-letter preceeding directional unitary single-quote
RE2 x1_v_q("([ ([{<])'"); //  a valid non-letter preceeding undirected embedded quotes
RE2 ndndcomma_x("([^\\p{N}]),([^\\p{N}])"); // non-digit,non-digit
RE2 pdndcomma_x("([\\p{N}]),([^\\p{N}])"); // digit,non-digit
RE2 ndpdcomma_x("([^\\p{N}]),([\\p{N}])"); // non-digit,digit
RE2 symbol_x("([;:@\\#\\$%&\\p{Sc}\\p{So}])"); // usable punctuation mark not a quote or a brace
RE2 contract_x("'([sSmMdD]) "); // english single letter contraction forms, as embedded
RE2 right_x("[({¿¡]+"); // symbols which conjoin to the right
RE2 left_x("[,.?!:;\\%\\p{Sc}})]+"); // symbols conjoin to the left
RE2 curr_en_x("^[Nn]?[\'][\\p{L}]"); // english contraction suffixes conjoin to the left
RE2 pre_en_x(".*[\\p{L}\\p{N}]+$"); // valid english contraction prefixes
RE2 curr_fr_x(".*[\\p{L}\\p{N}]+[\']"); // french/italian contraction prefixes conjoin to the right
RE2 post_fr_x("^[\\p{L}\\p{N}]*"); // valid french/italian contraction suffixes
// anything rarely used will just be given as a string and compiled on demand by RE2

const char *
SPC_BYTE = " ";
//const char *
//URL_VALID_SYM_CHARS = "-._~:/?#[]@!$&'()*+,;=";

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


const char *ESCAPE_MOSES[] = {
        "&#124;", // | 0
        "&#91;", // [ 1
        "&#93;",  // ] 2
        "&amp;", // & 3 (26)
        "&lt;", // < 4 (3c)
        "&gt;", // > 5 (3e)
        "&apos;", // ' 6 (27)
        "&quot;", // " 7 (22)
};

const std::set<std::string>
ESCAPE_SET = {
    std::string(ESCAPE_MOSES[0]),
    std::string(ESCAPE_MOSES[1]),
    std::string(ESCAPE_MOSES[2]),
    std::string(ESCAPE_MOSES[3]),
    std::string(ESCAPE_MOSES[4]),
    std::string(ESCAPE_MOSES[5]),
    std::string(ESCAPE_MOSES[6]),
    std::string(ESCAPE_MOSES[7]),
};

const std::map<std::wstring,gunichar>
ENTITY_MAP = {
    { std::wstring(L"&quot;"), L'"' },
    { std::wstring(L"&amp;"), L'&' },
    { std::wstring(L"&apos;"), L'\'' },
    { std::wstring(L"&lt;"), L'<' },
    { std::wstring(L"&gt;"), L'>' },
    { std::wstring(L"&nbsp;"), L'\u00A0' },
    { std::wstring(L"&iexcl;"), L'\u00A1' },
    { std::wstring(L"&cent;"), L'\u00A2' },
    { std::wstring(L"&pound;"), L'\u00A3' },
    { std::wstring(L"&curren;"), L'\u00A4' },
    { std::wstring(L"&yen;"), L'\u00A5' },
    { std::wstring(L"&brvbar;"), L'\u00A6' },
    { std::wstring(L"&sect;"), L'\u00A7' },
    { std::wstring(L"&uml;"), L'\u00A8' },
    { std::wstring(L"&copy;"), L'\u00A9' },
    { std::wstring(L"&ordf;"), L'\u00AA' },
    { std::wstring(L"&laquo;"), L'\u00AB' },
    { std::wstring(L"&not;"), L'\u00AC' },
    { std::wstring(L"&shy;"), L'\u00AD' },
    { std::wstring(L"&reg;"), L'\u00AE' },
    { std::wstring(L"&macr;"), L'\u00AF' },
    { std::wstring(L"&deg;"), L'\u00B0' },
    { std::wstring(L"&plusmn;"), L'\u00B1' },
    { std::wstring(L"&sup2;"), L'\u00B2' },
    { std::wstring(L"&sup3;"), L'\u00B3' },
    { std::wstring(L"&acute;"), L'\u00B4' },
    { std::wstring(L"&micro;"), L'\u00B5' },
    { std::wstring(L"&para;"), L'\u00B6' },
    { std::wstring(L"&middot;"), L'\u00B7' },
    { std::wstring(L"&cedil;"), L'\u00B8' },
    { std::wstring(L"&sup1;"), L'\u00B9' },
    { std::wstring(L"&ordm;"), L'\u00BA' },
    { std::wstring(L"&raquo;"), L'\u00BB' },
    { std::wstring(L"&frac14;"), L'\u00BC' },
    { std::wstring(L"&frac12;"), L'\u00BD' },
    { std::wstring(L"&frac34;"), L'\u00BE' },
    { std::wstring(L"&iquest;"), L'\u00BF' },
    { std::wstring(L"&Agrave;"), L'\u00C0' },
    { std::wstring(L"&Aacute;"), L'\u00C1' },
    { std::wstring(L"&Acirc;"), L'\u00C2' },
    { std::wstring(L"&Atilde;"), L'\u00C3' },
    { std::wstring(L"&Auml;"), L'\u00C4' },
    { std::wstring(L"&Aring;"), L'\u00C5' },
    { std::wstring(L"&AElig;"), L'\u00C6' },
    { std::wstring(L"&Ccedil;"), L'\u00C7' },
    { std::wstring(L"&Egrave;"), L'\u00C8' },
    { std::wstring(L"&Eacute;"), L'\u00C9' },
    { std::wstring(L"&Ecirc;"), L'\u00CA' },
    { std::wstring(L"&Euml;"), L'\u00CB' },
    { std::wstring(L"&Igrave;"), L'\u00CC' },
    { std::wstring(L"&Iacute;"), L'\u00CD' },
    { std::wstring(L"&Icirc;"), L'\u00CE' },
    { std::wstring(L"&Iuml;"), L'\u00CF' },
    { std::wstring(L"&ETH;"), L'\u00D0' },
    { std::wstring(L"&Ntilde;"), L'\u00D1' },
    { std::wstring(L"&Ograve;"), L'\u00D2' },
    { std::wstring(L"&Oacute;"), L'\u00D3' },
    { std::wstring(L"&Ocirc;"), L'\u00D4' },
    { std::wstring(L"&Otilde;"), L'\u00D5' },
    { std::wstring(L"&Ouml;"), L'\u00D6' },
    { std::wstring(L"&times;"), L'\u00D7' },
    { std::wstring(L"&Oslash;"), L'\u00D8' },
    { std::wstring(L"&Ugrave;"), L'\u00D9' },
    { std::wstring(L"&Uacute;"), L'\u00DA' },
    { std::wstring(L"&Ucirc;"), L'\u00DB' },
    { std::wstring(L"&Uuml;"), L'\u00DC' },
    { std::wstring(L"&Yacute;"), L'\u00DD' },
    { std::wstring(L"&THORN;"), L'\u00DE' },
    { std::wstring(L"&szlig;"), L'\u00DF' },
    { std::wstring(L"&agrave;"), L'\u00E0' },
    { std::wstring(L"&aacute;"), L'\u00E1' },
    { std::wstring(L"&acirc;"), L'\u00E2' },
    { std::wstring(L"&atilde;"), L'\u00E3' },
    { std::wstring(L"&auml;"), L'\u00E4' },
    { std::wstring(L"&aring;"), L'\u00E5' },
    { std::wstring(L"&aelig;"), L'\u00E6' },
    { std::wstring(L"&ccedil;"), L'\u00E7' },
    { std::wstring(L"&egrave;"), L'\u00E8' },
    { std::wstring(L"&eacute;"), L'\u00E9' },
    { std::wstring(L"&ecirc;"), L'\u00EA' },
    { std::wstring(L"&euml;"), L'\u00EB' },
    { std::wstring(L"&igrave;"), L'\u00EC' },
    { std::wstring(L"&iacute;"), L'\u00ED' },
    { std::wstring(L"&icirc;"), L'\u00EE' },
    { std::wstring(L"&iuml;"), L'\u00EF' },
    { std::wstring(L"&eth;"), L'\u00F0' },
    { std::wstring(L"&ntilde;"), L'\u00F1' },
    { std::wstring(L"&ograve;"), L'\u00F2' },
    { std::wstring(L"&oacute;"), L'\u00F3' },
    { std::wstring(L"&ocirc;"), L'\u00F4' },
    { std::wstring(L"&otilde;"), L'\u00F5' },
    { std::wstring(L"&ouml;"), L'\u00F6' },
    { std::wstring(L"&divide;"), L'\u00F7' },
    { std::wstring(L"&oslash;"), L'\u00F8' },
    { std::wstring(L"&ugrave;"), L'\u00F9' },
    { std::wstring(L"&uacute;"), L'\u00FA' },
    { std::wstring(L"&ucirc;"), L'\u00FB' },
    { std::wstring(L"&uuml;"), L'\u00FC' },
    { std::wstring(L"&yacute;"), L'\u00FD' },
    { std::wstring(L"&thorn;"), L'\u00FE' },
    { std::wstring(L"&yuml;"), L'\u00FF' },
    { std::wstring(L"&OElig;"), L'\u0152' },
    { std::wstring(L"&oelig;"), L'\u0153' },
    { std::wstring(L"&Scaron;"), L'\u0160' },
    { std::wstring(L"&scaron;"), L'\u0161' },
    { std::wstring(L"&Yuml;"), L'\u0178' },
    { std::wstring(L"&fnof;"), L'\u0192' },
    { std::wstring(L"&circ;"), L'\u02C6' },
    { std::wstring(L"&tilde;"), L'\u02DC' },
    { std::wstring(L"&Alpha;"), L'\u0391' },
    { std::wstring(L"&Beta;"), L'\u0392' },
    { std::wstring(L"&Gamma;"), L'\u0393' },
    { std::wstring(L"&Delta;"), L'\u0394' },
    { std::wstring(L"&Epsilon;"), L'\u0395' },
    { std::wstring(L"&Zeta;"), L'\u0396' },
    { std::wstring(L"&Eta;"), L'\u0397' },
    { std::wstring(L"&Theta;"), L'\u0398' },
    { std::wstring(L"&Iota;"), L'\u0399' },
    { std::wstring(L"&Kappa;"), L'\u039A' },
    { std::wstring(L"&Lambda;"), L'\u039B' },
    { std::wstring(L"&Mu;"), L'\u039C' },
    { std::wstring(L"&Nu;"), L'\u039D' },
    { std::wstring(L"&Xi;"), L'\u039E' },
    { std::wstring(L"&Omicron;"), L'\u039F' },
    { std::wstring(L"&Pi;"), L'\u03A0' },
    { std::wstring(L"&Rho;"), L'\u03A1' },
    { std::wstring(L"&Sigma;"), L'\u03A3' },
    { std::wstring(L"&Tau;"), L'\u03A4' },
    { std::wstring(L"&Upsilon;"), L'\u03A5' },
    { std::wstring(L"&Phi;"), L'\u03A6' },
    { std::wstring(L"&Chi;"), L'\u03A7' },
    { std::wstring(L"&Psi;"), L'\u03A8' },
    { std::wstring(L"&Omega;"), L'\u03A9' },
    { std::wstring(L"&alpha;"), L'\u03B1' },
    { std::wstring(L"&beta;"), L'\u03B2' },
    { std::wstring(L"&gamma;"), L'\u03B3' },
    { std::wstring(L"&delta;"), L'\u03B4' },
    { std::wstring(L"&epsilon;"), L'\u03B5' },
    { std::wstring(L"&zeta;"), L'\u03B6' },
    { std::wstring(L"&eta;"), L'\u03B7' },
    { std::wstring(L"&theta;"), L'\u03B8' },
    { std::wstring(L"&iota;"), L'\u03B9' },
    { std::wstring(L"&kappa;"), L'\u03BA' },
    { std::wstring(L"&lambda;"), L'\u03BB' },
    { std::wstring(L"&mu;"), L'\u03BC' },
    { std::wstring(L"&nu;"), L'\u03BD' },
    { std::wstring(L"&xi;"), L'\u03BE' },
    { std::wstring(L"&omicron;"), L'\u03BF' },
    { std::wstring(L"&pi;"), L'\u03C0' },
    { std::wstring(L"&rho;"), L'\u03C1' },
    { std::wstring(L"&sigmaf;"), L'\u03C2' },
    { std::wstring(L"&sigma;"), L'\u03C3' },
    { std::wstring(L"&tau;"), L'\u03C4' },
    { std::wstring(L"&upsilon;"), L'\u03C5' },
    { std::wstring(L"&phi;"), L'\u03C6' },
    { std::wstring(L"&chi;"), L'\u03C7' },
    { std::wstring(L"&psi;"), L'\u03C8' },
    { std::wstring(L"&omega;"), L'\u03C9' },
    { std::wstring(L"&thetasym;"), L'\u03D1' },
    { std::wstring(L"&upsih;"), L'\u03D2' },
    { std::wstring(L"&piv;"), L'\u03D6' },
    { std::wstring(L"&ensp;"), L'\u2002' },
    { std::wstring(L"&emsp;"), L'\u2003' },
    { std::wstring(L"&thinsp;"), L'\u2009' },
    { std::wstring(L"&zwnj;"), L'\u200C' },
    { std::wstring(L"&zwj;"), L'\u200D' },
    { std::wstring(L"&lrm;"), L'\u200E' },
    { std::wstring(L"&rlm;"), L'\u200F' },
    { std::wstring(L"&ndash;"), L'\u2013' },
    { std::wstring(L"&mdash;"), L'\u2014' },
    { std::wstring(L"&lsquo;"), L'\u2018' },
    { std::wstring(L"&rsquo;"), L'\u2019' },
    { std::wstring(L"&sbquo;"), L'\u201A' },
    { std::wstring(L"&ldquo;"), L'\u201C' },
    { std::wstring(L"&rdquo;"), L'\u201D' },
    { std::wstring(L"&bdquo;"), L'\u201E' },
    { std::wstring(L"&dagger;"), L'\u2020' },
    { std::wstring(L"&Dagger;"), L'\u2021' },
    { std::wstring(L"&bull;"), L'\u2022' },
    { std::wstring(L"&hellip;"), L'\u2026' },
    { std::wstring(L"&permil;"), L'\u2030' },
    { std::wstring(L"&prime;"), L'\u2032' },
    { std::wstring(L"&Prime;"), L'\u2033' },
    { std::wstring(L"&lsaquo;"), L'\u2039' },
    { std::wstring(L"&rsaquo;"), L'\u203A' },
    { std::wstring(L"&oline;"), L'\u203E' },
    { std::wstring(L"&frasl;"), L'\u2044' },
    { std::wstring(L"&euro;"), L'\u20AC' },
    { std::wstring(L"&image;"), L'\u2111' },
    { std::wstring(L"&weierp;"), L'\u2118' },
    { std::wstring(L"&real;"), L'\u211C' },
    { std::wstring(L"&trade;"), L'\u2122' },
    { std::wstring(L"&alefsym;"), L'\u2135' },
    { std::wstring(L"&larr;"), L'\u2190' },
    { std::wstring(L"&uarr;"), L'\u2191' },
    { std::wstring(L"&rarr;"), L'\u2192' },
    { std::wstring(L"&darr;"), L'\u2193' },
    { std::wstring(L"&harr;"), L'\u2194' },
    { std::wstring(L"&crarr;"), L'\u21B5' },
    { std::wstring(L"&lArr;"), L'\u21D0' },
    { std::wstring(L"&uArr;"), L'\u21D1' },
    { std::wstring(L"&rArr;"), L'\u21D2' },
    { std::wstring(L"&dArr;"), L'\u21D3' },
    { std::wstring(L"&hArr;"), L'\u21D4' },
    { std::wstring(L"&forall;"), L'\u2200' },
    { std::wstring(L"&part;"), L'\u2202' },
    { std::wstring(L"&exist;"), L'\u2203' },
    { std::wstring(L"&empty;"), L'\u2205' },
    { std::wstring(L"&nabla;"), L'\u2207' },
    { std::wstring(L"&isin;"), L'\u2208' },
    { std::wstring(L"&notin;"), L'\u2209' },
    { std::wstring(L"&ni;"), L'\u220B' },
    { std::wstring(L"&prod;"), L'\u220F' },
    { std::wstring(L"&sum;"), L'\u2211' },
    { std::wstring(L"&minus;"), L'\u2212' },
    { std::wstring(L"&lowast;"), L'\u2217' },
    { std::wstring(L"&radic;"), L'\u221A' },
    { std::wstring(L"&prop;"), L'\u221D' },
    { std::wstring(L"&infin;"), L'\u221E' },
    { std::wstring(L"&ang;"), L'\u2220' },
    { std::wstring(L"&and;"), L'\u2227' },
    { std::wstring(L"&or;"), L'\u2228' },
    { std::wstring(L"&cap;"), L'\u2229' },
    { std::wstring(L"&cup;"), L'\u222A' },
    { std::wstring(L"&int;"), L'\u222B' },
    { std::wstring(L"&there4;"), L'\u2234' },
    { std::wstring(L"&sim;"), L'\u223C' },
    { std::wstring(L"&cong;"), L'\u2245' },
    { std::wstring(L"&asymp;"), L'\u2248' },
    { std::wstring(L"&ne;"), L'\u2260' },
    { std::wstring(L"&equiv;"), L'\u2261' },
    { std::wstring(L"&le;"), L'\u2264' },
    { std::wstring(L"&ge;"), L'\u2265' },
    { std::wstring(L"&sub;"), L'\u2282' },
    { std::wstring(L"&sup;"), L'\u2283' },
    { std::wstring(L"&nsub;"), L'\u2284' },
    { std::wstring(L"&sube;"), L'\u2286' },
    { std::wstring(L"&supe;"), L'\u2287' },
    { std::wstring(L"&oplus;"), L'\u2295' },
    { std::wstring(L"&otimes;"), L'\u2297' },
    { std::wstring(L"&perp;"), L'\u22A5' },
    { std::wstring(L"&sdot;"), L'\u22C5' },
    { std::wstring(L"&lceil;"), L'\u2308' },
    { std::wstring(L"&rceil;"), L'\u2309' },
    { std::wstring(L"&lfloor;"), L'\u230A' },
    { std::wstring(L"&rfloor;"), L'\u230B' },
    { std::wstring(L"&lang;"), L'\u2329' },
    { std::wstring(L"&rang;"), L'\u232A' },
    { std::wstring(L"&loz;"), L'\u25CA' },
    { std::wstring(L"&spades;"), L'\u2660' },
    { std::wstring(L"&clubs;"), L'\u2663' },
    { std::wstring(L"&hearts;"), L'\u2665' },
    { std::wstring(L"&diams;"), L'\u2666' }
};

inline gunichar
get_entity(gunichar *ptr, size_t len) {
    // try hex, decimal entity first
    gunichar ech(0);
    if (ptr[1] == gunichar(L'#') && len > 3) {
        std::wstringstream wss;
        int wch = 0;
        try {
            wss << std::hex << std::wstring((wchar_t *)(ptr+2),len-3);
            wss >> wch;
            ech = gunichar(wch);
        } catch (...) {
            ech = 0;
        }
    } else if (g_unichar_type(ptr[1]) == G_UNICODE_DECIMAL_NUMBER) {
        std::wstringstream wss;
        int wch = 0;
        try {
            wss << std::dec << std::wstring((wchar_t *)(ptr+1),len-2);
            wss >> wch;
            ech = gunichar(wch);
        } catch (...) {
            ech = 0;
        }
    }
    if (ech)
        return ech;

    std::map<std::wstring,gunichar>::const_iterator it =
        ENTITY_MAP.find(std::wstring((wchar_t *)(ptr),len));
    return it != ENTITY_MAP.end() ? it->second : gunichar(0);
}


inline gunichar
get_entity(char *ptr, size_t len) {
    glong ulen = 0;
    gunichar *gtmp = g_utf8_to_ucs4_fast((const gchar *)ptr, len, &ulen);
    gunichar gch = get_entity(gtmp,ulen);
    g_free(gtmp);
    return gch;
}


inline std::string
trim(const std::string& in)
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


inline std::vector<std::string>
split(const std::string& in)
{
    std::vector<std::string> outv;
    std::istringstream iss(in);
    std::copy(std::istream_iterator<std::string>(iss),
              std::istream_iterator<std::string>(),
              std::back_inserter(outv));
    return outv;
}

}; // end anonymous namespace


#ifdef TOKENIZER_NAMESPACE
namespace TOKENIZER_NAMESPACE {
#endif


void
Tokenizer::set_config_dir(const std::string& dir) {
    if (dir.empty()) {
        cfg_dir = ".";
    } else {
        cfg_dir.assign(dir);
    }
}


Tokenizer::Tokenizer(const Parameters& _)
    : nthreads(_.nthreads ? _.nthreads : 1)
    , chunksize(_.chunksize)
    , lang_iso(_.lang_iso)
    , english_p(_.lang_iso.compare("en")==0)
    , latin_p((!english_p) && (_.lang_iso.compare("fr")==0 || _.lang_iso.compare("it")==0))
    , skip_xml_p(_.detag_p)
    , skip_alltags_p(_.alltag_p)
    , entities_p(_.entities_p)
    , escape_p(_.escape_p)
    , unescape_p(_.unescape_p)
    , aggressive_hyphen_p(_.aggro_p)
    , supersub_p(_.supersub_p)
    , url_p(_.url_p)
    , downcase_p(_.downcase_p)
    , normalize_p(_.normalize_p)
    , penn_p(_.penn_p)
    , narrow_latin_p(_.narrow_latin_p)
    , narrow_kana_p(_.narrow_kana_p)
    , refined_p(_.refined_p)
    , drop_bad_p(_.drop_bad_p)
    , splits_p(_.split_p)
    , verbose_p(_.verbose_p)
    , para_marks_p(_.para_marks_p)
    , split_breaks_p(_.split_breaks_p)
{
    if (_.cfg_path)
        set_config_dir(_.cfg_path);
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
Tokenizer::init(const char *cfg_dir_optional) {
    if (cfg_dir_optional)
        set_config_dir(std::string(cfg_dir_optional));

    std::string dir_path(cfg_dir);
    dir_path.append("/nonbreaking_prefixes");
    if (::access(dir_path.c_str(),X_OK)) {
        dir_path = cfg_dir;
    }

    std::string nbpre_path(dir_path);
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


void
Tokenizer::reset() {
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
Tokenizer::unescape(std::string& word) {
    std::ostringstream oss;
    std::size_t was = 0; // last processed
    std::size_t pos = 0; // last unprocessed
    std::size_t len = 0; // processed length
    bool hit = false;
    for (std::size_t endp=0;
         (pos = word.find('&',was)) != std::string::npos && (endp = word.find(';',pos)) != std::string::npos;
         was = endp == std::string::npos ? pos : 1+endp) {
        len = endp - pos + 1;
        glong ulen(0);
        gunichar *gtmp = g_utf8_to_ucs4_fast((const gchar *)word.c_str()+pos, len, &ulen);
        gunichar gbuf[2] = { 0 };
        if ((gbuf[0] = get_entity(gtmp,ulen)) != gunichar(0)) {
            gchar *gstr = g_ucs4_to_utf8(gbuf,ulen,0,0,0);
            if (escape_p && ESCAPE_SET.find(std::string(gstr)) != ESCAPE_SET.end()) {
                // do not unescape moses escapes when escape flag is turned on
                oss << word.substr(was,1+endp-was);
            } else {
                if (was < pos)
                    oss << word.substr(was,pos-was);
                oss << gstr;
                was += ulen;
                hit = true;
            }
            g_free(gstr);
        } else {
            oss << word.substr(was,1+endp-was);
        }
        g_free(gtmp);
    }
    if (was < word.size())
        oss << word.substr(was);
    if (hit)
        word = oss.str();
    return hit;
}


bool
Tokenizer::escape(std::string& text) {
    bool mod_p = false;
    std::string outs;

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
                // check for a pre-existing escape
                const char *sc = strchr(pt,';');
                if (!sc || sc-pt < 2 || sc-pt > 9) {
                    sequence_p = ESCAPE_MOSES[3];
                }
            } else if (*pt == '\'') {
                sequence_p = ESCAPE_MOSES[6];
            } else if (*pt == '"') {
                sequence_p = ESCAPE_MOSES[7];
            }
        } else if (*pt > ']') {
            if (*pt =='|') { // 7c
                sequence_p = ESCAPE_MOSES[0];
            }
        } else if (*pt > 'Z') {
            if (*pt == '<') { // 3e
                sequence_p = ESCAPE_MOSES[4];
            } else if (*pt == '>') { // 3c
                sequence_p = ESCAPE_MOSES[5];
            } else if (*pt == '[') { // 5b
                sequence_p = ESCAPE_MOSES[1];
            } else if (*pt == ']') { // 5d
                sequence_p = ESCAPE_MOSES[2];
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
Tokenizer::penn_tokenize(const std::string& buf)
{
    static const char *comma_refs = "\\1 , \\2";
    static const char *isolate_ref = " \\1 ";
    static const char *special_refs = "\\1 @\\2@ \\3";

    std::string text(buf);
    std::string outs;
    if (skip_alltags_p)
        RE2::GlobalReplace(&text,genl_tags_x,SPC_BYTE);

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
    if (escape_p)
        escape(ntext);

    // strip out wrapping spaces from line in result string
    outs.assign(ntext.substr(1,ntext.size()-2));
    return outs;
}


std::string
Tokenizer::quik_tokenize(const std::string& buf)
{
    std::string text(buf);
    size_t pos;
    int num = 0;

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

    gunichar prev_uch(0);
    gunichar next_uch(*ucs4);
    gunichar curr_uch(0);

    GUnicodeType curr_type(G_UNICODE_UNASSIGNED);
    GUnicodeType next_type((ucs4 && *ucs4) ? g_unichar_type(*ucs4) : G_UNICODE_UNASSIGNED);
    GUnicodeType prev_type(G_UNICODE_UNASSIGNED);

    bool post_break_p = false;
    bool in_num_p = next_uch <= gunichar(L'9') && next_uch >= gunichar(L'0');
    bool in_url_p = false;
    int since_start = 0;
    int alpha_prefix = 0;
    int bad_length = 0;

    while (ucs4 < lim4) {
        prev_uch = curr_uch;
        prev_type = curr_type;
        curr_uch = next_uch;
        curr_type = next_type;

        if (++nxt4 >= lim4) {
            next_uch = 0;
            next_type = G_UNICODE_UNASSIGNED;
        } else {
            next_uch = *nxt4;
            next_type = g_unichar_type(next_uch);
        }

        if (url_p) {
            if (!in_url_p && *ucs4 < 0x80L) { // url chars must be in the basic plane
                if (!since_start) {
                    if (std::isalpha(char(*ucs4)))
                        alpha_prefix++;
                } else if (alpha_prefix == since_start
                           && char(*ucs4) == ':'
                           && next_type != G_UNICODE_SPACE_SEPARATOR) {
                    in_url_p = true;
                }
            }
        }

        bool pre_break_p = false;
        const wchar_t *substitute_p = 0;

        if (post_break_p) {
            *uptr++ = gunichar(L' ');
            since_start = bad_length = 0;
            in_url_p = in_num_p = post_break_p = false;
        }

    retry:

        switch (curr_type) {
        case G_UNICODE_MODIFIER_LETTER:
        case G_UNICODE_OTHER_LETTER:
        case G_UNICODE_TITLECASE_LETTER:
            if (in_url_p || in_num_p)
                pre_break_p = true;
            // fallthough
        case G_UNICODE_UPPERCASE_LETTER:
        case G_UNICODE_LOWERCASE_LETTER:
            if (downcase_p && curr_type == G_UNICODE_UPPERCASE_LETTER)
                curr_uch = g_unichar_tolower(*ucs4);
            break;
        case G_UNICODE_SPACING_MARK:
            pre_break_p = true;
            in_num_p = false;
            curr_uch = 0;
            break;
        case G_UNICODE_DECIMAL_NUMBER:
        case G_UNICODE_LETTER_NUMBER:
        case G_UNICODE_OTHER_NUMBER:
            if (!in_num_p && !in_url_p) {
                switch (prev_type) {
                case G_UNICODE_DASH_PUNCTUATION:
                case G_UNICODE_FORMAT:
                case G_UNICODE_OTHER_PUNCTUATION:
                case G_UNICODE_UPPERCASE_LETTER:
                case G_UNICODE_LOWERCASE_LETTER:
                case G_UNICODE_DECIMAL_NUMBER:
                    break;
                default:
                    pre_break_p = true;
                }
            }
            in_num_p = true;
            break;
        case G_UNICODE_CONNECT_PUNCTUATION:
            if (curr_uch != gunichar(L'_')) {
                if (in_url_p) {
                    in_url_p = false;
                    post_break_p = pre_break_p = true;
                }
            }
            if (in_num_p) {
                post_break_p = pre_break_p = true;
            } else {
                switch (next_type) {
                case G_UNICODE_LOWERCASE_LETTER:
                case G_UNICODE_MODIFIER_LETTER:
                case G_UNICODE_OTHER_LETTER:
                case G_UNICODE_TITLECASE_LETTER:
                    break;
                default:
                    post_break_p = pre_break_p = true;
                }
                switch (prev_type) {
                case G_UNICODE_LOWERCASE_LETTER:
                case G_UNICODE_MODIFIER_LETTER:
                case G_UNICODE_OTHER_LETTER:
                case G_UNICODE_TITLECASE_LETTER:
                    break;
                default:
                    post_break_p = pre_break_p = true;
                }
            }
            break;
        case G_UNICODE_FORMAT:
            in_url_p = in_num_p = false;
            break;
        case G_UNICODE_DASH_PUNCTUATION:
            if (aggressive_hyphen_p && !in_url_p && curr_uch != next_uch && prev_uch != curr_uch && (!(prev_uch == L' ' || !prev_uch) && !(next_uch == L' ' || !next_uch))) {
                substitute_p = L"@-@";
                post_break_p = pre_break_p = true;
            } else if ( ( curr_uch > gunichar(L'\u002D') && curr_uch < gunichar(L'\u2010') ) ||
                        ( curr_uch > gunichar(L'\u2011')
                          && curr_uch != gunichar(L'\u30A0')
                          && curr_uch < gunichar(L'\uFE63') ) ) {
                // dash, not a hyphen
                post_break_p = pre_break_p = true;
            } else if (next_type == G_UNICODE_SPACE_SEPARATOR) {
            } else {
                if (prev_type == curr_type) {
                    if (next_type != curr_type) {
                        post_break_p = !in_url_p;
                    }
                } else if (next_type == curr_type) {
                    pre_break_p = !in_url_p;
                } else if ((prev_type == G_UNICODE_UPPERCASE_LETTER ||
                            prev_type == G_UNICODE_LOWERCASE_LETTER) &&
                           next_type == G_UNICODE_DECIMAL_NUMBER) {
                    in_num_p = false;
                } else if (in_num_p || since_start == 0) {
                    switch (next_type) {
                    case G_UNICODE_UPPERCASE_LETTER:
                    case G_UNICODE_LOWERCASE_LETTER:
                    case G_UNICODE_MODIFIER_LETTER:
                    case G_UNICODE_OTHER_LETTER:
                    case G_UNICODE_TITLECASE_LETTER:
                    case G_UNICODE_SPACE_SEPARATOR:
                        in_num_p = false;
                        break;
                    case G_UNICODE_DECIMAL_NUMBER:
                    case G_UNICODE_LETTER_NUMBER:
                    case G_UNICODE_OTHER_NUMBER:
                    case G_UNICODE_OTHER_PUNCTUATION:
                        break;
                    default:
                        post_break_p = true;
                        pre_break_p = prev_uch != curr_uch;
                    }
                } else if (in_url_p) {
                    pre_break_p = curr_uch != gunichar(L'-');
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
                        case G_UNICODE_OTHER_PUNCTUATION:
                            if (prev_type != next_type)
                                break;
                        default:
                            post_break_p = pre_break_p = prev_uch != curr_uch;
                        }
                        break;
                    default:
                        post_break_p = pre_break_p = prev_uch != curr_uch;
                        break;
                    }
                }
            }
            break;
        case G_UNICODE_OTHER_PUNCTUATION:
            switch (curr_uch) {
            case gunichar(L':'):
            case gunichar(L'/'):
                if (refined_p && !in_url_p
                    && prev_type == G_UNICODE_DECIMAL_NUMBER
                    && next_type == G_UNICODE_DECIMAL_NUMBER) {
                    break;
                }
            // fall-through
            case gunichar(L'!'):
            case gunichar(L'#'):
            case gunichar(L';'):
            case gunichar(L'?'):
            case gunichar(L'@'):
                post_break_p = pre_break_p = !in_url_p || next_type != G_UNICODE_SPACE_SEPARATOR;
            break;
            case gunichar(L'+'):
                post_break_p = pre_break_p = !in_num_p && since_start > 0;
                in_num_p = in_num_p || since_start == 0;
                break;
            case gunichar(L'&'):
                if (unescape_p) {
                    if (next_type == G_UNICODE_LOWERCASE_LETTER || next_type == G_UNICODE_UPPERCASE_LETTER
                        || next_type == G_UNICODE_DECIMAL_NUMBER || next_uch == gunichar(L'#')) {
                        gunichar *eptr = nxt4;
                        GUnicodeType eptr_type(G_UNICODE_UNASSIGNED);
                        for (++eptr; eptr < lim4 && *eptr != gunichar(L';'); ++eptr) {
                            eptr_type = g_unichar_type(*eptr);
                            if (eptr_type != G_UNICODE_LOWERCASE_LETTER
                                && eptr_type != G_UNICODE_UPPERCASE_LETTER
                                && eptr_type != G_UNICODE_DECIMAL_NUMBER)
                                break;
                        }
                        gunichar ech(0);
                        if (*eptr == gunichar(L';') && (ech = get_entity(ucs4,eptr-ucs4+1))) {
                            curr_uch = ech;
                            curr_type = g_unichar_type(ech);
                            ucs4 = eptr;
                            nxt4 = ++eptr;
                            next_uch = *nxt4;
                            next_type = nxt4 < lim4 ? g_unichar_type(next_uch) : G_UNICODE_UNASSIGNED;
                            goto retry;
                        }
                    }
                }
                if (entities_p && !in_url_p) {
                    gunichar *cur4 = nxt4;
                    if (*cur4 == gunichar('#')) ++cur4;
                    while (g_unichar_isalnum(*cur4)) ++cur4;
                    if (cur4 > nxt4 && *cur4 == gunichar(';')) {
                        if (since_start) {
                            *uptr++ = gunichar(L' ');
                            since_start = 0;
                        }
                        ++cur4;
                        memcpy(uptr,ucs4,cur4-ucs4);
                        uptr += cur4-ucs4;
                        ucs4 = cur4;
                        *uptr++ = gunichar(L' ');
                        pre_break_p = post_break_p = false;
                        curr_uch = *ucs4;
                        curr_type = ucs4 < lim4 ? g_unichar_type(curr_uch) : G_UNICODE_UNASSIGNED;
                        nxt4 = ++cur4;
                        next_uch = *nxt4;
                        next_type = nxt4 < lim4 ? g_unichar_type(next_uch) : G_UNICODE_UNASSIGNED;
                        goto retry;
                    }

                }
                post_break_p = pre_break_p = !in_url_p || next_type != G_UNICODE_SPACE_SEPARATOR;
                if (escape_p)
                    substitute_p = L"&amp;";
                break;
            case gunichar(L'\''):
                if (english_p) {
                    if (!in_url_p) {
                        bool next_letter_p = next_type == G_UNICODE_LOWERCASE_LETTER
                            || next_type == G_UNICODE_UPPERCASE_LETTER;
                        pre_break_p = true;
                        if (next_letter_p && refined_p) {
                            // break sha n't instead of shan 't:
                            if (prev_uch == gunichar(L'n') || prev_uch == gunichar(L'N')) {
                                *(uptr - 1) = gunichar(L' ');
                                *(uptr++) = prev_uch;
                                pre_break_p = false;
                            }
                        }
                        post_break_p = since_start == 0
                            || (!next_letter_p && next_type != G_UNICODE_DECIMAL_NUMBER);
                    }
                } else if (latin_p) {
                    post_break_p = !in_url_p;
                    pre_break_p = !in_url_p && prev_type != G_UNICODE_LOWERCASE_LETTER && prev_type != G_UNICODE_UPPERCASE_LETTER;
                } else {
                    post_break_p = pre_break_p = !in_url_p;
                }
                if (escape_p)
                    substitute_p = L"&apos;";
                break;
            case gunichar(L'"'):
                post_break_p = pre_break_p = true;
                if (escape_p)
                    substitute_p = L"&quot;";
                break;
            case gunichar(L','):
                pre_break_p = !in_num_p || next_type != G_UNICODE_DECIMAL_NUMBER;
                post_break_p = !in_num_p && next_type != G_UNICODE_DECIMAL_NUMBER;
                break;
            case gunichar(L'%'):
                if (refined_p) {
                    pre_break_p = !in_num_p;
                    post_break_p = !in_num_p && next_type != G_UNICODE_DECIMAL_NUMBER;
                } else {
                    post_break_p = pre_break_p = true;
                }
                break;
            case gunichar(L'.'):
                if (prev_uch != '.') {
                    if (!in_num_p) {
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
                                                pre_break_p = true;
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
                                        pre_break_p = true;
                                    }
                                    break;
                                }
                                default:
                                    pre_break_p = true;
                                    break;
                                }
                            }
                            break;
                        }
                    } else {
                        switch (next_type) {
                        case G_UNICODE_DECIMAL_NUMBER:
                        case G_UNICODE_LOWERCASE_LETTER:
                        case G_UNICODE_UPPERCASE_LETTER:
                            break;
                        default:
                            pre_break_p = true;
                        }
                    }
                } else if (next_uch != '.') {
                    post_break_p = true;
                }
                break;
            default:
                post_break_p = pre_break_p = true;
                break;
            }
            break;
        case G_UNICODE_CLOSE_PUNCTUATION:
        case G_UNICODE_FINAL_PUNCTUATION:
        case G_UNICODE_INITIAL_PUNCTUATION:
        case G_UNICODE_OPEN_PUNCTUATION:
            switch (curr_uch) {
            case gunichar(L'('):
            case gunichar(L')'):
                break;
            case gunichar(L'['):
                if (escape_p)
                    substitute_p = L"&#91;";
                break;
            case gunichar(L']'):
                if (escape_p)
                    substitute_p = L"&#93;";
                break;
            default:
                in_url_p = false;
            }
            post_break_p = pre_break_p = !in_url_p;
            break;
        case G_UNICODE_CURRENCY_SYMBOL:
            if (refined_p) {
                post_break_p = in_num_p; // was in number, so break it
                pre_break_p = !in_num_p;
                in_num_p = in_num_p || next_type == G_UNICODE_DECIMAL_NUMBER || next_uch == gunichar(L'.') || next_uch == gunichar(L',');
            } else {
                post_break_p = pre_break_p = true;
                in_num_p = false;
            }
            if (curr_uch != gunichar(L'$'))
                in_url_p = false;
            break;
        case G_UNICODE_MODIFIER_SYMBOL:
        case G_UNICODE_MATH_SYMBOL:
            switch (curr_uch) {
            case gunichar(L'`'):
                if (english_p) {
                    if (!in_url_p) {
                        pre_break_p = true;
                        post_break_p = since_start == 0 ||
                            (next_type != G_UNICODE_LOWERCASE_LETTER && next_type != G_UNICODE_UPPERCASE_LETTER && next_type != G_UNICODE_DECIMAL_NUMBER);
                    }
                } else if (latin_p) {
                    post_break_p = !in_url_p;
                    pre_break_p = !in_url_p && prev_type != G_UNICODE_LOWERCASE_LETTER && prev_type != G_UNICODE_UPPERCASE_LETTER;
                } else {
                    post_break_p = pre_break_p = !in_url_p;
                }
                if (escape_p)
                    substitute_p = L"&apos;";
                else
                    curr_uch = gunichar(L'\'');
                break;
            case gunichar(L'|'):
                if (escape_p)
                    substitute_p = L"&#124;";
                post_break_p = pre_break_p = true;
                break;
            case gunichar(L'<'):
                if (escape_p)
                    substitute_p = L"&lt;";
                post_break_p = pre_break_p = true;
                break;
            case gunichar(L'>'):
                if (escape_p)
                    substitute_p = L"&gt;";
                post_break_p = pre_break_p = true;
                break;
            case gunichar(L'%'):
                post_break_p = in_num_p;
                pre_break_p = !in_num_p && !in_url_p;
                in_num_p = false;
                break;
            case gunichar(L'='):
            case gunichar(L'~'):
                in_num_p = false;
            post_break_p = pre_break_p = !in_url_p;
            break;
            case gunichar(L'+'):
                post_break_p = pre_break_p = !in_url_p;
                if (in_url_p) {
                    in_num_p = false;
                } else if (refined_p) {
                    // handle floating point as e.g. 1.2e+3.4
                    bool next_digit_p = next_type == G_UNICODE_DECIMAL_NUMBER ||
                        next_uch == gunichar(L'.');
                    pre_break_p = !in_num_p;
                    in_num_p = next_digit_p && prev_type != G_UNICODE_DECIMAL_NUMBER;
                    post_break_p = !in_num_p;
                } else {
                    in_num_p = in_num_p || since_start == 0;
                }
                break;
            default:
                post_break_p = pre_break_p = true;
                break;
            }
            break;
        case G_UNICODE_OTHER_SYMBOL:
            post_break_p = pre_break_p = true;
            break;
        case G_UNICODE_CONTROL:
            if (drop_bad_p) {
                curr_uch = gunichar(L' ');
            } else if (curr_uch < gunichar(L' ')) {
                curr_uch = gunichar(L' ');
            } else if (curr_uch == gunichar(L'\u0092') &&
                       (next_type == G_UNICODE_LOWERCASE_LETTER || next_type == G_UNICODE_UPPERCASE_LETTER)) {
                // observed corpus corruption case
                if (english_p) {
                    pre_break_p = true;
                    post_break_p = since_start == 0 ||
                        (next_type != G_UNICODE_LOWERCASE_LETTER && next_type != G_UNICODE_UPPERCASE_LETTER && next_type != G_UNICODE_DECIMAL_NUMBER);
                } else if (latin_p) {
                    post_break_p = true;
                    pre_break_p = prev_type != G_UNICODE_LOWERCASE_LETTER && prev_type != G_UNICODE_UPPERCASE_LETTER;
                } else {
                    post_break_p = pre_break_p = true;
                }
                if (escape_p)
                    substitute_p = L"&apos;";
                else
                    curr_uch = gunichar(L'\'');
            } else {
                post_break_p = pre_break_p = true;
            }
            in_url_p = in_num_p = false;
            break;
        case G_UNICODE_LINE_SEPARATOR:
        case G_UNICODE_SPACE_SEPARATOR:
            curr_uch = gunichar(L' ');
            in_url_p = in_num_p = false;
            break;
        case G_UNICODE_ENCLOSING_MARK:
            in_url_p = false;
            break;
        case G_UNICODE_NON_SPACING_MARK:
        case G_UNICODE_PRIVATE_USE:
        case G_UNICODE_SURROGATE:
            in_url_p = in_num_p = false;
            break;
        case G_UNICODE_UNASSIGNED:
        default:
            // malformed bytes are dropped (invalid utf8 unicode)
            if (drop_bad_p) {
                curr_uch = 0;
            } else {
                pre_break_p = since_start > 0 && bad_length == 0;
                curr_type = G_UNICODE_UNASSIGNED;
            }
            in_url_p = in_num_p = false;
            break;
        }

        if (pre_break_p || curr_uch == gunichar(L' ') || (bad_length && curr_type != G_UNICODE_UNASSIGNED)) {
            if (since_start) {
                // non-empty token emitted previously, so pre-break must emit token separator
                *uptr++ = gunichar(L' ');
                since_start = bad_length = 0;
            }
            if (curr_uch == gunichar(L' '))
                // suppress emission below, fall-through to substitute logic
                curr_uch = 0;
        }

        if (substitute_p) {
            for (gunichar *sptr = (gunichar *)substitute_p; *sptr; ++sptr) {
                *uptr++ = *sptr;
                since_start++;
            }
            in_url_p = in_num_p = false;
        } else if (curr_uch) {
            *uptr++ = curr_uch;
            since_start++;
            if (curr_type == G_UNICODE_UNASSIGNED)
                bad_length++;
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

    // escape moses meta-characters
    if (escape_p)
        escape(text);

    return text;
}


std::size_t
Tokenizer::tokenize(std::istream& is, std::ostream& os)
{
    std::size_t line_no = 0;
    std::size_t perchunk = chunksize ? chunksize : 2000;
    std::vector< std::vector< std::string > > lines(nthreads);
    std::vector< std::vector< std::string > > results(nthreads);
    std::vector< boost::thread > workers(nthreads);
    bool done_p = !(is.good() && os.good());


    for (std::size_t tranche = 0; !done_p; ++tranche) {

        // for loop starting threads for chunks of input
        for (std::size_t ithread = 0; ithread < nthreads; ++ithread) {

            lines[ithread].resize(perchunk);
            std::size_t line_pos = 0;

            for ( ; line_pos < perchunk; ++line_pos) {

                std::string istr;
                std::getline(is,istr);

                if (skip_alltags_p) {
                    RE2::GlobalReplace(&istr,genl_tags_x,SPC_BYTE);
                    istr = trim(istr);
                }
                line_no++;

                if (istr.empty()) {
                    if (is.eof()) {
                        done_p = true;
                        lines[ithread].resize(line_pos);
                        results[ithread].resize(line_pos);
                        break;
                    }
                    lines[ithread][line_pos].clear();
                } else if (skip_xml_p &&
                           (RE2::FullMatch(istr,tag_line_x) || RE2::FullMatch(istr,white_line_x))) {
                    lines[ithread][line_pos].clear();
                } else {
                    lines[ithread][line_pos] =
                        std::string(SPC_BYTE).append(istr).append(SPC_BYTE);
                }
            }

            if (line_pos) {
                workers[ithread] =
                    boost::thread(VectorTokenizerCallable(this,lines[ithread],results[ithread]));
            }
        } // end for loop starting threads

        for (std::size_t ithread = 0; ithread < nthreads; ++ithread) {
            if (!workers[ithread].joinable())
                continue;

            workers[ithread].join();

            std::size_t nres = results[ithread].size();
            std::size_t nlin = lines[ithread].size();

            if (nlin != nres) {
                std::ostringstream emsg;
                emsg << "Tranche " << tranche
                     << " worker " << ithread << "/" << nthreads
                     << " |lines|==" << nlin << " != |results|==" << nres;
                throw std::runtime_error(emsg.str());
            }

            for (std::size_t ires = 0; ires < nres; ++ires)
                os << results[ithread][ires] << std::endl;

        } // end loop over joined results

        if (verbose_p) {
            std::cerr << line_no << ' ';
            std::cerr.flush();
        }

    } // end loop over chunks

    return line_no;
}


std::string
Tokenizer::detokenize(const std::string& buf)
{
    std::vector<std::string> words = split(trim(buf));

    std::size_t squotes = 0;
    std::size_t dquotes = 0;
    std::string prepends("");

    std::ostringstream oss;

    std::size_t nwords = words.size();
    std::size_t iword = 0;

    if (unescape_p)
        for (auto &word: words)
            unescape(word);

    for (auto &word: words) {
        if (RE2::FullMatch(word,right_x)) {
            if (iword)
                oss << SPC_BYTE;
            oss << word;
            prepends.clear();
        } else if (RE2::FullMatch(word,left_x)) {
            oss << word;
            prepends = SPC_BYTE;
        } else if (english_p && iword
                   && RE2::FullMatch(word,curr_en_x)
                   && RE2::FullMatch(words[iword-1],pre_en_x)) {
            oss << word;
            prepends = SPC_BYTE;
        } else if (latin_p && iword < nwords - 2
                   && RE2::FullMatch(word,curr_fr_x)
                   && RE2::FullMatch(words[iword+1],post_fr_x)) {
            oss << prepends << word;
            prepends.clear();
        } else if (word.size() == 1) {
            if ((word.at(0) == '\'' && ((squotes % 2) == 0 )) ||
                (word.at(0) == '"' && ((dquotes % 2) == 0))) {
                if (english_p && iword
                    && word.at(0) == '\''
                    && std::tolower(words[iword-1].at(words[iword-1].size()-1)) == 's') {
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
                if (std::isalnum(word.at(0)))
                    oss << prepends;
                oss << word;
                prepends = SPC_BYTE;
                if (word.at(0) == '\'')
                    squotes++;
                else if (word.at(0) == '"')
                    dquotes++;
			}
		} else {
            oss << prepends << word;
            prepends = SPC_BYTE;
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


std::vector<std::string>
Tokenizer::splitter(const std::string &istr, bool *continuation_ptr) {
    std::vector<std::string> parts;
    glong ncp = 0;
    glong ocp = 0;
    glong icp = 0;
    gunichar *ucs4 = g_utf8_to_ucs4_fast((gchar *)istr.c_str(),istr.size(),&ncp);
    if (ncp == 0) {
        g_free(ucs4);
        return parts;
    }
    gunichar *uout = (gunichar *)g_malloc0(2*ncp*sizeof(gunichar));

    const wchar_t GENL_HYPH = L'\u2010';
    const wchar_t IDEO_STOP = L'\u3002';
    const wchar_t KANA_MDOT = L'\u30FB';
    const wchar_t WAVE_DASH = L'\u301C';
    //const wchar_t WAVY_DASH = L'\u3030';
    const wchar_t KANA_DHYP = L'\u30A0';
    const wchar_t SMAL_HYPH = L'\uFE63';
    const wchar_t WIDE_EXCL = L'\uFF01';
    const wchar_t WIDE_PCTS = L'\uFF05';
    //const wchar_t WIDE_HYPH = L'\uFF0D';
    const wchar_t WIDE_STOP = L'\uFF0E';
    const wchar_t WIDE_QUES = L'\uFF1F';
    const wchar_t INVERT_QM = L'\u00BF';
    const wchar_t INVERT_EX = L'\u00A1';

    wchar_t currwc = 0;

    std::size_t init_word = 0;
    std::size_t fini_word = 0;
    std::size_t finilen = 0;
    std::size_t dotslen = 0;

	  const std::size_t SEQ_LIM = 6;

    charclass_t prev_class = empty;
    charclass_t curr_class = empty;
    std::vector<charclass_t> seq(SEQ_LIM, empty);
    std::vector<std::size_t> pos(SEQ_LIM, 0);
    std::size_t seqpos = 0;

    GUnicodeType curr_type = G_UNICODE_UNASSIGNED;
    //bool prev_word_p = false;
    bool curr_word_p = false;

    std::vector<std::size_t> breaks;
    std::set<std::size_t> suppress;

    for (; icp <= ncp; ++icp) {
        currwc = wchar_t(ucs4[icp]);
        curr_type = g_unichar_type(currwc);
        prev_class = curr_class;
        //prev_word_p = curr_word_p;

        switch (curr_type) {
        case G_UNICODE_DECIMAL_NUMBER:
        case G_UNICODE_OTHER_NUMBER:
            curr_class = numba;
            curr_word_p = true;
            break;
        case G_UNICODE_LOWERCASE_LETTER:
        case G_UNICODE_MODIFIER_LETTER:
        case G_UNICODE_OTHER_LETTER:
            curr_class = letta;
            curr_word_p = true;
            break;
        case G_UNICODE_UPPERCASE_LETTER:
        case G_UNICODE_TITLECASE_LETTER:
            curr_class = upper;
            curr_word_p = true;
            break;
        case G_UNICODE_OPEN_PUNCTUATION:
        case G_UNICODE_INITIAL_PUNCTUATION:
            curr_class = pinit;
            curr_word_p = false;
            break;
        case G_UNICODE_DASH_PUNCTUATION:
            curr_class = hyphn;
            if (currwc <= GENL_HYPH) {
                curr_word_p = true;
            } else if (currwc >= SMAL_HYPH) {
                curr_word_p = true;
            } else {
                curr_word_p = (currwc >= WAVE_DASH) && (currwc <= KANA_DHYP);
            }
            break;
        case G_UNICODE_CLOSE_PUNCTUATION:
        case G_UNICODE_FINAL_PUNCTUATION:
            curr_class = pfini;
            curr_word_p = false;
            break;
        case G_UNICODE_OTHER_PUNCTUATION:
            if (currwc == L'\'' || currwc == L'"') {
                curr_class = quote;
                curr_word_p = false;
            } else if (currwc == L'.' || currwc == IDEO_STOP || currwc == WIDE_STOP || currwc == KANA_MDOT) {
                curr_class = stops;
                curr_word_p = true;
            } else if (currwc == L'?' || currwc == '!' || currwc == WIDE_EXCL || currwc == WIDE_QUES) {
                curr_class = marks;
                curr_word_p = false;
            } else if (currwc == INVERT_QM || currwc == INVERT_EX) {
                curr_class = pinit;
                curr_word_p = false;
            } else if ( currwc == L'%' || currwc == WIDE_PCTS) {
                curr_class = pfpct;
                curr_word_p = true;
            } else {
                curr_class = empty;
                curr_word_p = false;
            }
            break;
        default:
            if (!g_unichar_isgraph(currwc)) {
                curr_class = blank;
            } else {
                curr_class = empty;
            }
            curr_word_p = false;
            break;
        }

        //  # condition for prefix test
        //  $words[$i] =~ /([\p{IsAlnum}\.\-]*)([\'\"\)\]\%\p{IsPf}]*)(\.+)$/
        //  $words[$i+1] =~ /^([ ]*[\'\"\(\[\¿\¡\p{IsPi}]*[ ]*[\p{IsUpper}0-9])/

        bool check_abbr_p = false;
        if (curr_class == stops) {
            if (prev_class != stops) {
                dotslen = 1;
            } else {
                dotslen++;
            }
        } else if (curr_word_p) {
            if (!fini_word) {
                init_word = ocp;
            }
            fini_word = ocp+1;
            dotslen = finilen = 0;
        } else if (curr_class >= quote && curr_class <= pfpct && curr_class != pinit) {
            finilen++;
            dotslen = 0;
            init_word = fini_word = 0;
        } else if (dotslen) {
            if (fini_word > init_word) {
                if (prev_class!=stops || seqpos<1 || (ocp-pos[seqpos-1])<dotslen)
                    check_abbr_p = false;
                else
                    check_abbr_p = dotslen < 2;
            }
            dotslen = 0;
        } else {
            init_word = fini_word = 0;
        }

        if (check_abbr_p) {
            // not a valid word character or post-word punctuation character:  check word
            std::wstring k((wchar_t *)uout+init_word,fini_word-init_word);
            if (finilen == 0 && nbpre_gen_ucs4.find(k) != nbpre_gen_ucs4.end()) {
                suppress.insert(std::size_t(ocp));
                seqpos = 0;
            } else {
                bool acro_p = false;
                bool found_upper_p = false;
                for (glong ii = init_word; ii < ocp; ++ii) {
                    if (uout[ii] == L'.') {
                        acro_p = true;
                    } else if (acro_p) {
                        if (uout[ii] != L'.' && uout[ii] != L'-') {
                            GUnicodeType i_type = g_unichar_type(uout[ii]);
                            if (i_type != G_UNICODE_UPPERCASE_LETTER) {
                                acro_p = false;
                            } else {
                                found_upper_p = true;
                            }
                        }
                    }
                }
                if (acro_p && found_upper_p) {
                    suppress.insert(std::size_t(ocp));
                    seqpos = 0;
                } else {
                    // check forward:
                    // $words[$i+1] =~ /^([ ]*[\'\"\(\[\¿\¡\p{IsPi}]*[ ]*[\p{IsUpper}0-9])/
                    int fcp = icp;
                    int state = (curr_class == pinit || curr_class == quote) ? 1 : 0;
                    bool num_p = true;
                    while (fcp < ncp) {
                        GUnicodeType f_type = g_unichar_type(ucs4[fcp]);
                        bool f_white = g_unichar_isgraph(ucs4[fcp]);
                        switch (state) {
                        case 0:
                            if (!f_white) {
                                ++fcp;
                                continue;
                            } else if (f_type == G_UNICODE_INITIAL_PUNCTUATION || f_type == G_UNICODE_OPEN_PUNCTUATION ||
                                       ucs4[fcp] == L'"'|| ucs4[fcp] == '\'' || ucs4[fcp] == INVERT_QM || ucs4[fcp] == INVERT_EX) {
                                num_p = false;
                                state = 1;
                                ++fcp;
                                continue;
                            } else if (f_type == G_UNICODE_UPPERCASE_LETTER || f_type == G_UNICODE_DECIMAL_NUMBER) {
                                if (num_p)
                                    num_p = f_type == G_UNICODE_DECIMAL_NUMBER;
                                state = 3;
                                ++fcp;
                            }
                            break;
                        case 1:
                            if (!f_white) {
                                ++fcp;
                                state = 2;
                                continue;
                            } else if (f_type == G_UNICODE_INITIAL_PUNCTUATION || f_type == G_UNICODE_OPEN_PUNCTUATION ||
                                       ucs4[fcp] == L'"'|| ucs4[fcp] == '\'' || ucs4[fcp] == INVERT_QM || ucs4[fcp] == INVERT_EX) {
                                ++fcp;
                                continue;
                            } else if (f_type == G_UNICODE_UPPERCASE_LETTER || f_type == G_UNICODE_DECIMAL_NUMBER) {
                                if (num_p)
                                    num_p = f_type == G_UNICODE_DECIMAL_NUMBER;
                                state = 3;
                                ++fcp;
                            }
                            break;
                        case 2:
                            if (!f_white) {
                                ++fcp;
                                continue;
                            } else if (f_type == G_UNICODE_UPPERCASE_LETTER || f_type == G_UNICODE_DECIMAL_NUMBER) {
                                if (num_p)
                                    num_p = f_type == G_UNICODE_DECIMAL_NUMBER;
                                state = 3;
                                ++fcp;
                                break;
                            }
                            break;
                        }
                        break;
                    }
                    if (num_p && state == 3 && nbpre_num_ucs4.find(k) != nbpre_num_ucs4.end()) {
                        suppress.insert(std::size_t(ocp));
                        seqpos = 0;
                    }
                }
            }
            init_word = fini_word = 0;
        }

        if (seqpos >= SEQ_LIM) {
            seqpos = 0;
        }

        if (curr_class == stops || curr_class == marks) {
            if (!seqpos) {
                seq[seqpos] = curr_class;
                pos[seqpos] = ocp;
                seqpos++;
                uout[ocp++] = gunichar(currwc);
                continue;
            } else if (seqpos>1 && (seq[seqpos-1]==blank || seq[seqpos-1]==quote || seq[seqpos-1]==pfini)) {
                // handle "[?!.] ..." which is common in some corpora
                if (seq[seqpos-2] == curr_class || seq[seqpos-2] == marks) {
                    seqpos--;
                    uout[ocp++] = gunichar(currwc);
                    continue;
                }
                seqpos = 0;
            } else if (seq[seqpos-1] != curr_class) {
                seqpos = 0;
            } else if (curr_class == marks) {
                seqpos = 0;
            } else {
                uout[ocp++] = gunichar(currwc);
                continue;
            }
        }

        if (!seqpos) {
            if (curr_class != blank) {
                uout[ocp++] = gunichar(currwc);
            } else if (curr_class != prev_class) {
                uout[ocp++] = L' ';
            }
            continue;
        }

        if (curr_class == blank) {
            if (prev_class != blank) {
                seq[seqpos] = blank;
                pos[seqpos] = ocp;
                seqpos++;
                uout[ocp++] = L' ';
            }
            if (icp < ncp)
                continue;
        }

        if (curr_class >= quote && curr_class <= pfini) {
            if (prev_class < quote || prev_class > pfini) {
                seq[seqpos] = curr_class;
                pos[seqpos] = ocp;
                seqpos++;
            } else if (curr_class == quote && prev_class != curr_class) {
                curr_class = prev_class;
            } else if (prev_class == quote) {
                seq[seqpos] = prev_class = curr_class;
            }
            uout[ocp++] = gunichar(currwc);
            continue;
        }

        //	$text =~ s/([?!]) +([\'\"\(\[\¿\¡\p{IsPi}]*[\p{IsUpper}])/$1\n$2/g;
        //	#multi-dots followed by sentence starters 2
        //  $text =~ s/(\.[\.]+) +([\'\"\(\[\¿\¡\p{IsPi}]*[\p{IsUpper}])/$1\n$2/g;
        //  # add breaks for sentences that end with some sort of punctuation inside a quote or parenthetical and are followed by a possible sentence starter punctuation and upper case 4
        //  $text =~ s/([?!\.][\ ]*[\'\"\)\]\p{IsPf}]+) +([\'\"\(\[\¿\¡\p{IsPi}]*[\ ]*[\p{IsUpper}])/$1\n$2/g;
        //  # add breaks for sentences that end with some sort of punctuation are followed by a sentence starter punctuation and upper case 8
        //  $text =~ s/([?!\.]) +([\'\"\(\[\¿\¡\p{IsPi}]+[\ ]*[\p{IsUpper}])/$1\n$2/g;

        std::size_t iblank = 0;
        if (curr_class == upper || icp == ncp) {
            if (seqpos && (seq[0] == stops || seq[0] == marks)) {
                switch (seqpos) {
                case 2:
                    if (seq[1] == blank)
                        iblank = 1;
                    break;
                case 3:
                    switch (seq[1]) {
                    case blank:
                        if (seq[2] == quote || seq[2] == pinit)
                            iblank = 1;
                        break;
                    case quote:
                    case pfini:
                        if (seq[2] == blank)
                            iblank = 2;
                        break;
                    default:
                        break;
                    }
                    break;
                case 4:
                    switch (seq[1]) {
                    case blank:
                        iblank = 1;
                        switch (seq[2]) {
                        case quote:
                            switch (seq[3]) {
                            case quote:
                            case pinit:
                                break;
                            case blank:
                                iblank = 3;
                                break;
                            default:
                                iblank = 0; // invalid
                                break;
                            }
                            break;
                        case pinit:
                            if (seq[3] != blank)
                                iblank = 0; // invalid
                            break;
                        case pfini:
                            if (seq[3] == blank)
                                iblank = 3;
                            break;
                        default:
                            iblank = 0; // invalid
                            break;
                        }
                        break;
                    case quote:
                    case pfini:
                        iblank = (seq[2] == blank && (seq[3] == quote || seq[3] == pinit)) ? 2 : 0;
                        break;
                    default:
                        iblank = 0; // invalid
                        break;
                    }
                    break;
                case 5:
                    iblank = (seq[1] == blank) ? 2 : 1;
                    if (seq[iblank] == quote || seq[iblank] == pfini)
                        iblank++;
                    if (seq[iblank] != blank) {
                        iblank = 0; // invalid
                    } else {
                        if (seq[iblank+1] != quote && seq[iblank+1] != pinit) {
                            iblank = 0; // invalid
                        } else if (iblank+2 < seqpos) {
                            if (seq[iblank+2] != blank)
                                iblank = 0; // invalid
                        }
                    }
                    break;
                }
            }
            if (iblank && suppress.find(pos[iblank]) == suppress.end()) {
                breaks.push_back(pos[iblank]);
                suppress.insert(pos[iblank]);
            }
        }

        uout[ocp++] = gunichar(currwc);
        seqpos = 0;
    }

    std::vector<std::size_t>::iterator it = breaks.begin();
    glong iop = 0;
    while (iop < ocp) {
        glong endpos = it == breaks.end() ? ocp : *it++;
        glong nextpos = endpos + 1;
        while (endpos > iop) {
            std::size_t chkpos = endpos-1;
            if (uout[chkpos] == L'\n' || uout[chkpos] == L' ') {
                endpos = chkpos;
                continue;
            }
            if (g_unichar_isgraph(uout[chkpos]))
                break;
            endpos = chkpos;
        }
        if (endpos > iop) {
            gchar *pre = g_ucs4_to_utf8(uout+iop,endpos-iop,0,0,0);
            parts.push_back(std::string(pre));
            g_free(pre);
        }
        if (continuation_ptr)
            *continuation_ptr = endpos > iop;
        iop = nextpos;
    }

    g_free(uout);
    g_free(ucs4);

    return parts;
}


std::pair<std::size_t,std::size_t>
Tokenizer::splitter(std::istream& is, std::ostream& os)
{
    std::pair<std::size_t,std::size_t> counts = { 0, 0 };
    bool continuation_p = false;
    bool pending_gap = false;
    bool paragraph_p = false;

    while (is.good() && os.good()) {
        std::string istr;

        std::getline(is,istr);
        counts.first++;

        if (istr.empty() && (is.eof() ||!para_marks_p))
            continue;

        if (skip_xml_p && (RE2::FullMatch(istr,tag_line_x) || RE2::FullMatch(istr,white_line_x)))
            continue;

        std::vector<std::string> sentences(splitter(istr,&continuation_p));
        if (sentences.empty()) {
            if (!paragraph_p) {
                if (pending_gap)
                    os << std::endl;
                pending_gap = false;
                if (para_marks_p)
                    os << "<P>" << std::endl;
                paragraph_p = true;
            }
            continue;
        }

        paragraph_p = false;
        std::size_t nsents = sentences.size();
        counts.second += nsents;

        if (pending_gap) {
            os << " ";
            pending_gap = false;
        }

        for (std::size_t ii = 0; ii < nsents-1; ++ii)
            os << sentences[ii] << std::endl;

        os << sentences[nsents-1];

        if (continuation_p)
            pending_gap = !split_breaks_p;
        if (!pending_gap)
            os << std::endl;
    }

    if (pending_gap)
        os << std::endl;

    return counts;
}


#ifdef TOKENIZER_NAMESPACE
}; // namespace
#endif

