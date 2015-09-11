#include "NMT_Wrapper.h"
#include<iostream>
#include <string>
#include <boost/python.hpp>

using namespace std;
using namespace boost::python;

int main(int argc, char *argv[])
{
    string state_path = string(argv[1]);
    string model_path = string(argv[2]);
    NMT_Wrapper* wrapper = new NMT_Wrapper();
    bool res = wrapper->Init(state_path, model_path);
    if (res == false) cout << "No wrapper" << endl;
    double prob;
    PyObject* next_state = NULL;
    PyObject* current_state = NULL;

    string source_sentence;
    cout << "Input sentence:";
    getline(cin, source_sentence);
    while (1) {
        if (source_sentence.size() < 3) return 0;

        string last_word = "";
        while (true) {
            cout << "Next word: ";
            string next_word;
            cin >> next_word;
            bool res = wrapper->GetProb(next_word, source_sentence, last_word,
                                        current_state, prob, next_state);
            if (res == false) { cout << "gone wrong.\n"; }
            cout << "Word: " << next_word << "; Prob: " << prob << endl;
            cout << next_state << endl;
            current_state = next_state;
            last_word = next_word;
            if (next_word == ".") {
                getline(cin, source_sentence);
                cout << "Input sentence:";
                getline(cin, source_sentence);
                break;
            }
        }
    }

    return 0;
}
