"""
A module for interfacing with ``split-sentences.perl`` from Moses toolkit.

Copyright ® 2016-2017, Luís Gomes <luismsgomes@gmail.com>
"""

usage = """
Usage:
    moses-sentence-splitter [options] <lang> [<inputfile> [<outputfile>]]
    moses-sentence-splitter --selftest [--verbose]

Options:
    --selftest, -t  Run selftests.
    --verbose, -v   Be more verbose.
    --unwrap, -u    Assume that the text is wrapped and try to unwrap it.
                    Note that this option will cause all consecutive non-empty
                    lines to be buffered in memory.  If you give this option
                    make sure that you have empty lines separating paragraphs.
                    When this option is not given, each line is assumed to be
                    an independent paragraph or sentence and thus will not be
                    joined with other lines.
    --more          Also split on colons and semi-colons.

2016, Luís Gomes <luismsgomes@gmail.com>
"""


from docopt import docopt
from openfile import openfile
from os import path
from toolwrapper import ToolWrapper
import sys


class MosesSentenceSplitter(ToolWrapper):
    """
    A class for interfacing with ``split-sentences.perl`` from Moses toolkit.

    This class communicates with split-sentences.perl process via pipes. When
    the MosesSentenceSplitter object is no longer needed, the close() method
    should be called to free system resources. The class supports the context
    manager interface. If used in a with statement, the close() method is
    invoked automatically.

    When attribute ``more`` is True, colons and semi-colons are considered
    sentence separators.

    >>> split_sents = MosesSentenceSplitter('en')
    >>> split_sents(['Hello World! Hello', 'again.'])
    ['Hello World!', 'Hello again.']

    """

    def __init__(self, lang="en", more=True):
        self.lang = lang
        program = path.join(
            path.dirname(__file__),
            "split-sentences.perl"
        )
        argv = ["perl", program, "-q", "-b", "-l", self.lang]
        if more:
            argv.append("-m")
        super().__init__(argv)

    def __str__(self):
        return "MosesSentenceSplitter(lang=\"{lang}\")".format(lang=self.lang)

    def __call__(self, paragraph):
        """Splits sentences within a paragraph.
        The paragraph is a list of non-empty lines.  XML-like tags are not
         allowed.
        """
        assert isinstance(paragraph, (list, tuple))
        if not paragraph:  # empty paragraph is OK
            return []
        assert all(isinstance(line, str) for line in paragraph)
        paragraph = [line.strip() for line in paragraph]
        assert all(paragraph), "blank lines are not allowed"
        for line in paragraph:
            self.writeline(line)
        self.writeline("<P>")
        sentences = []
        while True:
            sentence = self.readline().strip()
            if sentence == "<P>":
                break
            sentences.append(sentence)
        return sentences


def read_paragraphs(inputfile, wrapped=True):
    lines = map(str.strip, inputfile)
    if wrapped:
        paragraph = []
        for line in lines:
            if line:
                paragraph.append(line)
            elif paragraph:
                yield paragraph
                paragraph = []
        if paragraph:
            yield paragraph
    else:
        for line in lines:
            yield [line] if line else []


def write_paragraphs(paragraphs, outputfile, blank_sep=True):
    for paragraph in paragraphs:
        for sentence in paragraph:
            print(sentence, file=outputfile)
        if blank_sep or not paragraph:
            print(file=outputfile)  # paragraph separator


def main():
    args = docopt(usage)
    if args["--selftest"]:
        import doctest
        import mosestokenizer.sentsplitter
        doctest.testmod(mosestokenizer.sentsplitter)
        if not args["<lang>"]:
            sys.exit(0)
    split_sents = MosesSentenceSplitter(args["<lang>"], more=args["--more"])
    inputfile = openfile(args["<inputfile>"])
    outputfile = openfile(args["<outputfile>"], "wt")
    with inputfile, outputfile:
        paragraphs = read_paragraphs(inputfile, wrapped=args["--unwrap"])
        paragraphs = map(split_sents, paragraphs)
        write_paragraphs(paragraphs, outputfile, blank_sep=args["--unwrap"])


if __name__ == "__main__":
    main()
