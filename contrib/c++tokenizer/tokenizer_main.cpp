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
    std::cerr << "Usage: " << path << "[-{v|x|p|a|e|s|u]* [LL] [-{c|o} PATH]* INFILE*" << std::endl;
    std::cerr << " -a -- aggressive hyphenization" << std::endl;
    std::cerr << " -e -- escape entities" << std::endl;
    std::cerr << " -c DIR -- config (pattern) file directory" << std::endl;
    std::cerr << " -d -- downcase" << std::endl;
    std::cerr << " -o OUT -- output file path" << std::endl;
    std::cerr << " -p -- penn treebank style" << std::endl;
    std::cerr << " -s -- super- and sub-script conjoining" << std::endl;
    std::cerr << " -u -- disable url handling" << std::endl;
    std::cerr << " -v -- verbose" << std::endl;
    std::cerr << " -w -- word filter" << std::endl;
    std::cerr << " -x -- skip xml tag lines" << std::endl;
    std::cerr << " -y -- skip all xml tags" << std::endl;
    std::cerr << "Default is -c ., stdin, stdout." << std::endl;
    std::cerr << "LL in en,fr,it affect contraction." << std::endl;
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
        if (line.empty()) continue;
        std::vector<std::string> tokens(tize.tokens(line));
        int count = 0;
        for (auto& token: tokens) {
            std::string word(token_word(token));
            if (word.empty()) continue;
            ofs << word << ' ';
            count++;
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

    while (++av,--ac) { 
        if (**av == '-') {
            switch (av[0][1]) {
            case 'a':
                params.aggro_p = true;
                break;
            case 'h':
                usage(prog);
                exit(0);
            case 'c':
                params.next_cfg_p = true;
                break;
            case 'd':
                params.downcase_p = true;
                break;
            case 'e':
                params.escape_p = false;
                break;
            case 'o':
                params.next_output_p = true;
                break;
            case 'p':
                params.penn_p = true;
                break;
            case 's':
                params.supersub_p = true;
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
        } else if (params.lang_iso.empty() && strlen(*av) == 2) {
            params.lang_iso = *av;
        } else if (params.next_output_p) {
            params.next_output_p = false;
            params.out_path = *av;
        } else if (params.next_cfg_p) {
            params.next_cfg_p = false;
            params.cfg_path = *av;
        } else {
            params.args.push_back(std::string(*av));
        }
    }

    if (!params.cfg_path) {
        params.cfg_path = getenv("TOKENIZER_SHARED_DIR");
    }
    if (!params.cfg_path) {
        if (!::access("../shared/.",X_OK)) {
            if (!::access("../shared/moses/.",X_OK)) {
                params.cfg_path = "../shared/moses";
            } else {
                params.cfg_path = "../shared";
            }
        } else if (!::access("./shared/.",X_OK)) {
            if (!::access("./shared/moses/.",X_OK)) {
                params.cfg_path = "./shared/moses";
            } else {
                params.cfg_path = "./shared";
            }
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
        Tokenizer::set_config_dir(std::string(params.cfg_path));
    } 

    std::unique_ptr<std::ofstream> pofs = 0;
    if (!params.out_path.empty()) {
        pofs.reset(new std::ofstream(params.out_path.c_str()));
    }
    std::ostream& ofs(pofs ? *pofs : std::cout);

    Tokenizer tize(params.lang_iso,params.detag_p,params.alltag_p,!params.escape_p,params.aggro_p,params.supersub_p,params.url_p,params.downcase_p,params.penn_p,params.verbose_p);
    tize.init();
    size_t nlines = 0;

    if (params.words_p) {
        if (params.args.empty()) {
            nlines += copy_words(tize,std::cin,ofs);
        } else {
            for (std::string& arg : params.args) {
                try {
                    std::ifstream ifs(arg.c_str());
                    nlines += copy_words(tize,ifs,ofs);
                } catch (...) {
                    std::cerr << "Exception extracting words from path " << arg << std::endl;
                }
            }
        }
    } else if (params.args.empty()) {
        nlines = tize.tokenize(std::cin,ofs);
    } else {
        for (std::string& arg : params.args) {
            try {
                std::ifstream ifs(arg.c_str());
                nlines = tize.tokenize(ifs,ofs);
            } catch (...) {
                std::cerr << "Exception tokenizing from path " << arg << std::endl;
            }
        }
    }

    if (params.verbose_p)
        std::cerr << "%%% tokenized lines: " << nlines << std::endl;
    
    return rc;
}


