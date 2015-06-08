#include "tokenizer.h"
#include "Parameters.h"
#include <memory>
#include <vector>
#include <cctype>
#include <cstring>

#ifdef TOKENIZER_NAMESPACE
using namespace TOKENIZER_NAMESPACE ;
#endif


void
usage(const char *path)
{
    std::cerr << "Usage: " << path << "[-{v|x|p|a|e|s|u|n|N]* [LL] [-{c|o} PATH]* INFILE*" << std::endl;
    std::cerr << " -a -- aggressive hyphenization" << std::endl;
    std::cerr << " -b -- drop bad bytes" << std::endl;
    std::cerr << " -B -- splitter will split on linebreak" << std::endl;
    std::cerr << " -c DIR -- config (pattern) file directory" << std::endl;
    std::cerr << " -d -- downcase" << std::endl;
    std::cerr << " -D -- detokenize" << std::endl;
    std::cerr << " -e -- do not escape entities during tokenization" << std::endl;
    std::cerr << " -E -- preserve entities during tokenization" << std::endl;
    std::cerr << " -k -- narrow kana" << std::endl;
    std::cerr << " -n -- narrow latin" << std::endl;
    std::cerr << " -N -- normalize" << std::endl;
    std::cerr << " -o OUT -- output file path" << std::endl;
    std::cerr << " -p -- penn treebank style" << std::endl;
    std::cerr << " -r -- refined contraction and quantity conjoining" << std::endl;
    std::cerr << " -s -- super- and sub-script conjoining" << std::endl;
    std::cerr << " -S -- buffer and sentence-split lines" << std::endl;
    std::cerr << " -T -- do not tokenize, just split, no <P> marks" << std::endl;
    std::cerr << " -t N[,C] -- use N threads (1), chunksize C lines" << std::endl;
    std::cerr << " -u -- disable url handling" << std::endl;
    std::cerr << " -U -- unescape entities before tokenization, after detokenization" << std::endl;
    std::cerr << " -v -- verbose" << std::endl;
    std::cerr << " -w -- word filter" << std::endl;
    std::cerr << " -x -- skip xml tag lines" << std::endl;
    std::cerr << " -y -- skip all xml tags" << std::endl;
    std::cerr << " -X -- split only, with <P> marks" << std::endl;
    std::cerr << "Default is -c ., stdin, stdout." << std::endl;
    std::cerr << "LL in en,fr,it affect contraction.  LL selects nonbreaking prefix file" << std::endl;
    std::cerr << "nonbreaking_prefix.LL is sought in getenv('TOKENIZER_SHARED_DIR')." << std::endl;
    return;
}


std::string token_word(const std::string& in) {
    int pos = -1;
    int digits_prefixed = 0;
    int nalpha = 0;
    int len = in.size();
    std::vector<char> cv;
    int last_quirk = -1;
    while (++pos < len) {
        char ch = in.at(pos);
        if (std::isdigit(ch)) {
            if (digits_prefixed > 0) {
                last_quirk = pos;
                break;
            }
            digits_prefixed--;
            cv.push_back(std::tolower(ch));
        } else if (std::isalpha(ch)) {
            if (digits_prefixed < 0)
                digits_prefixed = -digits_prefixed;
            cv.push_back(std::tolower(ch));
            nalpha++;
        } else {
            if (digits_prefixed < 0)
                digits_prefixed = -digits_prefixed;
            last_quirk = pos;
            if ((ch == '-' || ch == '\'') && pos != 0) {
                cv.push_back(ch);
            } else {
                break;
            }
        }
    }
    if (last_quirk == pos || (digits_prefixed > 0 && nalpha == 0))
        cv.clear(); // invalid word
    return std::string(cv.begin(),cv.end());
}


int
copy_words(Tokenizer& tize, std::istream& ifs, std::ostream& ofs) {
    int nlines = 0;
    std::string line;
    while (ifs.good() && std::getline(ifs,line)) {
        if (line.empty())
            continue;
        std::vector<std::string> tokens(tize.tokens(line));
        int count = 0;
        bool was_break = false;

        for (auto& token: tokens) {
            if (token.empty()) {
                if (count || was_break) {
                    ofs << std::endl;
                    count = 0;
                    nlines++;
                    was_break = true;
                    continue;
                }
            }
            was_break = false;

            std::string word(token_word(token));
            if (word.empty()) {
                continue;
            }

            if (count++) {
                ofs << ' ';
            }
            ofs << word;
        }

        if (count) {
            ofs << std::endl;
            nlines++;
        }
    }
    return nlines;
}


