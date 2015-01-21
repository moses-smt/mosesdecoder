#include "tokenizer.h"
#include <memory>
#include <vector>
#include <cctype>

#ifdef TOKENIZER_NAMESPACE
using namespace TOKENIZER_NAMESPACE ;
#endif


void 
usage(const char *path) 
{
    std::cerr << "Usage: " << path << "[-{v|x|p|a|e|]* [LL] [-{c|o} PATH]* INFILE*" << std::endl;
    std::cerr << " -v -- verbose" << std::endl;
    std::cerr << " -w -- word filter" << std::endl;
    std::cerr << " -x -- skip xml tag lines" << std::endl;
    std::cerr << " -y -- skip all xml tags" << std::endl;
    std::cerr << " -e -- escape entities" << std::endl;
    std::cerr << " -a -- aggressive hyphenization" << std::endl;
    std::cerr << " -p -- treebank-3 style" << std::endl;
    std::cerr << " -c DIR -- config (pattern) file directory" << std::endl;
    std::cerr << " -o OUT -- output file path" << std::endl;
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
    if (last_quirk == pos || digits_prefixed > 0 && nalpha == 0)
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
    std::string lang_iso;
    std::vector<std::string> args;
    std::string out_path;
    char *cfg_path = 0;
    bool next_cfg_p = false;
    bool next_output_p = false;
    bool verbose_p = false;
    bool detag_p = false;
    bool alltag_p = false;
    bool escape_p = true;
    bool aggro_p = false;
    bool penn_p = false;
    bool words_p = false;

    const char *prog = av[0];
    while (++av,--ac) { 
        if (**av == '-') {
            switch (av[0][1]) {
            case 'h':
                usage(prog);
                exit(0);
            case 'c':
                next_cfg_p = true;
                break;
            case 'o':
                next_output_p = true;
                break;
            case 'v':
                verbose_p = true;
                break;
            case 'e':
                escape_p = false;
                break;
            case 'w':
                words_p = true;
                break;
            case 'x':
                detag_p = true;
                break;
            case 'y':
                alltag_p = true;
                break;
            case 'a':
                aggro_p = true;
                break;
            case 'l':
                // ignored
                break;
            case 'p':
                penn_p = true;
                break;
            default:
                std::cerr << "Unknown option: " << *av << std::endl;
                ::exit(1);
            }
        } else if (lang_iso.empty() && strlen(*av) == 2) {
            lang_iso = *av;
        } else if (**av == '-') {
            ++*av;
        } else if (next_output_p) {
            next_output_p = false;
            out_path = *av;
        } else if (next_cfg_p) {
            next_cfg_p = false;
            cfg_path = *av;
        } else {
            args.push_back(std::string(*av));
        }
    }

    if (!cfg_path) {
        cfg_path = getenv("TOKENIZER_SHARED_DIR");
    }
    if (cfg_path) {
        Tokenizer::set_config_dir(std::string(cfg_path));
    } 

    std::unique_ptr<std::ofstream> pofs = 0;
    if (!out_path.empty()) {
        pofs.reset(new std::ofstream(out_path.c_str()));
    }
    std::ostream& ofs(pofs ? *pofs : std::cout);

    Tokenizer tize(lang_iso,detag_p,alltag_p,!escape_p,aggro_p,penn_p,verbose_p);
    tize.init();
    size_t nlines = 0;

    if (words_p) {
        if (args.empty()) {
            nlines += copy_words(tize,std::cin,ofs);
        } else {
            for (std::string& arg : args) {
                try {
                    std::ifstream ifs(arg.c_str());
                    nlines += copy_words(tize,ifs,ofs);
                } catch (...) {
                    std::cerr << "Exception extracting words from path " << arg << std::endl;
                }
            }
        }
    } else if (args.empty()) {
        nlines = tize.tokenize(std::cin,ofs);
    } else {
        for (std::string& arg : args) {
            try {
                std::ifstream ifs(arg.c_str());
                nlines = tize.tokenize(ifs,ofs);
            } catch (...) {
                std::cerr << "Exception tokenizing from path " << arg << std::endl;
            }
        }
    }

    if (verbose_p)
        std::cerr << "%%% tokenized lines: " << nlines << std::endl;
    
    return rc;
}


