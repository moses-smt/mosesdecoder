#!/usr/bin/python

#
# Normalise the references or  nbest list, prior to statistic and feature extraction
#

import optparse,sys, math, re, xml.sax.saxutils

preserve_case = True

normalize1 = [
    ('<skipped>', ''),         # strip "skipped" tags
    (r'-\n', ''),              # strip end-of-line hyphenation and join lines
    (r'\n', ' '),              # join lines
#    (r'(\d)\s+(?=\d)', r'\1'), # join digits
]
normalize1 = [(re.compile(pattern), replace) for (pattern, replace) in normalize1]

normalize2 = [
    (r'([\{-\~\[-\` -\&\(-\+\:-\@\/])',r' \1 '), # tokenize punctuation. apostrophe is missing
    (r'([^0-9])([\.,])',r'\1 \2 '),              # tokenize period and comma unless preceded by a digit
    (r'([\.,])([^0-9])',r' \1 \2'),              # tokenize period and comma unless followed by a digit
    (r'([0-9])(-)',r'\1 \2 ')                    # tokenize dash when preceded by a digit
]
normalize2 = [(re.compile(pattern), replace) for (pattern, replace) in normalize2]

def normalize(s):
    '''Normalize and tokenize text. This is lifted from NIST mteval-v11a.pl.'''
    # Added to bypass NIST-style pre-processing of hyp and ref files -- wade
    if type(s) is not str:
        s = " ".join(s)
    # language-independent part:
    for (pattern, replace) in normalize1:
        s = re.sub(pattern, replace, s)
    s = xml.sax.saxutils.unescape(s, {'&quot;':'"'})
    # language-dependent part (assuming Western languages):
    s = " %s " % s
    if not preserve_case:
        s = s.lower()         # this might not be identical to the original
    for (pattern, replace) in normalize2:
        s = re.sub(pattern, replace, s)
    return s.split()

def process_nbest():
    print>>sys.stderr, "Processing nbest file"
    for line in sys.stdin:
        sep = "||| "
        fields = line[:-1].split(sep)
        normalised = normalize(fields[1])
        fields[1] = " ".join(normalised) + "  "
        print>>sys.stdout,sep.join(fields)


def process_refs():
    print>>sys.stderr, "Processing text file"
    for line in sys.stdin:
        normalised = normalize(line[:-1])
        print>>sys.stdout,(" ".join(normalised))

def main():
    parser = optparse.OptionParser(usage="usage: %prog [options] < input > output")
    parser.add_option("-n","--nbest",action="store_true",default=False,dest="nbest",
        help="Process nbest file")
    (options,args) = parser.parse_args()
    if options.nbest:
        process_nbest()
    else:
        process_refs()

if __name__ == "__main__":
    main()

