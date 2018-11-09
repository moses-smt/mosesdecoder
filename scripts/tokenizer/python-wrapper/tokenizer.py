"""
A module for interfacing with ``tokenizer.perl`` from Moses.

Copyright ® 2016-2017, Luís Gomes <luismsgomes@gmail.com>
"""

usage = """
Usage:
    moses-tokenizer [options] <lang> [<inputfile> [<outputfile>]]
    moses-tokenizer --selftest [--verbose]

Options:
    --selftest, -t  Run selftests.
    --verbose, -v   Be more verbose.
    --old           Use older version (1.0) of the tokenizer.
                    If this option is not given, then version 1.1
                    will be used.

2016, Luís Gomes <luismsgomes@gmail.com>
"""


from docopt import docopt
from openfile import openfile
from os import path
from toolwrapper import ToolWrapper
import sys


class MosesTokenizer(ToolWrapper):
    """A module for interfacing with ``tokenizer.perl`` from Moses.

    This class communicates with tokenizer.perl process via pipes. When the
    MosesTokenizer object is no longer needed, the close() method should be
    called to free system resources. The class supports the context manager
    interface. If used in a with statement, the close() method is invoked
    automatically.

    >>> tokenize = MosesTokenizer('en')
    >>> tokenize('Hello World!')
    ['Hello', 'World', '!']
    """

    def __init__(self, lang="en"):
        self.lang = lang
        program = path.join(
            path.dirname(__file__),
            "../tokenizer.perl"
        )
        argv = ["perl", program, "-q", "-l", self.lang]

        # -b = disable output buffering
        # -a = aggressive hyphen splitting
        argv.extend(["-b", "-a"])
        super().__init__(argv)

    def __str__(self):
        return "MosesTokenizer(lang=\"{lang}\")".format(lang=self.lang)

    def __call__(self, sentence):
        """Tokenizes a single sentence.

        Newline characters are not allowed in the sentence to be tokenized.
        """
        assert isinstance(sentence, str)
        sentence = sentence.rstrip("\n")
        assert "\n" not in sentence
        if not sentence:
            return []
        self.writeline(sentence)
        return self.readline().split()


def main():
    args = docopt(usage)
    if args["--selftest"]:
        import doctest
        import mosestokenizer.tokenizer
        doctest.testmod(mosestokenizer.tokenizer)
        if not args["<lang>"]:
            sys.exit(0)
    tokenize = MosesTokenizer(
        args["<lang>"]
    )
    inputfile = openfile(args["<inputfile>"])
    outputfile = openfile(args["<outputfile>"], "wt")
    with inputfile, outputfile:
        for line in inputfile:
            print(*tokenize(line), file=outputfile)

if __name__ == "__main__":
    main()
