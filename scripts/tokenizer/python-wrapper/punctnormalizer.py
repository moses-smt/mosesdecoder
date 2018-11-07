"""
A module for interfacing with ``normalize-punctuation.perl`` from Moses.

Copyright ® 2016-2017, Luís Gomes <luismsgomes@gmail.com>
"""

usage = """
Usage:
    moses-punct-normalizer [options] <lang> [<inputfile> [<outputfile>]]
    moses-punct-normalizer --selftest [--verbose]

Options:
    --selftest, -t  Run selftests.
    --verbose, -v   Be more verbose.

2016, Luís Gomes <luismsgomes@gmail.com>
"""


from docopt import docopt
from os import path
from toolwrapper import ToolWrapper
import sys


class MosesPunctuationNormalizer(ToolWrapper):
    """A module for interfacing with ``normalize-punctuation.perl`` from Moses.

    This class communicates with normalize-punctuation.perl process via pipes.
    When the MosesPunctuationNormalizer object is no longer needed, the close()
    method should be called to free system resources. The class supports the
    context manager interface. If used in a with statement, the close() method
    is invoked automatically.

    >>> normalize = MosesPunctuationNormalizer("en")
    >>> normalize("«Hello World» — she said…")
    '"Hello World" - she said...'
    """

    def __init__(self, lang="en"):
        self.lang = lang
        program = path.join(
            path.dirname(__file__),
            "normalize-punctuation.perl"
        )
        argv = ["perl", program, "-b", "-l", self.lang]
        super().__init__(argv)

    def __str__(self):
        return "MosesPunctuationNormalizer(lang=\"{lang}\")".format(
            lang=self.lang
        )

    def __call__(self, line):
        """Normalizes punctuation of a single line of text.

        Newline characters are not allowed in the text to be normalized.
        """
        assert isinstance(line, str)
        line = line.strip()
        assert "\n" not in line
        if not line:
            return []
        self.writeline(line)
        return self.readline()


def main():
    args = docopt(usage)
    if args["--selftest"]:
        import doctest
        import mosestokenizer.punctnormalizer
        doctest.testmod(mosestokenizer.punctnormalizer)
        if not args["<lang>"]:
            sys.exit(0)
    normalize = MosesPunctuationNormalizer(args["<lang>"])
    inputfile = open(args["<inputfile>"]) if args["<inputfile>"] else sys.stdin
    outputfile = open(args["<outputfile>"], "wt") if args["<outputfile>"] else sys.stdout
    with inputfile, outputfile:
        for line in inputfile:
            print(normalize(line), file=outputfile)

if __name__ == "__main__":
    main()