int main(int ac, char **av)
{
    int rc = 0;
    Parameters params;

    const char *prog = av[0];
    bool next_cfg_p = false;
    bool next_output_p = false;
    bool next_threads_p = false;
    bool detokenize_p = std::strstr(av[0],"detokenize") != 0;
    if (!detokenize_p)
        params.split_p = std::strstr(av[0],"splitter") != 0;

    while (++av,--ac) {
        if (**av == '-') {
            switch (av[0][1]) {
            case 'a':
                params.aggro_p = true;
                break;
            case 'b':
                params.drop_bad_p = true;
                break;
            case 'B':
                params.split_breaks_p = true;
                break;
            case 'c':
                next_cfg_p = true;
                break;
            case 'd':
                params.downcase_p = true;
                break;
            case 'D':
                detokenize_p = !detokenize_p;
                break;
            case 'e':
                params.escape_p = !params.escape_p;
                break;
            case 'E':
                params.entities_p = true;
                break;
            case 'h':
                usage(prog);
                exit(0);
            case 'k':
                params.narrow_kana_p = true;
                break;
            case 'n':
                params.narrow_latin_p = true;
                break;
            case 'N':
                params.normalize_p = true;
                break;
            case 'o':
                next_output_p = true;
                break;
            case 'p':
                params.penn_p = true;
                break;
            case 'r':
                params.refined_p = true;
                break;
            case 's':
                params.supersub_p = true;
                break;
            case 'S':
                params.split_p = !params.split_p;
                break;
            case 'T':
                params.notokenization_p = true;
                params.para_marks_p = false;
                break;
            case 't':
                next_threads_p = true;
                break;
            case 'U':
                params.unescape_p = true;
                break;
            case 'u':
                params.url_p = false;
                break;
            case 'v':
                params.verbose_p = true;
                break;
            case 'w':
                params.words_p = true;
                break;
            case 'x':
                params.detag_p = true;
                break;
            case 'X':
                params.notokenization_p = true;
                params.para_marks_p = true;
                break;
            case 'y':
                params.alltag_p = true;
                break;
            case 'l':
                // ignored
                break;
            default:
                std::cerr << "Unknown option: " << *av << std::endl;
                ::exit(1);
            }
        } else if (params.lang_iso.empty() && strlen(*av) == 2 && !isdigit(**av)) {
            params.lang_iso = *av;
        } else if (next_output_p) {
            next_output_p = false;
            params.out_path = *av;
        } else if (next_cfg_p) {
            next_cfg_p = false;
            params.cfg_path = *av;
        } else if (next_threads_p) {
            next_threads_p = false;
            char *comma = strchr(*av,',');
            if (comma) {
                *comma++ = 0;
                params.chunksize = std::strtoul(comma,0,0);
            }
            params.nthreads = std::strtoul(*av,0,0);
        } else {
            params.args.push_back(std::string(*av));
        }
    }

    if (!params.cfg_path) {
        params.cfg_path = getenv("TOKENIZER_SHARED_DIR");
    }
    if (!params.cfg_path) {
        if (!::access("../share/.",X_OK)) {
            if (!::access("../share/moses/.",X_OK)) {
                params.cfg_path = "../share/moses";
            } else {
                params.cfg_path = "../share";
            }
        } else if (!::access("./scripts/share/.",X_OK)) {
            params.cfg_path = "./scripts/share";
        } else if (!::access("./nonbreaking_prefix.en",R_OK)) {
            params.cfg_path = ".";
        } else {
            const char *slash = std::strrchr(prog,'/');
            if (slash) {
                std::string cfg_dir_str(prog,slash-prog);
                std::string cfg_shr_str(cfg_dir_str);
                cfg_shr_str.append("/shared");
                std::string cfg_mos_str(cfg_shr_str);
                cfg_mos_str.append("/moses");
                if (!::access(cfg_mos_str.c_str(),X_OK)) {
                    params.cfg_path = strdup(cfg_mos_str.c_str());
                } else if (!::access(cfg_shr_str.c_str(),X_OK)) {
                    params.cfg_path = strdup(cfg_shr_str.c_str());
                } else if (!::access(cfg_dir_str.c_str(),X_OK)) {
                    params.cfg_path = strdup(cfg_dir_str.c_str());
                }
            }
        }
    }
    if (params.cfg_path) {
        if (params.verbose_p) {
            std::cerr << "config path: " << params.cfg_path << std::endl;
        }
    }

    std::unique_ptr<std::ofstream> pofs = 0;
    if (!params.out_path.empty()) {
        pofs.reset(new std::ofstream(params.out_path.c_str()));
    }
    std::ostream& ofs(pofs ? *pofs : std::cout);

    if (params.lang_iso.empty())
        params.lang_iso = "en";

    Tokenizer tize(params);
    tize.init();
    std::pair<std::size_t,std::size_t> plines = { 0, 0 };

    if (params.words_p) {
        if (params.args.empty()) {
            plines.first += copy_words(tize,std::cin,ofs);
        } else {
            for (std::string& arg : params.args) {
                try {
                    std::ifstream ifs(arg.c_str());
                    plines.first += copy_words(tize,ifs,ofs);
                } catch (...) {
                    std::cerr << "Exception extracting words from path " << arg << std::endl;
                }
            }
        }
    } else if (params.args.empty()) {
        if (detokenize_p) {
            plines.first = tize.detokenize(std::cin,ofs);
        } else if (params.notokenization_p) {
            plines = tize.splitter(std::cin,ofs);
        } else {
            plines.first = tize.tokenize(std::cin,ofs);
        }
    } else {
        for (std::string& arg : params.args) {
            try {
                std::ifstream ifs(arg.c_str());
                if (detokenize_p) {
                    plines.first = tize.detokenize(ifs,ofs);
                } else if (params.notokenization_p) {
                    plines = tize.splitter(ifs,ofs);
                } else {
                    plines.first = tize.tokenize(ifs,ofs);
                }
            } catch (...) {
                std::cerr << "Exception tokenizing from path " << arg << std::endl;
            }
        }
    }

    if (params.verbose_p) {
        std::cerr << "%%% " << plines.first << " lines." << std::endl;
        if (plines.second) {
            std::cerr << "%%% " << plines.second << " sentences." << std::endl;
        }
    }
    return rc;
}


