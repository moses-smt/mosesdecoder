"""
A module for interfacing with ``detokenizer.perl`` from Moses.

Copyright ® 2017, Luís Gomes <luismsgomes@gmail.com>
"""

usage = """
Usage:
    moses-detokenizer [options] <lang> [<inputfile> [<outputfile>]]
    moses-detokenizer --selftest [--verbose]

Options:
    --selftest, -t  Run selftests.
    --verbose, -v   Be more verbose.

2017, Luís Gomes <luismsgomes@gmail.com>
"""


from docopt import docopt
from openfile import openfile
from os import path
from toolwrapper import ToolWrapper
import sys


class MosesDetokenizer(ToolWrapper):
    """A module for interfacing with ``detokenizer.perl`` from Moses.

    This class communicates with detokenizer.perl process via pipes. When the
    MosesDetokenizer object is no longer needed, the close() method should be
    called to free system resources. The class supports the context manager
    interface. If used in a with statement, the close() method is invoked
    automatically.

    >>> detokenize = MosesDetokenizer('en')
    >>> detokenize('Hello', 'World', '!')
    'Hello World!'
    """

    def __init__(self, lang="en"):
        self.lang = lang
        program = path.join(path.dirname(__file__), "detokenizer.perl")
        # -q = quiet
        # -b = disable output buffering
        argv = ["perl", program, "-q", "-b", "-l", self.lang]
        super().__init__(argv)

    def __str__(self):
        return "MosesDetokenizer(lang=\"{lang}\")".format(lang=self.lang)

    def __call__(self, sentence):
        """Detokenizes a single sentence.

        Newline characters are not allowed in tokens.
        """
        assert isinstance(sentence, (list, tuple))
        assert all(isinstance(token, str) for token in sentence)
        assert all("\n" not in token for token in sentence)
        if not sentence:
            return ""
        self.writeline(" ".join(sentence))
        return self.readline()


def main():
    args = docopt(usage)
    if args["--selftest"]:
        import doctest
        import mosestokenizer.detokenizer
        doctest.testmod(mosestokenizer.detokenizer)
        if not args["<lang>"]:
            sys.exit(0)
    detokenize = MosesDetokenizer(args["<lang>"])
    inputfile = openfile(args["<inputfile>"])
    outputfile = openfile(args["<outputfile>"], "wt")
    with inputfile, outputfile:
        for line in inputfile:
            print(detokenize(line.split()), file=outputfile)

if __name__ == "__main__":
    main()
